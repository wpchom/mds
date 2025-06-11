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

/* Define ------------------------------------------------------------------ */
MDS_LOG_MODULE_DECLARE(kernel, CONFIG_MDS_KERNEL_LOG_LEVEL);

/* Function ---------------------------------------------------------------- */
static MDS_Err_t THREAD_Terminate(MDS_Thread_t *thread)
{
    MDS_ThreadState_t state = MDS_ThreadGetState(thread);
    if (state == MDS_THREAD_STATE_TERMINATED) {
        return (MDS_EAGAIN);
    }

    MDS_TimerDeInit(&(thread->timer));
    MDS_SchedulerRemoveThread(thread);
    MDS_KernelPushDefunct(thread);

    return (MDS_EOK);
}

static void THREAD_Exit(void)
{
    MDS_PANIC("thread should exit");
}

static void THREAD_Entry(void *arg)
{
    MDS_Thread_t *thread = (MDS_Thread_t *)arg;

    if (thread->entry != NULL) {
        thread->entry(thread->arg);
    }
    MDS_HOOK_CALL(KERNEL, thread, (thread, MDS_KERNEL_TRACE_THREAD_EXIT));

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(thread->spinlock));
    if (err == MDS_EOK) {
        THREAD_Terminate(thread);
        MDS_ThreadSetState(thread, MDS_THREAD_STATE_TERMINATED);
    }
    MDS_SpinLockRelease(&(thread->spinlock));
    MDS_CoreInterruptRestore(lock);

    MDS_KernelSchedulerCheck();
}

static void THREAD_Timeout(const MDS_Timer_t *timer, MDS_Arg_t *arg)
{
    UNUSED(timer);

    MDS_Thread_t *thread = (MDS_Thread_t *)arg;

    MDS_ASSERT(thread != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(thread->object)) == MDS_OBJECT_TYPE_THREAD);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(thread->spinlock));
    if (err == MDS_EOK) {
        MDS_ThreadState_t state = MDS_ThreadGetState(thread);
        if (state == MDS_THREAD_STATE_SUSPENDED) {
            MDS_SchedulerInsertThread(thread);
            thread->err = MDS_ETIMEOUT;
        } else {
            thread->err = MDS_EACCES;
            MDS_LOG_W();
        }
    }
    MDS_SpinLockRelease(&(thread->spinlock));
    MDS_CoreInterruptRestore(lock);

    MDS_KernelSchedulerCheck();
}

static MDS_Err_t THREAD_Init(MDS_Thread_t *thread, MDS_ThreadEntry_t entry, MDS_Arg_t *arg,
                             void *stackPool, size_t stackSize, MDS_ThreadPriority_t priority,
                             MDS_Timeout_t timeout)
{
    MDS_ASSERT(entry != NULL);
    MDS_ASSERT(stackPool != NULL);
    MDS_ASSERT(stackSize > 0);
    MDS_ASSERT(timeout.ticks > 0);

    MDS_SpinLockInit(&(thread->spinlock));
    MDS_DListInitNode(&(thread->nodeWait.node));

    thread->entry = entry;
    thread->arg = arg;
    thread->stackSize = stackSize;
    thread->stackBase = stackPool;

    thread->stackPoint = MDS_CoreThreadStackInit(thread->stackBase, thread->stackSize,
                                                 (void *)THREAD_Entry, (void *)thread,
                                                 (void *)THREAD_Exit);

    thread->initTick = timeout.ticks;
    thread->remainTick = timeout.ticks;
    thread->err = MDS_TimerInit(&(thread->timer), thread->object.name, THREAD_Timeout, NULL,
                                (MDS_Arg_t *)thread);

    thread->initPrio = priority;
    thread->currPrio = priority;

    MDS_ThreadSetState(thread, MDS_THREAD_STATE_INACTIVED);
    thread->eventOpt = MDS_EVENT_OPT_NONE;
    thread->eventMask.mask = 0U;

    MDS_HOOK_CALL(KERNEL, thread, (thread, MDS_KERNEL_TRACE_THREAD_INIT));

    return (thread->err);
}

static MDS_Err_t MDS_ThreadClose(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(thread->object)) == MDS_OBJECT_TYPE_THREAD);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(thread->spinlock));
    if (err == MDS_EOK) {
        err = THREAD_Terminate(thread);
        if (err == MDS_EOK) {
            MDS_ThreadSetState(thread, MDS_THREAD_STATE_TERMINATED | MDS_THREAD_FLAG_YIELD);
        }
    }
    MDS_SpinLockRelease(&(thread->spinlock));
    MDS_CoreInterruptRestore(lock);

    return (err);
}

MDS_Err_t MDS_ThreadInit(MDS_Thread_t *thread, const char *name, MDS_ThreadEntry_t entry,
                         MDS_Arg_t *arg, void *stackPool, size_t stackSize,
                         MDS_ThreadPriority_t priority, MDS_Timeout_t timeout)
{
    MDS_ASSERT(thread != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(thread->object), MDS_OBJECT_TYPE_THREAD, name);
    if (err == MDS_EOK) {
        err = THREAD_Init(thread, entry, arg, stackPool, stackSize, priority, timeout);
        if (err != MDS_EOK) {
            MDS_ObjectDeInit(&(thread->object));
        }
    }

    return (err);
}

MDS_Err_t MDS_ThreadDeInit(MDS_Thread_t *thread)
{
    return (MDS_ThreadClose(thread));
}

MDS_Thread_t *MDS_ThreadCreate(const char *name, MDS_ThreadEntry_t entry, MDS_Arg_t *arg,
                               size_t stackSize, MDS_ThreadPriority_t priority,
                               MDS_Timeout_t timeout)
{
    MDS_ASSERT(stackSize > 0);

    void *stackPool = MDS_SysMemAlloc(stackSize);

    if (stackPool != NULL) {
        MDS_Thread_t *thread = (MDS_Thread_t *)MDS_ObjectCreate(sizeof(MDS_Thread_t),
                                                                MDS_OBJECT_TYPE_THREAD, name);
        if (thread != NULL) {
            if (THREAD_Init(thread, entry, arg, stackPool, stackSize, priority, timeout) ==
                MDS_EOK) {
                return (thread);
            }
            MDS_ObjectDestroy(&(thread->object));
        }
        MDS_SysMemFree(stackPool);
    }

    MDS_LOG_D("[thread] entry:%p stack size:%zu priority:%d ticks:%lu create failed", entry,
              stackSize, priority.priority, (unsigned long)timeout.ticks);

    return (NULL);
}

MDS_Err_t MDS_ThreadDestroy(MDS_Thread_t *thread)
{
    return (MDS_ThreadClose(thread));
}

MDS_Err_t MDS_ThreadStartup(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(thread->object)) == MDS_OBJECT_TYPE_THREAD);

    MDS_LOG_D("[thread] thread(%p) entry:%p state:%x to startup", thread, thread->entry,
              thread->state);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(thread->spinlock));
    if (err == MDS_EOK) {
        MDS_ThreadState_t state = MDS_ThreadGetState(thread);
        if (state != MDS_THREAD_STATE_INACTIVED) {
            err = MDS_EACCES;
        } else {
            MDS_TimerStop(&(thread->timer));

            MDS_SchedulerRemoveThread(thread);

            MDS_SchedulerInsertThread(thread);
            MDS_ThreadSetState(thread, MDS_THREAD_STATE_READY);
        }
    }
    MDS_SpinLockRelease(&(thread->spinlock));
    MDS_CoreInterruptRestore(lock);

    if (MDS_KernelCurrentThread() != NULL) {
        MDS_KernelSchedulerCheck();
    }

    return (err);
}

MDS_Err_t MDS_ThreadResume(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(thread->object)) == MDS_OBJECT_TYPE_THREAD);

    MDS_LOG_D("[thread] thread(%p) entry:%p state:%x to resume", thread, thread->entry,
              thread->state);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(thread->spinlock));
    if (err == MDS_EOK) {
        MDS_ThreadState_t state = MDS_ThreadGetState(thread);
        if (state != MDS_THREAD_STATE_SUSPENDED) {
            err = MDS_EACCES;
        } else {
            MDS_TimerStop(&(thread->timer));

            MDS_SchedulerRemoveThread(thread);

            MDS_SchedulerInsertThread(thread);
            MDS_ThreadSetState(thread, MDS_THREAD_STATE_READY);
        }
    }
    MDS_SpinLockRelease(&(thread->spinlock));
    MDS_CoreInterruptRestore(lock);

    MDS_HOOK_CALL(KERNEL, thread, (thread, MDS_KERNEL_TRACE_THREAD_RESUME));

    return (err);
}

MDS_Err_t MDS_ThreadSuspend(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(thread->object)) == MDS_OBJECT_TYPE_THREAD);

    MDS_LOG_D("[thread] thread(%p) entry:%p state:%x to suspend", thread, thread->entry,
              thread->state);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(thread->spinlock));
    if (err == MDS_EOK) {
        MDS_ThreadState_t state = MDS_ThreadGetState(thread);
        if ((state != MDS_THREAD_STATE_READY) && (state != MDS_THREAD_STATE_RUNNING)) {
            err = MDS_EAGAIN;

        } else {
            MDS_TimerStop(&(thread->timer));

            MDS_SchedulerRemoveThread(thread);
            MDS_ThreadSetState(thread, MDS_THREAD_STATE_SUSPENDED);
        }
    }
    MDS_SpinLockRelease(&(thread->spinlock));
    MDS_CoreInterruptRestore(lock);

    MDS_HOOK_CALL(KERNEL, thread, (thread, MDS_KERNEL_TRACE_THREAD_SUSPEND));

    return (err);
}

MDS_Err_t MDS_ThreadSetPriority(MDS_Thread_t *thread, MDS_ThreadPriority_t priority)
{
    MDS_ASSERT(thread != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(thread->object)) == MDS_OBJECT_TYPE_THREAD);

    MDS_LOG_D("[thread] thread(%p) entry:%p state:%x chanage priority:%d->%d", thread,
              thread->entry, thread->state, thread->currPrio.priority, priority.priority);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(thread->spinlock));
    if (err == MDS_EOK) {
        MDS_ThreadState_t state = MDS_ThreadGetState(thread);
        if (state == MDS_THREAD_STATE_READY) {
            MDS_SchedulerRemoveThread(thread);
            thread->currPrio = priority;
            MDS_SchedulerInsertThread(thread);
        } else {
            thread->currPrio = priority;
        }
    }
    MDS_SpinLockRelease(&(thread->spinlock));
    MDS_CoreInterruptRestore(lock);

    return (err);
}

MDS_Err_t MDS_ThreadResetPriority(MDS_Thread_t *thread)
{
    return (MDS_ThreadSetPriority(thread, thread->initPrio));
}

MDS_ThreadState_t MDS_ThreadGetState(const MDS_Thread_t *thread)
{
    return (thread->state & MDS_THREAD_STATE_MASK);
}

MDS_Err_t MDS_ThreadDelay(MDS_Timeout_t timeout)
{
    MDS_Thread_t *thread = MDS_KernelCurrentThread();
    MDS_ASSERT(thread != NULL);
    if (thread == NULL) {
        MDS_ClockDelayRawTick(timeout);
        return (MDS_EFAULT);
    }

    MDS_Lock_t lock = MDS_CoreInterruptLock();

    if (timeout.ticks == MDS_CLOCK_TICK_NO_WAIT) {
        thread->remainTick = thread->initTick;
        MDS_ThreadSetYield(thread);
    } else {
        MDS_ThreadSuspend(thread);
        MDS_SysTimerStart(&(thread->timer), timeout, MDS_TIMEOUT_NO_WAIT);
    }

    MDS_CoreInterruptRestore(lock);

    MDS_KernelSchedulerCheck();

    if (thread->err == MDS_ETIMEOUT) {
        thread->err = MDS_EOK;
    }

    return (thread->err);
}

MDS_Err_t MDS_ThreadYield(void)
{
    MDS_Thread_t *thread = MDS_KernelCurrentThread();
    if (thread == NULL) {
        return (MDS_EFAULT);
    }

    MDS_Lock_t lock = MDS_CoreInterruptLock();

    thread->remainTick = thread->initTick;
    MDS_ThreadSetYield(thread);

    MDS_CoreInterruptRestore(lock);

    MDS_KernelSchedulerCheck();

    return (thread->err);
}

void MDS_ThreadRemainTicks(MDS_Tick_t ticks)
{
    MDS_Thread_t *thread = MDS_KernelCurrentThread();
    if (thread == NULL) {
        return;
    }

    MDS_Lock_t lock = MDS_CoreInterruptLock();

    if (thread->remainTick > ticks) {
        thread->remainTick -= ticks;
    } else {
        thread->remainTick = 0;
    }

    ticks = thread->remainTick;
    if (ticks == 0) {
        thread->remainTick = thread->initTick;
        MDS_ThreadSetYield(thread);
    }

    MDS_CoreInterruptRestore(lock);

    if (ticks == 0) {
        MDS_KernelSchedulerCheck();
    }
}
