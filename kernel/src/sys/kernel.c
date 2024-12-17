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
MDS_HOOK_INIT(SCHEDULER_SWITCH, MDS_Thread_t *toThread, MDS_Thread_t *fromThread);

/* Variable ---------------------------------------------------------------- */
static volatile int g_sysCriticalNest = 0;
static MDS_Thread_t *g_sysCurrThread = NULL;
static MDS_ListNode_t g_sysDefunctList = {.prev = &g_sysDefunctList, .next = &g_sysDefunctList};

/* Function ---------------------------------------------------------------- */
void MDS_KernelSchedulerCheck(void)
{
    MDS_Item_t lock = MDS_CoreInterruptLock();

    do {
        if (MDS_KernelGetCritical() != 0) {
            break;
        }

        MDS_Thread_t *toThread = MDS_SchedulerHighestPriorityThread();
        if (toThread == NULL) {
            toThread = MDS_KernelIdleThread();
        }

        MDS_Thread_t *currThread = g_sysCurrThread;
        bool threadReady = false;

        if ((currThread->state & MDS_THREAD_STATE_MASK) == MDS_THREAD_STATE_RUNNING) {
            if (currThread->currPrio < toThread->currPrio) {
                toThread = currThread;
            } else if ((currThread->currPrio == toThread->currPrio) &&
                       ((currThread->state & MDS_THREAD_FLAG_YIELD) == 0)) {
                toThread = currThread;
            } else {
                threadReady = true;
            }
            currThread->state &= ~MDS_THREAD_FLAG_YIELD;
        }

        if (toThread != currThread) {
            g_sysCurrThread = toThread;
            if (threadReady) {
                MDS_SchedulerInsertThread(currThread);
            }

            MDS_SchedulerRemoveThread(toThread);
            toThread->state = (toThread->state & ~MDS_THREAD_STATE_MASK) | MDS_THREAD_STATE_RUNNING;

            MDS_SCHEDULER_DEBUG("switch to thread(%p) entry:%p sp:%p priority:%u from thread(%p) entry:%p sp:%p",
                                toThread, toThread->entry, toThread->stackPoint, toThread->currPrio, currThread,
                                currThread->entry, currThread->stackPoint);

            if (!MDS_CoreThreadStackCheck(toThread)) {
                MDS_PANIC("switch to thread(%p) entry:%p stack has broken", toThread, toThread->entry,
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

void MDS_KernelPushDefunct(MDS_Thread_t *thread)
{
    MDS_SCHEDULER_DEBUG("push defunct thread(%p) entry:%p sp:%p priority:%u", thread, thread->entry, thread->stackPoint,
                        thread->currPrio);

    MDS_ListInsertNodePrev(&g_sysDefunctList, &(thread->node));
}

MDS_Thread_t *MDS_KernelPopDefunct(void)
{
    MDS_Thread_t *thread = NULL;

    if (!MDS_ListIsEmpty(&g_sysDefunctList)) {
        thread = CONTAINER_OF(g_sysDefunctList.next, MDS_Thread_t, node);

        MDS_SCHEDULER_DEBUG("pop defunct thread(%p) entry:%p sp:%p priority:%u", thread, thread->entry,
                            thread->stackPoint, thread->currPrio);

        MDS_ListRemoveNode(&(thread->node));
    }

    return (thread);
}

void MDS_KernelRemainThread(void)
{
    MDS_Thread_t *thread = MDS_KernelCurrentThread();
    if (thread == NULL) {
        return;
    }

    if (thread->remainTick > 0) {
        thread->remainTick -= 1;
    }

    if (thread->remainTick == 0) {
        thread->remainTick = thread->initTick;
        thread->state |= MDS_THREAD_FLAG_YIELD;
        MDS_KernelSchedulerCheck();
    }
}

void MDS_KernelInit(void)
{
    MDS_SchedulerInit();

    MDS_SysTimerInit();

    MDS_IdleThreadInit();
}

void MDS_KernelStartup(void)
{
    MDS_Thread_t *toThread = MDS_SchedulerHighestPriorityThread();
    if (toThread == NULL) {
        toThread = MDS_KernelIdleThread();
    }

    g_sysCurrThread = toThread;
    toThread->state = MDS_THREAD_STATE_RUNNING;

    MDS_SCHEDULER_DEBUG("scheduler thread startup with thread(%p) entry:%p sp:%p priority:%u", toThread,
                        toThread->entry, toThread->stackPoint, toThread->currPrio);

    MDS_CoreSchedulerStartup(&(toThread->stackPoint));
}

MDS_Thread_t *MDS_KernelCurrentThread(void)
{
    return (g_sysCurrThread);
}

void MDS_KernelEnterCritical(void)
{
    MDS_Item_t lock = MDS_CoreInterruptLock();

    g_sysCriticalNest += 1;

    MDS_CoreInterruptRestore(lock);
}

void MDS_KernelExitCritical(void)
{
    MDS_Item_t lock = MDS_CoreInterruptLock();

    g_sysCriticalNest -= 1;
    if (g_sysCriticalNest > 0) {
        MDS_CoreInterruptRestore(lock);
        return;
    } else {
        g_sysCriticalNest = 0;
        MDS_CoreInterruptRestore(lock);
    }

    if (MDS_KernelCurrentThread() != NULL) {
        MDS_KernelSchedulerCheck();
    }
}

size_t MDS_KernelGetCritical(void)
{
    return (g_sysCriticalNest);
}

MDS_Tick_t MDS_KernelGetSleepTick(void)
{
    MDS_Tick_t sleepTick = 0;

    MDS_Item_t lock = MDS_CoreInterruptLock();

    if (MDS_SchedulerHighestPriorityThread() == NULL) {
        MDS_Tick_t nextTick = MDS_SysTimerNextTick();
        MDS_Tick_t currTick = MDS_ClockGetTickCount();
        sleepTick = (nextTick > currTick) ? (nextTick - currTick) : (0);
    }

    MDS_CoreInterruptRestore(lock);

    return (sleepTick);
}

void MDS_KernelCompensateTick(MDS_Tick_t tickcount)
{
    MDS_Item_t lock = MDS_CoreInterruptLock();

    MDS_Tick_t currTick = MDS_ClockGetTickCount() + tickcount;
    MDS_ClockSetTickCount(currTick);
    MDS_SysTimerCheck();

    MDS_CoreInterruptRestore(lock);
}
