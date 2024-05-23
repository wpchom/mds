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
#ifndef __MDS_LPC_H__
#define __MDS_LPC_H__

/* Include ----------------------------------------------------------------- */
#include "mds_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Define ------------------------------------------------------------------ */
#ifdef MDS_LPC_VOTE_TYPE
typedef MDS_LPC_VOTE_TYPE MDS_LPC_Vote_t;
#else
typedef uint8_t MDS_LPC_Vote_t;
#endif

#ifndef MDS_LPC_LIST_OF_SLEEP
#define MDS_LPC_LIST_OF_SLEEP                                                                                          \
    MDS_LPC_SLEEP(LIGHT)                                                                                               \
    MDS_LPC_SLEEP(DEEP)                                                                                                \
    MDS_LPC_SLEEP(RESET)                                                                                               \
    MDS_LPC_SLEEP(SHUTDOWN)
#endif

#ifndef MDS_LPC_LIST_OF_RUN
#define MDS_LPC_LIST_OF_RUN                                                                                            \
    MDS_LPC_RUN(LOW)                                                                                                   \
    MDS_LPC_RUN(NORMAL)                                                                                                \
    MDS_LPC_RUN(HIGH)
#endif

/* Typedef ----------------------------------------------------------------- */
typedef enum MDS_LPC_Sleep {
    MDS_LPC_SLEEP_IDLE,
#define MDS_LPC_SLEEP(sleep) MDS_LPC_SLEEP_##sleep,
    MDS_LPC_LIST_OF_SLEEP
#undef MDS_LPC_SLEEP

    MDS_LPC_SLEEP_NUMS,
} MDS_LPC_Sleep_t;

typedef enum MDS_LPC_Run {
#define MDS_LPC_RUN(run) MDS_LPC_RUN_##run,
    MDS_LPC_LIST_OF_RUN
#undef MDS_LPC_RUN
    MDS_LPC_RUN_LOCK,

    MDS_LPC_RUN_NUMS,
} MDS_LPC_Run_t;

typedef struct MDS_LPC_DeviceOps {
    void (*suspend)(MDS_Arg_t *dev, MDS_LPC_Sleep_t sleep);
    void (*resume)(MDS_Arg_t *dev, MDS_LPC_Run_t run);
} MDS_LPC_DeviceOps_t;

typedef struct MDS_LPC_Device {
    MDS_ListNode_t node;
    MDS_Arg_t *dev;
    const MDS_LPC_DeviceOps_t *ops;
} MDS_LPC_Device_t;

typedef struct MDS_LPC_ManagerOps {
    MDS_Tick_t (*sleep)(MDS_LPC_Sleep_t sleep, MDS_Tick_t ticksleep);
    MDS_LPC_Run_t (*run)(MDS_LPC_Run_t run);
} MDS_LPC_ManagerOps_t;

typedef enum MDS_LPC_HookEvent {
    MDS_LPC_EVENT_SLEEP_ENTER,
    MDS_LPC_EVENT_SLEEP_EXIT,
    MDS_LPC_EVENT_SLEEP_REQUEST,
    MDS_LPC_EVENT_SLEEP_RELEASE,
    MDS_LPC_EVENT_RUN_BEFORE,
    MDS_LPC_EVENT_RUN_AFTER,
    MDS_LPC_EVENT_RUN_REQUEST,
    MDS_LPC_EVENT_RUN_RELEASE,
} MDS_LPC_HookEvent_t;

typedef void (*MDS_LPC_HookFunc_t)(MDS_LPC_HookEvent_t event, MDS_Item_t mode);

/* Function ---------------------------------------------------------------- */
extern MDS_LPC_Run_t MDS_LPC_Init(const MDS_LPC_ManagerOps_t *ops, MDS_Tick_t threshold, MDS_LPC_Sleep_t sleep,
                                  MDS_LPC_Run_t run);
extern void MDS_LPC_HookRegister(MDS_LPC_HookFunc_t notify);

extern void MDS_LPC_SleepModeDefault(MDS_LPC_Sleep_t sleep, MDS_Tick_t threshold);
extern void MDS_LPC_SleepModeRequest(MDS_LPC_Sleep_t sleep);
extern void MDS_LPC_SleepModeRelease(MDS_LPC_Sleep_t sleep);
extern void MDS_LPC_SleepModeForce(MDS_LPC_Sleep_t sleep, MDS_Tick_t tickSleep);

extern MDS_LPC_Run_t MDS_LPC_RunModeGet(void);
extern void MDS_LPC_RunModeRequest(MDS_LPC_Run_t run);
extern void MDS_LPC_RunModeRelease(MDS_LPC_Run_t run);
extern MDS_Err_t MDS_LPC_RunModeWait(MDS_LPC_Run_t run, MDS_Tick_t timeout);

extern MDS_Err_t MDS_LPC_DeviceRegister(MDS_LPC_Device_t *device, MDS_Arg_t *dev, const MDS_LPC_DeviceOps_t *ops);
extern MDS_Err_t MDS_LPC_DeviceUnregister(MDS_LPC_Device_t *device);

extern void MDS_LPC_StatisticClear(void);
extern void MDS_LPC_StatisticGet(MDS_Tick_t array[], size_t sz);

#ifdef __cplusplus
}
#endif

#endif /* __MDS_LPC_H__ */
