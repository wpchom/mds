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

/* Variable ---------------------------------------------------------------- */
static struct MDS_SysTick {
    volatile MDS_Tick_t tickcount;
    MDS_SpinLock_t spinlock;
} g_sysTick;

static struct MDS_UnixTime {
    int64_t ts;
    int8_t tz;
} g_unixTime;

/* Function ---------------------------------------------------------------- */
void MDS_SysTickHandler(void)
{
    MDS_ClockIncTickCount(1);
}

MDS_Tick_t MDS_ClockGetTickCount(void)
{
    MDS_Lock_t lock = MDS_CriticalLock(&(g_sysTick.spinlock));

    // TODO: atomic_load();
    MDS_Tick_t tickcnt = g_sysTick.tickcount;

    MDS_CriticalRestore(&(g_sysTick.spinlock), lock);

    return (tickcnt);
}

void MDS_ClockIncTickCount(MDS_Tick_t ticks)
{
    // TODO: atomic_add all cpu?

    MDS_Lock_t lock = MDS_CriticalLock(&(g_sysTick.spinlock));

    g_sysTick.tickcount += ticks;

    MDS_CriticalRestore(&(g_sysTick.spinlock), lock);

    MDS_ThreadRemainTicks(ticks);

    MDS_SysTimerCheck();
}

__attribute__((weak)) MDS_TimeStamp_t MDS_ClockGetTimestamp(int8_t *tz)
{
    MDS_Lock_t lock = MDS_CriticalLock(NULL);

    MDS_Tick_t ticks = MDS_ClockGetTickCount();
    MDS_TimeStamp_t ts = {.ts = g_unixTime.ts + MDS_CLOCK_TICK_TO_MS(ticks)};
    int8_t _tz = g_unixTime.tz;

    MDS_CriticalRestore(NULL, lock);

    if (tz != NULL) {
        *tz = _tz;
    }
    return (ts);
}

__attribute__((weak)) void MDS_ClockSetTimestamp(MDS_TimeStamp_t ts, int8_t tz)
{
    MDS_Lock_t lock = MDS_CriticalLock(NULL);

    MDS_Tick_t ticks = MDS_ClockGetTickCount();
    g_unixTime.ts = ts.ts - MDS_CLOCK_TICK_TO_MS(ticks);
    g_unixTime.tz = tz;

    MDS_CriticalRestore(NULL, lock);
}
