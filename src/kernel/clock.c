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

/* Function ---------------------------------------------------------------- */
void MDS_SysTickHandler(void)
{
    MDS_ClockIncTickCount(1);
}

MDS_Tick_t MDS_ClockGetTickCount(void)
{
    // atomic_load();
    return (g_sysTick.tickcount);
}

void MDS_ClockIncTickCount(MDS_Tick_t ticks)
{
    // atomic_add all cpu?

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(g_sysTick.spinlock));
    if (err == MDS_EOK) {
        // atomic_add
        g_sysTick.tickcount += ticks;
    }
    MDS_SpinLockRelease(&(g_sysTick.spinlock));
    MDS_CoreInterruptRestore(lock);

    MDS_ThreadRemainTicks(ticks);

    MDS_SysTimerCheck();
}
