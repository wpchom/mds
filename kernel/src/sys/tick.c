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
static volatile MDS_Tick_t g_sysTickCount = 0U;

/* Function ---------------------------------------------------------------- */
MDS_Tick_t MDS_SysTickGetCount(void)
{
    return (g_sysTickCount);
}

void MDS_SysTickSetCount(MDS_Tick_t tickcnt)
{
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    g_sysTickCount = tickcnt;

    MDS_CoreInterruptRestore(lock);
}

void MDS_SysTickIncCount(void)
{
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    g_sysTickCount += 1U;

    MDS_SchedulerRemainThread();

    MDS_CoreInterruptRestore(lock);

    MDS_SysTimerCheck();
}
