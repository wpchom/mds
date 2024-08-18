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

/* Define ------------------------------------------------------------------ */
#if (defined(MDS_DEBUG_SCHEDULER) && (MDS_DEBUG_SCHEDULER > 0))
#define MDS_SCHEDULER_PRINT(fmt, ...) MDS_LOG_D("[SCHEDULER]" fmt, ##__VA_ARGS__)
#else
#define MDS_SCHEDULER_PRINT(fmt, ...)
#endif

/* Kernel ------------------------------------------------------------------ */
extern void MDS_KernelSchedulerCheck(void);
extern void MDS_KernelPushDefunct(MDS_Thread_t *thread);
extern MDS_Thread_t *MDS_KernelPopDefunct(void);

/* Scheduler --------------------------------------------------------------- */
extern void MDS_SchedulerInit(void);
extern MDS_Thread_t *MDS_SchedulerGetHighestPriorityThread(void);
extern void MDS_SchedulerInsertThread(MDS_Thread_t *thread);
extern void MDS_SchedulerRemoveThread(MDS_Thread_t *thread);

/* Timer ------------------------------------------------------------------- */
extern void MDS_SysTimerInit(void);
extern void MDS_SysTimerCheck(void);
extern MDS_Tick_t MDS_SysTimerNextTick(void);

/* Thread ------------------------------------------------------------------ */
extern void MDS_IdleThreadInit(void);

/* Core -------------------------------------------------------------------- */
extern void *MDS_CoreThreadStackInit(void *stackBase, size_t stackSize, void *entry, void *arg, void *exit);
extern bool MDS_CoreThreadStackCheck(MDS_Thread_t *thread);
extern void MDS_CoreSchedulerSwitch(void *fromSP, void *toSP);
extern void MDS_CoreSchedulerStartup(void *toSP);

#ifdef __cplusplus
}
#endif

#endif /* __MDS_KERNEL_H__ */
