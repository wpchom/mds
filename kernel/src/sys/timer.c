/**
 * Copyright (c) [2022] [pchom]
 * [MDS] is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 **/
/* Include ----------------------------------------------------------------- */
#include "kernel.h"

/* Hook -------------------------------------------------------------------- */
MDS_HOOK_INIT(TIMER_ENTER, MDS_Timer_t *timer);
MDS_HOOK_INIT(TIMER_EXIT, MDS_Timer_t *timer);
MDS_HOOK_INIT(TIMER_START, MDS_Timer_t *timer);
MDS_HOOK_INIT(TIMER_STOP, MDS_Timer_t *timer);

/* Define ------------------------------------------------------------------ */
#if (defined(MDS_DEBUG_TIMER) && (MDS_DEBUG_TIMER > 0))
#define MDS_TIMER_PRINT(fmt, ...) MDS_LOG_D("[TIMER]" fmt, ##__VA_ARGS__)
#else
#define MDS_TIMER_PRINT(fmt, ...)
#endif

/* Variable ---------------------------------------------------------------- */
static MDS_ListNode_t g_sysTimerSkipList[MDS_TIMER_SKIPLIST_LEVEL];

#ifdef MDS_THREAD_TIMER_ENABLE
#ifndef MDS_THREAD_TIMER_STACKSIZE
#define MDS_THREAD_TIMER_STACKSIZE 256
#endif

#ifndef MDS_THREAD_TIMER_PRIORITY
#define MDS_THREAD_TIMER_PRIORITY 0
#endif

#ifndef MDS_THREAD_TIMER_TICKS
#define MDS_THREAD_TIMER_TICKS 16
#endif

struct MDS_SoftTimer {
    bool isBusy;
    MDS_ListNode_t skipList[MDS_TIMER_SKIPLIST_LEVEL];
    uint8_t stack[MDS_THREAD_TIMER_STACKSIZE];
    MDS_Thread_t thread;
} g_softTimer;
#endif

/* Function ---------------------------------------------------------------- */
static int TIMER_SkipListCompare(const MDS_ListNode_t *node, const void *value)
{
    const MDS_Timer_t *timer = CONTAINER_OF(node, MDS_Timer_t, node);
    MDS_Tick_t ticklimit = *((const MDS_Tick_t *)(value));
    MDS_Tick_t diffTick = timer->ticklimit - ticklimit;

    return ((diffTick != 0) && (diffTick < MDS_TIMER_TICK_MAX)) ? (1) : (-1);
}

static void TIMER_Check(MDS_ListNode_t timerList[], size_t size, bool isSoft)
{
    if ((timerList[0].next == NULL) || (timerList[0].prev == NULL)) {
        return;
    }

    MDS_ListNode_t runList = {.prev = &runList, .next = &runList};
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    while (!MDS_ListIsEmpty(&timerList[size - 1])) {
        MDS_Tick_t currTick = MDS_SysTickGetCount();
        MDS_Timer_t *t = CONTAINER_OF(timerList[size - 1].next, MDS_Timer_t, node[size - 1]);

        if ((currTick - t->ticklimit) >= MDS_TIMER_TICK_MAX) {
            break;
        }

        MDS_SkipListRemoveNode(t->node, size);
        if ((t->flags & MDS_TIMER_TYPE_PERIOD) == 0U) {
            t->flags &= ~MDS_TIMER_FLAG_ACTIVED;
        }
        MDS_ListInsertNodeNext(&runList, &(t->node[size - 1]));

        MDS_HOOK_CALL(TIMER_ENTER, t);

#ifdef MDS_THREAD_TIMER_ENABLE
        if (t->entry != NULL) {
            if (isSoft) {
                g_softTimer.isBusy = true;
                MDS_CoreInterruptRestore(lock);
                t->entry(t->arg);
                lock = MDS_CoreInterruptLock();
                g_softTimer.isBusy = false;
            } else {
                t->entry(t->arg);
            }
        }
#else
        UNUSED(isSoft);
        if (t->entry != NULL) {
            t->entry(t->arg);
        }
#endif

        MDS_HOOK_CALL(TIMER_EXIT, t);

        MDS_TIMER_PRINT("timer(%p) entry:%p flag:%x exit current tick:%u", t, t->entry, t->flags, currTick);

        if (MDS_ListIsEmpty(&runList)) {
            continue;
        }

        MDS_ListRemoveNode(&(t->node[size - 1]));
        if ((t->flags & (MDS_TIMER_FLAG_ACTIVED | MDS_TIMER_TYPE_PERIOD)) ==
            (MDS_TIMER_FLAG_ACTIVED | MDS_TIMER_TYPE_PERIOD)) {
            MDS_TimerStart(t, t->ticklimit - t->tickstart);
        }
    }

    MDS_CoreInterruptRestore(lock);
}

static MDS_Timer_t *TIMER_NextTickoutTimer(MDS_ListNode_t timerList[], size_t size)
{
    MDS_Timer_t *timer = NULL;
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    if (!MDS_ListIsEmpty(&(timerList[size - 1]))) {
        timer = CONTAINER_OF(timerList[size - 1].next, MDS_Timer_t, node[size - 1]);
    }

    MDS_CoreInterruptRestore(lock);

    return (timer);
}

#ifdef MDS_THREAD_TIMER_ENABLE
static __attribute__((noreturn)) void TIMER_ThreadEntry(MDS_Arg_t *arg)
{
    UNUSED(arg);

    MDS_LOOP {
        MDS_Timer_t *nextTimer = TIMER_NextTickoutTimer(g_softTimer.skipList, ARRAY_SIZE(g_softTimer.skipList));
        MDS_Tick_t nextTick = (nextTimer != NULL) ? (nextTimer->ticklimit) : (MDS_TICK_FOREVER);

        MDS_TIMER_PRINT("soft timer thread take a next check:%u", nextTick);

        if (nextTick == MDS_TICK_FOREVER) {
            MDS_Thread_t *thread = MDS_KernelCurrentThread();
            MDS_ThreadSuspend(thread);
            MDS_KernelSchedulerCheck();
        } else {
            MDS_Tick_t currTick = MDS_SysTickGetCount();
            if ((nextTick - currTick) < MDS_TIMER_TICK_MAX) {
                nextTick = nextTick - currTick;
                MDS_ThreadDelay(nextTick);
            }
        }

        TIMER_Check(g_softTimer.skipList, ARRAY_SIZE(g_softTimer.skipList), true);
    }
}
#endif

MDS_Err_t MDS_TimerInit(MDS_Timer_t *timer, const char *name, MDS_Mask_t type, MDS_TimerEntry_t entry, MDS_Arg_t *arg)
{
    MDS_ASSERT(timer != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(timer->object), MDS_OBJECT_TYPE_TIMER, name);
    if (err == MDS_EOK) {
        MDS_SkipListInitNode(timer->node, ARRAY_SIZE(timer->node));
        timer->entry = entry;
        timer->arg = arg;
        timer->flags = type & (~MDS_TIMER_FLAG_ACTIVED);
    }

    return (err);
}

MDS_Err_t MDS_TimerDeInit(MDS_Timer_t *timer)
{
    MDS_TimerStop(timer);

    return (MDS_ObjectDeInit(&(timer->object)));
}

MDS_Timer_t *MDS_TimerCreate(const char *name, MDS_Mask_t type, MDS_TimerEntry_t entry, MDS_Arg_t *arg)
{
    MDS_Timer_t *timer = (MDS_Timer_t *)MDS_ObjectCreate(sizeof(MDS_Timer_t), MDS_OBJECT_TYPE_TIMER, name);
    if (timer != NULL) {
        MDS_SkipListInitNode(timer->node, ARRAY_SIZE(timer->node));
        timer->entry = entry;
        timer->arg = arg;
        timer->flags = type & (~MDS_TIMER_FLAG_ACTIVED);
    } else {
        MDS_TIMER_PRINT("timer entry:%p type:%x create failed", entry, type);
    }

    return (timer);
}

MDS_Err_t MDS_TimerDestroy(MDS_Timer_t *timer)
{
    MDS_TimerStop(timer);

    return (MDS_ObjectDestory(&(timer->object)));
}

MDS_Err_t MDS_TimerStart(MDS_Timer_t *timer, MDS_Tick_t timeout)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(timer->object)) == MDS_OBJECT_TYPE_TIMER);

    if (timeout >= MDS_TIMER_TICK_MAX) {
        return (MDS_EINVAL);
    }

    static size_t skipRand = 0;
#ifdef MDS_THREAD_TIMER_ENABLE
    MDS_ListNode_t *timerList = ((timer->flags & MDS_TIMER_TYPE_SYSTEM) == 0U) ? (g_softTimer.skipList)
                                                                               : (g_sysTimerSkipList);
#else
    MDS_ListNode_t *timerList = g_sysTimerSkipList;
#endif

    do {
        register MDS_Item_t lock = MDS_CoreInterruptLock();

        MDS_SkipListRemoveNode(timer->node, ARRAY_SIZE(timer->node));
        timer->flags &= ~MDS_TIMER_FLAG_ACTIVED;

        if (timeout == 0) {
            MDS_HOOK_CALL(TIMER_STOP, timer);
        } else {
            timer->tickstart = MDS_SysTickGetCount();
            timer->ticklimit = timer->tickstart + timeout;

            MDS_HOOK_CALL(TIMER_START, timer);

            MDS_ListNode_t *skipList = MDS_SkipListSearchNode(timerList, ARRAY_SIZE(timer->node), &(timer->ticklimit),
                                                              TIMER_SkipListCompare);

            skipRand = skipRand + timer->tickstart + 1;
            MDS_SkipListInsertNode(skipList, timer->node, ARRAY_SIZE(timer->node), skipRand, MDS_TIMER_SKIPLIST_SHIFT);
            timer->flags |= MDS_TIMER_FLAG_ACTIVED;

#ifdef MDS_THREAD_TIMER_ENABLE
            if (((timer->flags & MDS_TIMER_TYPE_SYSTEM) == 0U) &&
                ((!(g_softTimer.isBusy)) &&
                 ((g_softTimer.thread.state & MDS_THREAD_STATE_MASK) == MDS_THREAD_STATE_BLOCKED))) {
                MDS_ThreadResume(&(g_softTimer.thread));
                MDS_CoreInterruptRestore(lock);
                MDS_KernelSchedulerCheck();
                break;
            }
#endif
        }

        MDS_CoreInterruptRestore(lock);
    } while (0);

    MDS_TIMER_PRINT("timer(%p) entry:%p flag:%x start timeout:%u tick on:%u out:%u", timer, timer->entry, timer->flags,
                    timeout, timer->tickstart, timer->ticklimit);

    return (MDS_EOK);
}

MDS_Err_t MDS_TimerStop(MDS_Timer_t *timer)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(timer->object)) == MDS_OBJECT_TYPE_TIMER);

    if ((timer->flags & MDS_TIMER_FLAG_ACTIVED) != 0U) {
        register MDS_Item_t lock = MDS_CoreInterruptLock();

        MDS_HOOK_CALL(TIMER_STOP, timer);

        MDS_SkipListRemoveNode(timer->node, ARRAY_SIZE(timer->node));
        timer->flags &= ~MDS_TIMER_FLAG_ACTIVED;

        MDS_CoreInterruptRestore(lock);

        MDS_TIMER_PRINT("timer(%p) entry:%p flag:%x stop", timer, timer->entry, timer->flags);
    }

    return (MDS_EOK);
}

bool MDS_TimerIsActived(const MDS_Timer_t *timer)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(timer->object)) == MDS_OBJECT_TYPE_TIMER);

    register MDS_Item_t lock = MDS_CoreInterruptLock();
    bool isActived = ((timer->flags & MDS_TIMER_FLAG_ACTIVED) != 0U) ? (true) : (false);
    MDS_CoreInterruptRestore(lock);

    return (isActived);
}

void MDS_SysTimerInit(void)
{
    MDS_SkipListInitNode(g_sysTimerSkipList, ARRAY_SIZE(g_sysTimerSkipList));

#ifdef MDS_THREAD_TIMER_ENABLE
    MDS_SkipListInitNode(g_softTimer.skipList, ARRAY_SIZE(g_softTimer.skipList));

    MDS_Err_t err = MDS_ThreadInit(&(g_softTimer.thread), "timer", TIMER_ThreadEntry, NULL, &(g_softTimer.stack),
                                   sizeof(g_softTimer.stack), MDS_THREAD_TIMER_PRIORITY, MDS_THREAD_TIMER_TICKS);
    if (err == MDS_EOK) {
        MDS_ThreadStartup(&(g_softTimer.thread));
    }
#endif
}

void MDS_SysTimerCheck(void)
{
    TIMER_Check(g_sysTimerSkipList, ARRAY_SIZE(g_sysTimerSkipList), false);
}

MDS_Tick_t MDS_SysTimerNextTick(void)
{
    MDS_Timer_t *timer = TIMER_NextTickoutTimer(g_sysTimerSkipList, ARRAY_SIZE(g_sysTimerSkipList));
    if (timer == NULL) {
        return (MDS_TICK_FOREVER);
    }

#if (defined(MDS_DEBUG_TIMER) && (MDS_DEBUG_TIMER > 0))
    static MDS_Timer_t *last = NULL;
    if (last != NULL) {
        MDS_Tick_t currTick = MDS_SysTickGetCount();
        MDS_TIMER_PRINT("next timer(%p) entry:%p currTick:%u nextTick:%u diffTick:%u", timer, timer->entry, currTick,
                        timer->ticklimit, timer->ticklimit - currTick);
    }
    last = timer;
#endif

    return (timer->ticklimit);
}
