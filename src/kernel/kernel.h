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
#ifndef __MDS_KERNEL_H__
#define __MDS_KERNEL_H__

/* Include ----------------------------------------------------------------- */
#include "mds_sys.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct MDS_KernelCpuInfo {
    MDS_Thread_t *currThread;
} MDS_KernelCpuInfo_t;

/* Core -------------------------------------------------------------------- */
void *MDS_CoreThreadStackInit(void *stackBase, size_t stackSize, void *entry, void *arg,
                              void *exit);
void MDS_CoreSchedulerStartup(void *toSP);
void MDS_CoreSchedulerSwitch(void *from, void *to);
bool MDS_CoreThreadStackCheck(void *sp);

/* Kernel ------------------------------------------------------------------ */
static inline void MDS_KernelWaitQueueInit(MDS_WaitQueue_t *queueWait)
{
    MDS_DListInitNode(&(queueWait->list));
}

static inline MDS_Thread_t *MDS_KernelWaitQueuePeek(MDS_WaitQueue_t *queueWait)
{
    if (!MDS_DListIsEmpty(&(queueWait->list))) {
        return (CONTAINER_OF(queueWait->list.next, MDS_Thread_t, nodeWait.node));
    } else {
        return (NULL);
    }
}

MDS_Err_t MDS_KernelWaitQueueSuspend(MDS_WaitQueue_t *queueWait, MDS_Thread_t *thread,
                                     MDS_Timeout_t timeout, bool isPrio);
MDS_Err_t MDS_KernelWaitQueueUntil(MDS_WaitQueue_t *queueWait, MDS_Thread_t *thread,
                                   MDS_Timeout_t timeout, bool isPrio, MDS_Lock_t *lock,
                                   MDS_SpinLock_t *spinlock);
MDS_Thread_t *MDS_KernelWaitQueueResume(MDS_WaitQueue_t *queueWait);
void MDS_KernelWaitQueueDrain(MDS_WaitQueue_t *queueWait);

void MDS_KernelSchedulerCheck(void);
void MDS_KernelPushDefunct(MDS_Thread_t *thread);
MDS_Thread_t *MDS_KernelPopDefunct(void);
MDS_Thread_t *MDS_KernelIdleThread(void);
void MDS_IdleThreadInit(void);

/* Scheduler --------------------------------------------------------------- */
void MDS_SchedulerInit(void);
void MDS_SchedulerInsertThread(MDS_Thread_t *thread);
void MDS_SchedulerRemoveThread(MDS_Thread_t *thread);
MDS_Thread_t *MDS_SchedulerPeekThread(void);

/* WorkQueue --------------------------------------------------------------- */
MDS_Err_t MDS_WorkQueueCheck(MDS_WorkQueue_t *workq, MDS_Lock_t *lock);

/* Timer ------------------------------------------------------------------- */
void MDS_SysTimerInit(void);
void MDS_SysTimerCheck(void);
MDS_Tick_t MDS_SysTimerNextTick(void);
MDS_Err_t MDS_SysTimerStart(MDS_Timer_t *timer, MDS_Timeout_t duration, MDS_Timeout_t period);

/* Thread ------------------------------------------------------------------ */
void MDS_ThreadRemainTicks(MDS_Tick_t ticks);

static inline void MDS_ThreadSetState(MDS_Thread_t *thread, MDS_ThreadState_t state)
{
    if (state == MDS_THREAD_STATE_RUNNING) {
        thread->state = state;
    } else {
        thread->state = (thread->state & ~MDS_THREAD_STATE_MASK) | state;
    }
}

static inline void MDS_ThreadSetYield(MDS_Thread_t *thread)
{
    thread->state |= MDS_THREAD_FLAG_YIELD;
}

static inline bool MDS_ThreadIsYield(const MDS_Thread_t *thread)
{
    return ((thread->state & MDS_THREAD_FLAG_YIELD) == MDS_THREAD_FLAG_YIELD);
}

#ifdef __cplusplus
}
#endif

#endif /* __MDS_KERNEL_H__ */
