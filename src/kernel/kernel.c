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
MDS_HOOK_DEFINE(KERNEL, __attribute__((weak)) MDS_HOOK_Kernel_t, ());

/* Variable ---------------------------------------------------------------- */
static struct {
    int lockNest;
    MDS_SpinLock_t spinlock;
} g_sysScheduler;

static struct {
    MDS_DListNode_t queue;
    MDS_SpinLock_t spinlock;
} g_sysDefunct = {.queue = MDS_DLIST_INIT(g_sysDefunct.queue)};

// static MDS_KernelCpuInfo_t g_sysCpuInfo[CONFIG_MDS_KERNEL_SMP_CPUS] = {0};
static MDS_Thread_t *g_sysCurrThread = NULL;

/* Function ---------------------------------------------------------------- */
void MDS_KernelSchdulerLockAcquire(void)
{
    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(g_sysScheduler.spinlock));
    if (err == MDS_EOK) {
        // atomic
        g_sysScheduler.lockNest += 1;
    }
    MDS_SpinLockRelease(&(g_sysScheduler.spinlock));
    MDS_CoreInterruptRestore(lock);
}

void MDS_KernelSchdulerLockRelease(void)
{
    int nest = 0;

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(g_sysScheduler.spinlock));
    if (err == MDS_EOK) {
        // atomic
        g_sysScheduler.lockNest -= 1;
        if (g_sysScheduler.lockNest < 0) {
            g_sysScheduler.lockNest = 0;
            MDS_LOG_E("[kernel] scheduler lock nest error, set to 0");
        }
        nest = g_sysScheduler.lockNest;
    }
    MDS_SpinLockRelease(&(g_sysScheduler.spinlock));
    MDS_CoreInterruptRestore(lock);

    if (nest == 0) {
        MDS_KernelSchedulerCheck();
    }
}

int MDS_KernelSchdulerLockLevel(void)
{
    return (g_sysScheduler.lockNest);
}

void MDS_KernelSchedulerCheck(void)
{
    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(g_sysScheduler.spinlock));

    do {
        if (err != MDS_EOK) {
            break;
        }

        MDS_Thread_t *currThread = MDS_KernelCurrentThread();
        if ((MDS_KernelSchdulerLockLevel() != 0) || (currThread == NULL)) {
            break;
        }

        MDS_Thread_t *toThread = MDS_SchedulerPeekThread();
        if (MDS_ThreadGetState(currThread) == MDS_THREAD_STATE_RUNNING) {
            if (currThread->currPrio.priority < toThread->currPrio.priority) {
                toThread = currThread;
            } else if ((currThread->currPrio.priority == toThread->currPrio.priority) &&
                       (!MDS_ThreadIsYield(currThread))) {
                toThread = currThread;
            } else {
                MDS_SchedulerInsertThread(currThread);
                MDS_ThreadSetState(currThread, MDS_THREAD_STATE_READY);
            }
        }

        MDS_SchedulerRemoveThread(toThread);
        MDS_ThreadSetState(toThread, MDS_THREAD_STATE_RUNNING);

        if (toThread != currThread) {
            MDS_LOG_D(
                "[kernel]switch to thread(%p) entry:%p sp:%p priority:%u from thread(%p) entry:%p sp:%p",
                toThread, toThread->entry, toThread->stackPoint, toThread->currPrio.priority,
                currThread, currThread->entry, currThread->stackPoint);

            if (!MDS_CoreThreadStackCheck(toThread)) {
                MDS_PANIC("switch to thread(%p) entry:%p stack:%p has broken", toThread,
                          toThread->entry, toThread->stackPoint);
            }

            MDS_HOOK_CALL(KERNEL, scheduler, (toThread, currThread));

            // cpuInfo
            g_sysCurrThread = toThread;

            MDS_CoreSchedulerSwitch(&(currThread->stackPoint), &(toThread->stackPoint));
        }
    } while (0);

    MDS_SpinLockRelease(&(g_sysScheduler.spinlock));
    MDS_CoreInterruptRestore(lock);
}

MDS_Err_t MDS_KernelWaitQueueSuspend(MDS_WaitQueue_t *queueWait, MDS_Thread_t *thread,
                                     MDS_Timeout_t timeout, bool isPrio)
{
    MDS_Err_t err = MDS_ThreadSuspend(thread);
    if (err != MDS_EOK) {
        return (err);
    }

    MDS_Thread_t *find = NULL;

    if (isPrio) {
        MDS_Thread_t *iter = NULL;
        MDS_LIST_FOREACH_NEXT (iter, nodeWait.node, &(queueWait->list)) {
            if (thread->currPrio.priority < iter->currPrio.priority) {
                find = iter;
                break;
            }
        }
    }

    if (find != NULL) {
        MDS_DListInsertNodePrev(&(find->nodeWait.node), &(thread->nodeWait.node));
    } else {
        MDS_DListInsertNodePrev(&(queueWait->list), &(thread->nodeWait.node));
    }

    err = MDS_SysTimerStart(&(thread->timer), timeout, MDS_TIMEOUT_NO_WAIT);

    return (err);
}

MDS_Err_t MDS_KernelWaitQueueUntil(MDS_WaitQueue_t *queueWait, MDS_Thread_t *thread,
                                   MDS_Timeout_t timeout, bool isPrio, MDS_Lock_t *lock,
                                   MDS_SpinLock_t *spinlock)
{
    MDS_Tick_t deltaTick = MDS_ClockGetTickCount();
    MDS_KernelWaitQueueSuspend(queueWait, thread, timeout, isPrio);

    MDS_Err_t err = MDS_SpinLockRelease(spinlock);
    MDS_CoreInterruptRestore(*lock);

    MDS_KernelSchedulerCheck();

    *lock = MDS_CoreInterruptLock();
    err = MDS_SpinLockAcquire(spinlock);

    if (err != MDS_EOK) {
        return (err);
    }

    if (thread->err != MDS_EOK) {
        err = thread->err;
        return (err);
    }

    deltaTick = MDS_ClockGetTickCount() - deltaTick;
    if (timeout.ticks <= deltaTick) {
        err = MDS_ETIMEOUT;
    }

    return (err);
}

MDS_Thread_t *MDS_KernelWaitQueueResume(MDS_WaitQueue_t *queueWait)
{
    MDS_Thread_t *thread = MDS_KernelWaitQueuePeek(queueWait);
    if (thread != NULL) {
        // no need to remove from list, thread resume will remove it to ready list
        MDS_Err_t err = MDS_ThreadResume(thread);
        if (err != MDS_EOK) {
            MDS_LOG_W("[kernel] wait queue resume thread err=%d", err);
        }
    }

    return (thread);
}

void MDS_KernelWaitQueueDrain(MDS_WaitQueue_t *queueWait)
{
    do {
        MDS_Thread_t *thread = MDS_KernelWaitQueueResume(queueWait);
        if (thread == NULL) {
            break;
        }

        thread->err = MDS_ENOENT;
    } while (0);
}

void MDS_KernelPushDefunct(MDS_Thread_t *thread)
{

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(g_sysDefunct.spinlock));
    if (err == MDS_EOK) {
        MDS_DListInsertNodePrev(&(g_sysDefunct.queue), &(thread->nodeWait.node));
    }
    MDS_SpinLockRelease(&(g_sysDefunct.spinlock));
    MDS_CoreInterruptRestore(lock);

    MDS_LOG_D("[kernel] push defunct thread(%p) entry:%p sp:%p priority:%u", thread, thread->entry,
              thread->stackPoint, thread->currPrio.priority);
}

MDS_Thread_t *MDS_KernelPopDefunct(void)
{
    MDS_Thread_t *thread = NULL;

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(g_sysDefunct.spinlock));
    if ((err == MDS_EOK) && (!MDS_DListIsEmpty(&g_sysDefunct.queue))) {
        thread = CONTAINER_OF(g_sysDefunct.queue.next, MDS_Thread_t, nodeWait.node);

        MDS_LOG_D("[kernel] pop defunct thread(%p) entry:%p sp:%p priority:%u", thread,
                  thread->entry, thread->stackPoint, thread->currPrio.priority);

        MDS_DListRemoveNode(&(thread->nodeWait.node));
    }
    MDS_SpinLockRelease(&(g_sysDefunct.spinlock));
    MDS_CoreInterruptRestore(lock);

    return (thread);
}

void MDS_KernelInit(void)
{
    MDS_SchedulerInit();

    MDS_IdleThreadInit();

    MDS_SysTimerInit();
}

void MDS_KernelStartup(void)
{
    MDS_Thread_t *toThread = MDS_SchedulerPeekThread();

    MDS_LOG_D("[kernel] scheduler thread startup with thread(%p) entry:%p sp:%p priority:%u",
              toThread, toThread->entry, toThread->stackPoint, toThread->currPrio.priority);

    MDS_ThreadSetState(toThread, MDS_THREAD_STATE_RUNNING);
    g_sysCurrThread = toThread;

    MDS_CoreSchedulerStartup(&(toThread->stackPoint));
}

MDS_Thread_t *MDS_KernelCurrentThread(void)
{
    // cpuInfo
    return (g_sysCurrThread);
}

MDS_Tick_t MDS_KernelGetSleepTick(void)
{
    MDS_Tick_t sleepTick = 0;

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(g_sysScheduler.spinlock));
    if (err == MDS_EOK) {
        // get all cpu is idle

        if (MDS_SchedulerPeekThread() == MDS_KernelIdleThread()) {
            MDS_Tick_t nextTick = MDS_SysTimerNextTick();
            MDS_Tick_t currTick = MDS_ClockGetTickCount();
            sleepTick = (nextTick > currTick) ? (nextTick - currTick) : (0);
        }
    }
    MDS_SpinLockRelease(&(g_sysScheduler.spinlock));
    MDS_CoreInterruptRestore(lock);

    return (sleepTick);
}

void MDS_KernelCompensateTick(MDS_Tick_t ticks)
{
    MDS_ClockIncTickCount(ticks);
}
