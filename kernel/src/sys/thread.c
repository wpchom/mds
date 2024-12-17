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
#include "mds_log.h"

/* Hook -------------------------------------------------------------------- */
MDS_HOOK_INIT(THREAD_EXIT, MDS_Thread_t *thread);
MDS_HOOK_INIT(THREAD_INIT, MDS_Thread_t *thread);
MDS_HOOK_INIT(THREAD_RESUME, MDS_Thread_t *thread);
MDS_HOOK_INIT(THREAD_SUSPEND, MDS_Thread_t *thread);

/* Define ------------------------------------------------------------------ */
#if (defined(MDS_THREAD_DEBUG_ENABLE) && (MDS_THREAD_DEBUG_ENABLE > 0))
#define MDS_THREAD_DEBUG(fmt, args...) MDS_LOG_D(fmt, ##args)
#else
#define MDS_THREAD_DEBUG(fmt, args...)
#endif

/* Function ---------------------------------------------------------------- */
static void THREAD_Terminate(MDS_Thread_t *thread)
{
    MDS_Item_t lock = MDS_CoreInterruptLock();

    MDS_TimerDeInit(&(thread->timer));

    MDS_SchedulerRemoveThread(thread);
    if ((thread->state & MDS_THREAD_STATE_MASK) != MDS_THREAD_STATE_INACTIVED) {
        MDS_KernelPushDefunct(thread);
    }

    thread->state = MDS_THREAD_STATE_TERMINATED;

    MDS_CoreInterruptRestore(lock);
}

static void THREAD_Entry(void *arg)
{
    MDS_Thread_t *thread = (MDS_Thread_t *)arg;

    if (thread->entry != NULL) {
        thread->entry(thread->arg);
    }

    THREAD_Terminate(thread);

    MDS_HOOK_CALL(THREAD_EXIT, thread);

    MDS_KernelSchedulerCheck();
}

static void THREAD_Exit(void)
{
    MDS_PANIC("thread should exit");
}

static void THREAD_Timeout(MDS_Arg_t *arg)
{
    MDS_Thread_t *thread = (MDS_Thread_t *)arg;

    MDS_ASSERT(thread != NULL);
    MDS_ASSERT((thread->state & MDS_THREAD_STATE_MASK) == MDS_THREAD_STATE_BLOCKED);
    MDS_ASSERT(MDS_ObjectGetType(&(thread->object)) == MDS_OBJECT_TYPE_THREAD);

    MDS_Item_t lock = MDS_CoreInterruptLock();

    MDS_SchedulerInsertThread(thread);

    thread->err = MDS_ETIME;

    MDS_CoreInterruptRestore(lock);

    MDS_KernelSchedulerCheck();
}

static MDS_Err_t THREAD_Init(MDS_Thread_t *thread, MDS_ThreadEntry_t entry, MDS_Arg_t *arg, void *stackPool,
                             size_t stackSize, MDS_ThreadPriority_t priority, MDS_Tick_t ticks)
{
    MDS_ASSERT(entry != NULL);
    MDS_ASSERT(stackPool != NULL);
    MDS_ASSERT(stackSize > 0);
    MDS_ASSERT(ticks > 0);

    MDS_ListInitNode(&(thread->node));

    thread->entry = entry;
    thread->arg = arg;
    thread->stackSize = stackSize;
    thread->stackBase = stackPool;

    thread->stackPoint = MDS_CoreThreadStackInit(thread->stackBase, thread->stackSize, (void *)THREAD_Entry,
                                                 (void *)thread, (void *)THREAD_Exit);

    thread->eventMask = 0U;
    thread->eventOpt = 0U;
    thread->state = MDS_THREAD_STATE_INACTIVED;

    thread->initPrio = priority;
    thread->currPrio = priority;

    thread->initTick = ticks;
    thread->remainTick = ticks;

    thread->err = MDS_EOK;
    MDS_Err_t err = MDS_TimerInit(&(thread->timer), thread->object.name, MDS_TIMER_TYPE_ONCE | MDS_TIMER_TYPE_SYSTEM,
                                  THREAD_Timeout, (MDS_Arg_t *)thread);

    MDS_HOOK_CALL(THREAD_INIT, thread);

    return (err);
}

MDS_Err_t MDS_ThreadInit(MDS_Thread_t *thread, const char *name, MDS_ThreadEntry_t entry, MDS_Arg_t *arg,
                         void *stackPool, size_t stackSize, MDS_ThreadPriority_t priority, MDS_Tick_t ticks)
{
    MDS_ASSERT(thread != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(thread->object), MDS_OBJECT_TYPE_THREAD, name);
    if (err == MDS_EOK) {
        err = THREAD_Init(thread, entry, arg, stackPool, stackSize, priority, ticks);
        if (err != MDS_EOK) {
            MDS_ObjectDeInit(&(thread->object));
        }
    }

    return (err);
}

MDS_Err_t MDS_ThreadDeInit(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(thread->object)) == MDS_OBJECT_TYPE_THREAD);

    if ((thread->state & MDS_THREAD_STATE_MASK) != MDS_THREAD_STATE_TERMINATED) {
        THREAD_Terminate(thread);
    }

    return (MDS_EOK);
}

MDS_Thread_t *MDS_ThreadCreate(const char *name, MDS_ThreadEntry_t entry, MDS_Arg_t *arg, size_t stackSize,
                               MDS_ThreadPriority_t priority, MDS_Tick_t ticks)
{
    MDS_ASSERT(stackSize > 0);

    void *stackPool = MDS_SysMemAlloc(stackSize);

    if (stackPool != NULL) {
        MDS_Thread_t *thread = (MDS_Thread_t *)MDS_ObjectCreate(sizeof(MDS_Thread_t), MDS_OBJECT_TYPE_THREAD, name);
        if (thread != NULL) {
            if (THREAD_Init(thread, entry, arg, stackPool, stackSize, priority, ticks) == MDS_EOK) {
                return (thread);
            }
            MDS_ObjectDestory(&(thread->object));
        }
        MDS_SysMemFree(stackPool);
    }

    MDS_THREAD_DEBUG("thread entry:%p stack size:%u priority:%u ticks:%u create failed", entry, stackSize, priority,
                     ticks);

    return (NULL);
}

MDS_Err_t MDS_ThreadDestroy(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(thread->object)) == MDS_OBJECT_TYPE_THREAD);

    if ((thread->state & MDS_THREAD_STATE_MASK) != MDS_THREAD_STATE_TERMINATED) {
        THREAD_Terminate(thread);
    }

    return (MDS_EOK);
}

MDS_Err_t MDS_ThreadStartup(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(thread->object)) == MDS_OBJECT_TYPE_THREAD);

    if ((thread->state & MDS_THREAD_STATE_MASK) == MDS_THREAD_STATE_INACTIVED) {
        MDS_THREAD_DEBUG("thread(%p) entry:%p startup with sp:%p priority:%u", thread, thread->entry,
                         thread->stackPoint, thread->currPrio);

        thread->state = MDS_THREAD_STATE_BLOCKED;
        MDS_ThreadResume(thread);

        if (MDS_KernelCurrentThread() != NULL) {
            MDS_KernelSchedulerCheck();
        }
    }

    return (MDS_EOK);
}

MDS_Err_t MDS_ThreadResume(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(thread->object)) == MDS_OBJECT_TYPE_THREAD);

    MDS_THREAD_DEBUG("thread(%p) entry:%p state:%x to resume", thread, thread->entry, thread->state);

    if ((thread->state & MDS_THREAD_STATE_MASK) != MDS_THREAD_STATE_BLOCKED) {
        return (MDS_EAGAIN);
    }

    MDS_Item_t lock = MDS_CoreInterruptLock();

    MDS_TimerStop(&(thread->timer));
    MDS_SchedulerInsertThread(thread);

    MDS_CoreInterruptRestore(lock);

    MDS_HOOK_CALL(THREAD_RESUME, thread);

    return (MDS_EOK);
}

MDS_Err_t MDS_ThreadSuspend(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(thread->object)) == MDS_OBJECT_TYPE_THREAD);

    MDS_THREAD_DEBUG("thread(%p) entry:%p state:%x to suspend", thread, thread->entry, thread->state);

    MDS_ThreadState_t state = thread->state & MDS_THREAD_STATE_MASK;
    if ((state != MDS_THREAD_STATE_READY) && (state != MDS_THREAD_STATE_RUNNING)) {
        return (MDS_EAGAIN);
    }

    MDS_HOOK_CALL(THREAD_SUSPEND, thread);

    MDS_Item_t lock = MDS_CoreInterruptLock();

    thread->err = MDS_EOK;
    MDS_TimerStop(&(thread->timer));
    MDS_SchedulerRemoveThread(thread);
    thread->state = (thread->state & ~MDS_THREAD_STATE_MASK) | MDS_THREAD_STATE_BLOCKED;

    MDS_CoreInterruptRestore(lock);

    return (MDS_EOK);
}

MDS_Err_t MDS_ThreadDelayTick(MDS_Tick_t delay)
{
    MDS_Item_t lock = MDS_CoreInterruptLock();
    MDS_Thread_t *thread = MDS_KernelCurrentThread();

    MDS_ASSERT(thread != NULL);

    if (delay == 0) {
        thread->remainTick = thread->initTick;
        thread->state |= MDS_THREAD_FLAG_YIELD;
    } else {
        MDS_ThreadSuspend(thread);
        MDS_TimerStart(&(thread->timer), delay);
    }

    MDS_CoreInterruptRestore(lock);

    MDS_KernelSchedulerCheck();

    if (thread->err == MDS_ETIME) {
        thread->err = MDS_EOK;
    }

    return (thread->err);
}

MDS_Err_t MDS_ThreadChangePriority(MDS_Thread_t *thread, MDS_ThreadPriority_t priority)
{
    MDS_ASSERT(thread != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(thread->object)) == MDS_OBJECT_TYPE_THREAD);

    MDS_THREAD_DEBUG("thread(%p) entry:%p state:%x chanage priority:%u->%u", thread, thread->entry, thread->state,
                     thread->currPrio, priority);

    MDS_Item_t lock = MDS_CoreInterruptLock();

    if ((thread->state & MDS_THREAD_STATE_MASK) == MDS_THREAD_STATE_READY) {
        MDS_SchedulerRemoveThread(thread);
        thread->currPrio = priority;
        MDS_SchedulerInsertThread(thread);
    } else {
        thread->currPrio = priority;
    }

    MDS_CoreInterruptRestore(lock);

    return (MDS_EOK);
}

MDS_ThreadState_t MDS_ThreadGetState(const MDS_Thread_t *thread)
{
    return (thread->state & MDS_THREAD_STATE_MASK);
}
