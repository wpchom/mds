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

#if (defined(CONFIG_MDS_TIMER_INDEPENDENT) && (CONFIG_MDS_TIMER_INDEPENDENT != 0))
#ifndef CONFIG_MDS_TIMER_THREAD_PRIORITY
#define CONFIG_MDS_TIMER_THREAD_PRIORITY 0
#endif

#ifndef CONFIG_MDS_TIMER_THREAD_STACKSIZE
#define CONFIG_MDS_TIMER_THREAD_STACKSIZE 256
#endif

#ifndef CONFIG_MDS_TIMER_THREAD_TICKS
#define CONFIG_MDS_TIMER_THREAD_TICKS 16
#endif
#endif

/* Variable ---------------------------------------------------------------- */
static MDS_WorkQueue_t g_sysTimerQueue;
#if (defined(CONFIG_MDS_TIMER_INDEPENDENT) && (CONFIG_MDS_TIMER_INDEPENDENT != 0))
static MDS_WorkQueue_t g_sysWorkQueue;
static MDS_Thread_t g_sysWorkqThread;
static uint8_t g_sysWorkqStack[CONFIG_MDS_TIMER_THREAD_STACKSIZE];
#endif

/* Function ---------------------------------------------------------------- */
void MDS_SysTimerInit(void)
{
    MDS_SpinLockInit(&(g_sysTimerQueue.spinlock));
    MDS_SkipListInitNode(g_sysTimerQueue.list, ARRAY_SIZE(g_sysTimerQueue.list));

#if (defined(CONFIG_MDS_TIMER_INDEPENDENT) && (CONFIG_MDS_TIMER_INDEPENDENT != 0))
    MDS_Err_t err = MDS_WorkQueueInit(&g_sysWorkQueue, "workq", &g_sysWorkqThread,
                                      &g_sysWorkqStack, sizeof(g_sysWorkqStack),
                                      MDS_THREAD_PRIORITY(CONFIG_MDS_TIMER_THREAD_PRIORITY),
                                      MDS_TIMEOUT_TICKS(CONFIG_MDS_TIMER_THREAD_TICKS));
    if (err == MDS_EOK) {
        MDS_WorkQueueStart(&g_sysTimerQueue);
    } else {
        MDS_LOG_W("[timer] soft timer queue init failed: %d", err);
    }
#endif
}

void MDS_SysTimerCheck(void)
{
    MDS_Lock_t lock = MDS_CriticalLock(&(g_sysTimerQueue.spinlock));
    MDS_WorkQueueCheck(&g_sysTimerQueue, &lock);
    MDS_CriticalRestore(&(g_sysTimerQueue.spinlock), lock);
}

MDS_Tick_t MDS_SysTimerNextTick(void)
{
    return (MDS_WorkQueueNextTick(&g_sysTimerQueue));
}

MDS_Err_t MDS_SysTimerStart(MDS_Timer_t *timer, MDS_Timeout_t duration, MDS_Timeout_t period)
{
    return (MDS_WorkNodeSubmit(&g_sysTimerQueue, timer, duration, period));
}

MDS_Err_t MDS_TimerInit(MDS_Timer_t *timer, const char *name, MDS_TimerEntry_t entry,
                        MDS_TimerEntry_t stop, MDS_Arg_t *arg)
{
    return (MDS_WorkNodeInit(timer, name, entry, stop, arg));
}

MDS_Err_t MDS_TimerDeInit(MDS_Timer_t *timer)
{
    return (MDS_WorkNodeDeInit(timer));
}

MDS_Timer_t *MDS_TimerCreate(const char *name, MDS_TimerEntry_t entry, MDS_TimerEntry_t stop,
                             MDS_Arg_t *arg)
{
    return (MDS_WorkNodeCreate(name, entry, stop, arg));
}

MDS_Err_t MDS_TimerDestroy(MDS_Timer_t *timer)
{
    return (MDS_WorkNodeDestroy(timer));
}

MDS_Err_t MDS_TimerStart(MDS_Timer_t *timer, MDS_Timeout_t duration, MDS_Timeout_t period)
{
#if (defined(CONFIG_MDS_TIMER_INDEPENDENT) && (CONFIG_MDS_TIMER_INDEPENDENT != 0))
    return (MDS_WorkNodeSubmit(&g_sysWorkQueue, timer, duration, period));
#else
    return (MDS_WorkNodeSubmit(&g_sysTimerQueue, timer, duration, period));
#endif
}

MDS_Err_t MDS_TimerStop(MDS_Timer_t *timer)
{
    return (MDS_WorkNodeCancle(timer));
}

bool MDS_TimerIsActive(const MDS_Timer_t *timer)
{
    return (MDS_WorkNodeIsSubmit(timer));
}
