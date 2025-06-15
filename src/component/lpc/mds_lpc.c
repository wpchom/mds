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
#include "lpc/mds_lpc.h"
#include "mds_log.h"

/* Define  ----------------------------------------------------------------- */
MDS_LOG_MODULE_DEFINE(lpc, CONFIG_MDS_LPC_LOG_LEVEL);

/* Typedef ----------------------------------------------------------------- */
typedef struct MDS_LPC_Manager {
    const MDS_LPC_ManagerOps_t *ops;
    MDS_LPC_HookFunc_t hook;

    MDS_Condition_t runCond;
    MDS_Mutex_t runMutex;

    MDS_Tick_t sleepThreshold;
    MDS_LPC_Sleep_t sleepDefault : 8;
    MDS_LPC_Run_t runDefault     : 8;
    MDS_LPC_Run_t runMode        : 8;

    MDS_LPC_Vote_t sleepVote[MDS_LPC_SLEEP_NUMS];
    MDS_LPC_Vote_t runVote[MDS_LPC_RUN_NUMS];

#if (defined(CONFIG_MDS_KERNEL_STATS_ENABLE) && (CONFIG_MDS_KERNEL_STATS_ENABLE != 0))
    MDS_Tick_t lastTick;
    MDS_Tick_t runTime[MDS_LPC_RUN_NUMS];
#endif

    MDS_SpinLock_t spinlock;
} MDS_LPC_Manager_t;

/* Variable ---------------------------------------------------------------- */
static MDS_DListNode_t g_lpcDevList = {.next = &g_lpcDevList, .prev = &g_lpcDevList};
static MDS_LPC_Manager_t g_lpcMgr;

/* Function ---------------------------------------------------------------- */
static void LPC_DeviceSuspend(MDS_LPC_Sleep_t sleep)
{
    MDS_LPC_Device_t *iter = NULL;

    MDS_LIST_FOREACH_NEXT (iter, node, &g_lpcDevList) {
        if ((iter->ops != NULL) && (iter->ops->suspend != NULL)) {
            iter->ops->suspend(iter->dev, sleep);
        }
    }
}

static void LPC_DeviceResume(MDS_LPC_Run_t run)
{
    MDS_LPC_Device_t *iter = NULL;

    MDS_LIST_FOREACH_PREV (iter, node, &g_lpcDevList) {
        if ((iter->ops != NULL) && (iter->ops->resume != NULL)) {
            iter->ops->resume(iter->dev, run);
        }
    }
}

static MDS_LPC_Sleep_t LPC_SleepModeUpdate(MDS_LPC_Manager_t *mgr)
{
    MDS_LPC_Sleep_t sleep = mgr->sleepDefault;

    MDS_Lock_t lock = MDS_CriticalLock(&(mgr->spinlock));

    for (size_t idx = 0; idx < mgr->sleepDefault; idx++) {
        if (mgr->sleepVote[idx] > 0) {
            sleep = (MDS_LPC_Sleep_t)idx;
            break;
        }
    }

    MDS_CriticalRestore(&(mgr->spinlock), lock);

    return (sleep);
}

static MDS_LPC_Run_t LPC_RunModeUpdate(MDS_LPC_Manager_t *mgr)
{
    MDS_LPC_Run_t run = mgr->runDefault;

    MDS_Lock_t lock = MDS_CriticalLock(&(mgr->spinlock));

    for (size_t idx = MDS_LPC_RUN_NUMS - 1; idx > mgr->runDefault; idx--) {
        if (mgr->runVote[idx] > 0) {
            run = (MDS_LPC_Run_t)idx;
            break;
        }
    }

    MDS_CriticalRestore(&(mgr->spinlock), lock);

    return (run);
}

__attribute__((unused)) static void LPC_SleepModeSwitch(MDS_LPC_Manager_t *mgr)
{
    MDS_ASSERT(mgr->ops != NULL);
    MDS_ASSERT(mgr->ops->sleep != NULL);

    MDS_LPC_Sleep_t sleepMode = LPC_SleepModeUpdate(mgr);

    MDS_Tick_t realSleep = 0;
    MDS_Tick_t planSleep = MDS_KernelGetSleepTick();
    MDS_Tick_t tickSleep = planSleep;

    if ((sleepMode <= MDS_LPC_SLEEP_IDLE) || (tickSleep <= mgr->sleepThreshold)) {
        sleepMode = MDS_LPC_SLEEP_IDLE;
    }

    if (mgr->hook != NULL) {
        mgr->hook(MDS_LPC_EVENT_SLEEP_ENTER, sleepMode);
    }
    if (sleepMode > MDS_LPC_SLEEP_IDLE) {
        LPC_DeviceSuspend(sleepMode);
    }

    do {
        MDS_Lock_t lock = MDS_CriticalLock(&(mgr->spinlock));

        MDS_Tick_t tickDelta = mgr->ops->sleep(sleepMode, tickSleep);
        realSleep += tickDelta;
        MDS_KernelCompensateTick(tickDelta);

        MDS_CriticalRestore(&(mgr->spinlock), lock);

        if ((tickDelta <= 0) || (realSleep >= planSleep)) {
            break;
        }
        tickSleep = MDS_KernelGetSleepTick();
    } while ((tickSleep > 0) && (sleepMode == LPC_SleepModeUpdate(mgr)));

    MDS_LPC_Run_t run = LPC_RunModeUpdate(mgr);
    if (mgr->ops->run != NULL) {
        mgr->runMode = mgr->ops->run(run);
    }
    if (mgr->runMode != run) {
#if (defined(MDS_KERNEL_STATS_ENABLE) && (MDS_KERNEL_STATS_ENABLE != 0))
        MDS_Tick_t curTick = MDS_ClockGetTickCount();
        mgr->runTime[mgr->runMode] += curTick - mgr->lastTick;
        mgr->lastTick = curTick;
#endif
        MDS_LOG_E("[lpc] resume to run:%u fail but run:%u after sleep:%u", run, mgr->runMode,
                  sleepMode);
    }

    if (sleepMode > MDS_LPC_SLEEP_IDLE) {
        LPC_DeviceResume(mgr->runMode);
    }

    MDS_ConditionBroadCast(&(mgr->runCond));

    if (mgr->hook != NULL) {
        mgr->hook(MDS_LPC_EVENT_SLEEP_EXIT, sleepMode);
    }

    MDS_LOG_W("[lpc] sleepMode:%u plan:%lu real:%lu resume runMode:%d", sleepMode, planSleep,
              realSleep, mgr->runMode);
}

static void LPC_RunModeSwitch(MDS_LPC_Manager_t *mgr)
{
    MDS_LPC_Run_t run = LPC_RunModeUpdate(mgr);
    if ((run == mgr->runMode) || (run == MDS_LPC_RUN_LOCK)) {
        return;
    }

#if (defined(MDS_KERNEL_STATS_ENABLE) && (MDS_KERNEL_STATS_ENABLE != 0))
    MDS_Tick_t curTick = MDS_ClockGetTickCount();
    mgr->runTime[mgr->runMode] += curTick - mgr->lastTick;
    mgr->lastTick = curTick;
#endif

    if (mgr->hook != NULL) {
        mgr->hook(MDS_LPC_EVENT_RUN_BEFORE, mgr->runMode);
    }

    if ((mgr->ops != NULL) && (mgr->ops->run != NULL)) {
        mgr->runMode = mgr->ops->run(run);
        if (mgr->runMode != run) {
            MDS_LOG_E("[lpc] switch to run:%u fail but run:%u", run, mgr->runMode);
        }
    }

    LPC_DeviceResume(mgr->runMode);

    if (mgr->hook != NULL) {
        mgr->hook(MDS_LPC_EVENT_RUN_AFTER, mgr->runMode);
    }

    MDS_ConditionBroadCast(&(mgr->runCond));
}

/* PowerManager ------------------------------------------------------------ */
void MDS_IdleLowPowerControl(void)
{
    if (g_lpcMgr.ops == NULL) {
        MDS_CoreIdleSleep();
        return;
    }

    MDS_KernelSchdulerLockAcquire();
    LPC_SleepModeSwitch(&g_lpcMgr);
    LPC_RunModeSwitch(&g_lpcMgr);
    MDS_KernelSchdulerLockRelease();
}

MDS_LPC_Run_t MDS_LPC_Init(const MDS_LPC_ManagerOps_t *ops, MDS_Tick_t threshold,
                           MDS_LPC_Sleep_t sleep, MDS_LPC_Run_t run)
{
    g_lpcMgr.ops = ops;
    g_lpcMgr.sleepThreshold = threshold;
    g_lpcMgr.sleepDefault = sleep;
    g_lpcMgr.runDefault = run;

    MDS_ConditionInit(&(g_lpcMgr.runCond), "lpc");
    MDS_MutexInit(&(g_lpcMgr.runMutex), "lpc");

    return (g_lpcMgr.runMode);
}

void MDS_LPC_NotifyRegister(MDS_LPC_HookFunc_t hook)
{
    g_lpcMgr.hook = hook;
}

void MDS_LPC_SleepModeDefault(MDS_LPC_Sleep_t sleep, MDS_Tick_t threshold)
{
    MDS_ASSERT(sleep < MDS_LPC_SLEEP_NUMS);

    MDS_Lock_t lock = MDS_CriticalLock(&(g_lpcMgr.spinlock));

    g_lpcMgr.sleepDefault = sleep;
    g_lpcMgr.sleepThreshold = threshold;

    MDS_CriticalRestore(&(g_lpcMgr.spinlock), lock);
}

MDS_Err_t MDS_LPC_SleepModeRequest(MDS_LPC_Sleep_t sleep)
{
    MDS_ASSERT(sleep < MDS_LPC_SLEEP_NUMS);

    MDS_Err_t err = MDS_EOK;

    if (g_lpcMgr.hook != NULL) {
        g_lpcMgr.hook(MDS_LPC_EVENT_SLEEP_REQUEST, sleep);
    }

    MDS_Lock_t lock = MDS_CriticalLock(&(g_lpcMgr.spinlock));

    if (g_lpcMgr.sleepVote[sleep] < ((MDS_LPC_Vote_t)(-1))) {
        g_lpcMgr.sleepVote[sleep] += 1;
    } else {
        err = MDS_ERANGE;
        MDS_LOG_E("[lpc] request sleep(%d) vote is full", sleep);
    }

    MDS_CriticalRestore(&(g_lpcMgr.spinlock), lock);

    return (err);
}

MDS_Err_t MDS_LPC_SleepModeRelease(MDS_LPC_Sleep_t sleep)
{
    MDS_ASSERT(sleep < MDS_LPC_SLEEP_NUMS);

    MDS_Err_t err = MDS_EOK;

    if (g_lpcMgr.hook != NULL) {
        g_lpcMgr.hook(MDS_LPC_EVENT_SLEEP_RELEASE, sleep);
    }

    MDS_Lock_t lock = MDS_CriticalLock(&(g_lpcMgr.spinlock));

    if (g_lpcMgr.sleepVote[sleep] > 0) {
        g_lpcMgr.sleepVote[sleep] -= 1;
    } else {
        err = MDS_ERANGE;
        MDS_LOG_E("[lpc] request sleep(%d) vote is empty", sleep);
    }

    MDS_CriticalRestore(&(g_lpcMgr.spinlock), lock);

    return (err);
}

MDS_Err_t MDS_LPC_SleepModeForce(MDS_LPC_Sleep_t sleep, MDS_Tick_t tickSleep)
{
    MDS_KernelSchdulerLockAcquire();

    LPC_DeviceSuspend(sleep);

    MDS_Lock_t lock = MDS_CriticalLock(&(g_lpcMgr.spinlock));
    g_lpcMgr.ops->sleep(sleep, tickSleep);
    MDS_CriticalRestore(&(g_lpcMgr.spinlock), lock);

    MDS_KernelSchdulerLockRelease();

    return (err);
}

MDS_LPC_Run_t MDS_LPC_RunModeGet(void)
{
    return (g_lpcMgr.runMode);
}

MDS_Err_t MDS_LPC_RunModeRequest(MDS_LPC_Run_t run)
{
    MDS_ASSERT(run < MDS_LPC_RUN_NUMS);

    MDS_Err_t err = MDS_EOK;

    if (g_lpcMgr.hook != NULL) {
        g_lpcMgr.hook(MDS_LPC_EVENT_RUN_REQUEST, run);
    }

    MDS_Lock_t lock = MDS_CriticalLock(&(g_lpcMgr.spinlock));

    if (g_lpcMgr.runVote[run] < ((MDS_LPC_Vote_t)(-1))) {
        g_lpcMgr.runVote[run] += 1;
    } else {
        err = MDS_ERANGE;
    }
    LPC_RunModeSwitch(&g_lpcMgr);

    MDS_CriticalRestore(&(g_lpcMgr.spinlock), lock);

    return (err);
}

MDS_Err_t MDS_LPC_RunModeRelease(MDS_LPC_Run_t run)
{
    MDS_ASSERT(run < MDS_LPC_RUN_NUMS);

    MDS_Err_t err = MDS_EOK;

    if (g_lpcMgr.hook != NULL) {
        g_lpcMgr.hook(MDS_LPC_EVENT_RUN_RELEASE, run);
    }

    MDS_Lock_t lock = MDS_CriticalLock(&(g_lpcMgr.spinlock));

    if (g_lpcMgr.runVote[run] > 0) {
        g_lpcMgr.runVote[run] -= 1;
        // run mode switch in next idle
    } else {
        err = MDS_ERANGE;
    }

    MDS_CriticalRestore(&(g_lpcMgr.spinlock), lock);

    return (err);
}

MDS_Err_t MDS_LPC_RunModeWait(MDS_LPC_Run_t run, MDS_Timeout_t timeout)
{
    MDS_Tick_t startTick = MDS_ClockGetTickCount();
    MDS_Err_t err = MDS_MutexAcquire(&(g_lpcMgr.runMutex), timeout);
    if (err != MDS_EOK) {
        return (err);
    }

    while ((g_lpcMgr.runMode < run) || (g_lpcMgr.runMode == MDS_LPC_RUN_LOCK)) {
        if (timeout.ticks != MDS_CLOCK_TICK_FOREVER) {
            MDS_Tick_t elapsedTick = MDS_ClockGetTickCount() - startTick;
            timeout.ticks = (elapsedTick <= timeout.ticks) ? (timeout.ticks - elapsedTick)
                                                           : (MDS_CLOCK_TICK_NO_WAIT);
        }
        err = MDS_ConditionWait(&(g_lpcMgr.runCond), &(g_lpcMgr.runMutex), timeout);
        if (err != MDS_EOK) {
            break;
        }
    }

    MDS_MutexRelease(&(g_lpcMgr.runMutex));

    MDS_LOG_W("[lpc] wait run:%u curr:%u timeout:%lu err:%d", run, g_lpcMgr.runMode, timeout.ticks,
              err);

    return (err);
}

MDS_Err_t MDS_LPC_DeviceRegister(MDS_LPC_Device_t *device, MDS_Arg_t *dev,
                                 const MDS_LPC_DeviceOps_t *ops)
{
    MDS_ASSERT(device != NULL);

    MDS_DListInitNode(&(device->node));
    device->dev = dev;
    device->ops = ops;
    MDS_DListInsertNodeNext(&g_lpcDevList, &(device->node));

    if ((device->ops != NULL) && (device->ops->resume != NULL)) {
        device->ops->resume(device->dev, g_lpcMgr.runMode);
    }

    return (MDS_EOK);
}

MDS_Err_t MDS_LPC_DeviceUnregister(MDS_LPC_Device_t *device)
{
    MDS_ASSERT(device != NULL);

    MDS_DListRemoveNode(&(device->node));

    return (MDS_EOK);
}

#if (defined(MDS_KERNEL_STATS_ENABLE) && (MDS_KERNEL_STATS_ENABLE != 0))
void MDS_LPC_RunTimeClear(void)
{
    size_t run;

    MDS_Lock_t lock = MDS_CriticalLock(&(g_lpcMgr.spinlock));

    for (run = 0; run < MDS_LPC_RUN_NUMS; run++) {
        g_lpcMgr.runTime[run] = 0;
    }
    g_lpcMgr.lastTick = MDS_ClockGetTickCount();

    MDS_CriticalRestore(&(g_lpcMgr.spinlock), lock);
}

void MDS_LPC_RunTimeGet(MDS_Tick_t array[], size_t sz)
{
    size_t run;

    MDS_ASSERT(array != NULL);

    for (run = 0; (run < MDS_LPC_RUN_NUMS) && (run < sz); run++) {
        array[run] = g_lpcMgr.runTime[run];
    }
}
#endif
