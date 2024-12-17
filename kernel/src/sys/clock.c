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
#ifndef MDS_SYSCLOCK_TIMEZONE_DEFAULT
#define MDS_SYSCLOCK_TIMEZONE_DEFAULT (+8)
#endif

#ifndef MDS_SYSCLOCK_TIMESTAMP_DEFAULT
#define MDS_SYSCLOCK_TIMESTAMP_DEFAULT 1577836800000LL
#endif

/* Variable ---------------------------------------------------------------- */
static volatile MDS_Tick_t g_sysClockTickCount = 0U;

static int8_t g_unixTimeZone = MDS_SYSCLOCK_TIMEZONE_DEFAULT;
static MDS_Time_t g_unixTimeBase = MDS_SYSCLOCK_TIMESTAMP_DEFAULT;

/* Function ---------------------------------------------------------------- */
MDS_Tick_t MDS_ClockGetTickCount(void)
{
    return (g_sysClockTickCount);
}

void MDS_ClockSetTickCount(MDS_Tick_t tickcnt)
{
    MDS_Item_t lock = MDS_CoreInterruptLock();

    g_sysClockTickCount = tickcnt;

    MDS_CoreInterruptRestore(lock);
}

void MDS_ClockIncTickCount(void)
{
    MDS_Item_t lock = MDS_CoreInterruptLock();

    g_sysClockTickCount += 1U;

    MDS_KernelRemainThread();

    MDS_CoreInterruptRestore(lock);

    MDS_SysTimerCheck();
}

__attribute__((weak)) MDS_Time_t MDS_ClockGetTimestamp(int8_t *tz)
{
    MDS_Tick_t ticks = MDS_ClockGetTickCount();
    MDS_Time_t ts = g_unixTimeBase + MDS_ClockTickToMs(ticks);

    if (tz != NULL) {
        *tz = g_unixTimeZone;
    }

    return (ts);
}

__attribute__((weak)) void MDS_ClockSetTimestamp(MDS_Time_t ts, int8_t tz)
{
    MDS_Tick_t ticks = MDS_ClockGetTickCount();

    g_unixTimeBase = ts - MDS_ClockTickToMs(ticks);
    g_unixTimeZone = tz;
}

