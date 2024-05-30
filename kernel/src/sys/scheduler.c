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
MDS_HOOK_INIT(SCHEDULER_SWITCH, MDS_Thread_t *toThread, MDS_Thread_t *fromThread);

/* Define ------------------------------------------------------------------ */
#if (defined(MDS_DEBUG_SCHEDULER) && (MDS_DEBUG_SCHEDULER > 0))
#define MDS_SCHEDULER_PRINT(fmt, ...) MDS_LOG_D("[SCHEDULER]" fmt, ##__VA_ARGS__)
#else
#define MDS_SCHEDULER_PRINT(fmt, ...)
#endif

/* Variable ---------------------------------------------------------------- */
static int g_sysCriticalNest = 0;
static MDS_Thread_t *g_sysCurrThread = NULL;
static MDS_ListNode_t g_sysSchedulerDefunct = {.prev = &g_sysSchedulerDefunct, .next = &g_sysSchedulerDefunct};

/* MLFQ Scheduler ---------------------------------------------------------- */
#if (MDS_THREAD_PRIORITY_MAX > 32)
#error "kernel mlfq scheduler supported max priority level:32 [0-31]"
#endif

static size_t g_sysThreadPrioMask = 0x00U;
static MDS_ListNode_t g_sysSchedulerTable[MDS_THREAD_PRIORITY_MAX];

static void MDS_SchedulerInit(void)
{
    MDS_SCHEDULER_PRINT("scheduler init with max priority:%u", ARRAY_SIZE(g_sysSchedulerTable));

    g_sysThreadPrioMask = 0x00U;
    MDS_SkipListInitNode(g_sysSchedulerTable, ARRAY_SIZE(g_sysSchedulerTable));
}

void MDS_SchedulerInsertThread(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread->currPrio < MDS_THREAD_PRIORITY_MAX);

    if (MDS_KernelCurrentThread() == thread) {
        thread->state = (thread->state & ~MDS_THREAD_STATE_MASK) | MDS_THREAD_STATE_RUNNING;
    } else {
        thread->state = (thread->state & ~MDS_THREAD_STATE_MASK) | MDS_THREAD_STATE_READY;

        MDS_ListRemoveNode(&(thread->node));

        if ((thread->state & MDS_THREAD_STATE_YIELD) != 0U) {
            MDS_ListInsertNodePrev(&(g_sysSchedulerTable[thread->currPrio]), &(thread->node));
        } else {
            MDS_ListInsertNodeNext(&(g_sysSchedulerTable[thread->currPrio]), &(thread->node));
        }

        g_sysThreadPrioMask |= (1UL << thread->currPrio);

        MDS_SCHEDULER_PRINT("scheduler insert thread(%p) entry:%p sp:%u priority:%u", thread, thread->entry,
                            thread->stackPoint, thread->currPrio);
    }
}

void MDS_SchedulerRemoveThread(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread->currPrio < MDS_THREAD_PRIORITY_MAX);

    MDS_SCHEDULER_PRINT("scheduler remove thread(%p) entry:%p sp:%u priority:%u", thread, thread->entry,
                        thread->stackPoint, thread->currPrio);

    MDS_ListRemoveNode(&(thread->node));

    if (MDS_ListIsEmpty(&(g_sysSchedulerTable[thread->currPrio]))) {
        g_sysThreadPrioMask &= ~(1UL << thread->currPrio);
    }
}

__attribute__((weak)) size_t MDS_SchedulerFFS(size_t value)
{
#if defined(__GNUC__)
    return (__builtin_ffs(value));
#else
    size_t num = 0;
#if __SIZE_MAX__ == __UINT64_MAX__
    if ((value & 0xFFFFFFFF) == 0) {
        num += 0x20;
        value >>= 0x20;
    }
#endif
    if ((value & 0xFFFF) == 0) {
        num += 0x10;
        value >>= 0x10;
    }
    if ((value & 0xFF) == 0) {
        num += 0x8;
        value >>= 0x8;
    }
    if ((value & 0xF) == 0) {
        num += 0x4;
        value >>= 0x4;
    }
    if ((value & 0x3) == 0) {
        num += 0x2;
        value >>= 0x2;
    }
    if ((value & 0x1) == 0) {
        num += 0x1;
    }
    return (num);

#endif
}

static MDS_Thread_t *MDS_SchedulerGetHighestPriorityThread(void)
{
    MDS_Thread_t *thread = NULL;
    size_t highestPrio = MDS_SchedulerFFS(g_sysThreadPrioMask);
    if (highestPrio != 0U) {
        thread = CONTAINER_OF(g_sysSchedulerTable[highestPrio - 1].next, MDS_Thread_t, node);
    }

    return (thread);
}

/* Scheduler --------------------------------------------------------------- */
void MDS_SchedulerCheck(void)
{
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    do {
        if (MDS_KernelGetCritical() != 0) {
            break;
        }

        MDS_Thread_t *toThread = MDS_SchedulerGetHighestPriorityThread();
        if (toThread == NULL) {
            break;
        }

        MDS_Thread_t *currThread = g_sysCurrThread;
        bool threadReady = false;

        if ((currThread->state & MDS_THREAD_STATE_MASK) == MDS_THREAD_STATE_RUNNING) {
            if (currThread->currPrio < toThread->currPrio) {
                toThread = currThread;
            } else if ((currThread->currPrio == toThread->currPrio) &&
                       ((currThread->state & MDS_THREAD_STATE_YIELD) == 0)) {
                toThread = currThread;
            } else {
                threadReady = true;
            }
            currThread->state &= ~MDS_THREAD_STATE_YIELD;
        }

        if (toThread != currThread) {
            g_sysCurrThread = toThread;
            if (threadReady) {
                MDS_SchedulerInsertThread(currThread);
            }

            MDS_SchedulerRemoveThread(toThread);
            toThread->state = (toThread->state & ~MDS_THREAD_STATE_MASK) | MDS_THREAD_STATE_RUNNING;

            MDS_SCHEDULER_PRINT(
                "scheduler switch to thread(%p) entry:%p sp:%p priority:%u from thread(%p) entry:%p sp:%p", toThread,
                toThread->entry, toThread->stackPoint, toThread->currPrio, currThread, currThread->entry,
                currThread->stackPoint);

            if (!MDS_CoreThreadStackCheck(toThread)) {
                MDS_PANIC("scheduler switch to thread(%p) entry:%p stack has broken", toThread, toThread->entry,
                          toThread->stackPoint);
            }

            MDS_HOOK_CALL(SCHEDULER_SWITCH, toThread, currThread);

            MDS_CoreSchedulerSwitch(&(currThread->stackPoint), &(toThread->stackPoint));
        } else {
            MDS_SchedulerRemoveThread(toThread);
            toThread->state = (toThread->state & ~MDS_THREAD_STATE_MASK) | MDS_THREAD_STATE_RUNNING;
        }
    } while (0);

    MDS_CoreInterruptRestore(lock);
}

void MDS_SchedulerPushDefunct(MDS_Thread_t *thread)
{
    MDS_SCHEDULER_PRINT("scheduler push defunct thread(%p) entry:%p sp:%p priority:%u", thread, thread->entry,
                        thread->stackPoint, thread->currPrio);

    MDS_ListInsertNodePrev(&g_sysSchedulerDefunct, &(thread->node));
}

MDS_Thread_t *MDS_SchedulerPopDefunct(void)
{
    MDS_Thread_t *thread = NULL;

    if (!MDS_ListIsEmpty(&g_sysSchedulerDefunct)) {
        thread = CONTAINER_OF(g_sysSchedulerDefunct.next, MDS_Thread_t, node);

        MDS_SCHEDULER_PRINT("scheduler pop defunct thread(%p) entry:%p sp:%p priority:%u", thread, thread->entry,
                            thread->stackPoint, thread->currPrio);

        MDS_ListRemoveNode(&(thread->node));
    }

    return (thread);
}

/* Kernel ------------------------------------------------------------------ */
void MDS_KernelInit(void)
{
    MDS_SchedulerInit();

    MDS_SysTimerInit();

    MDS_IdleThreadInit();
}

void MDS_KernelStartup(void)
{
    MDS_Thread_t *toThread = MDS_SchedulerGetHighestPriorityThread();
    if (toThread == NULL) {
        MDS_PANIC("scheduler no thread to startup");
    }

    g_sysCurrThread = toThread;
    toThread->state = MDS_THREAD_STATE_RUNNING;

    MDS_SCHEDULER_PRINT("scheduler thread startup with thread(%p) entry:%p sp:%p priority:%u", toThread,
                        toThread->entry, toThread->stackPoint, toThread->currPrio);

    MDS_CoreSchedulerStartup(&(toThread->stackPoint));
}

MDS_Thread_t *MDS_KernelCurrentThread(void)
{
    return (g_sysCurrThread);
}

void MDS_KernelEnterCritical(void)
{
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    g_sysCriticalNest += 1;

    MDS_CoreInterruptRestore(lock);
}

void MDS_KernelExitCritical(void)
{
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    g_sysCriticalNest -= 1;
    if (g_sysCriticalNest > 0) {
        MDS_CoreInterruptRestore(lock);
        return;
    } else {
        g_sysCriticalNest = 0;
        MDS_CoreInterruptRestore(lock);
    }

    if (MDS_KernelCurrentThread() != NULL) {
        MDS_SchedulerCheck();
    }
}

size_t MDS_KernelGetCritical(void)
{
    return (g_sysCriticalNest);
}

MDS_Tick_t MDS_KernelGetSleepTick(void)
{
    MDS_Tick_t sleepTick = 0;

    register MDS_Item_t lock = MDS_CoreInterruptLock();
    if (g_sysThreadPrioMask == 0) {
        MDS_Tick_t nextTick = MDS_SysTimerNextTick();
        MDS_Tick_t currTick = MDS_SysTickGetCount();
        sleepTick = (nextTick > currTick) ? (nextTick - currTick) : (0);
    }
    MDS_CoreInterruptRestore(lock);

    return (sleepTick);
}

void MDS_KernelCompensateTick(MDS_Tick_t tickcount)
{
    register MDS_Item_t lock = MDS_CoreInterruptLock();
    MDS_Tick_t currTick = MDS_SysTickGetCount() + tickcount;
    MDS_SysTickSetCount(currTick);
    MDS_SysTimerCheck();
    MDS_CoreInterruptRestore(lock);
}
