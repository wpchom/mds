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
static MDS_SpinLock_t g_sysSpinLock;

/* Function ---------------------------------------------------------------- */
void MDS_SpinLockInit(MDS_SpinLock_t *spinlock)
{
#if defined(CONFIG_MDS_KERNEL_SMP_CPUS) && (CONFIG_MDS_KERNEL_SMP_CPUS > 1)
    spinlock->locked = 0;
#else
    UNUSED(spinlock);
#endif
}

void MDS_SpinLockAcquire(MDS_SpinLock_t *spinlock)
{
#if defined(CONFIG_MDS_KERNEL_SMP_CPUS) && (CONFIG_MDS_KERNEL_SMP_CPUS > 1)
// record cpus
#else
    UNUSED(spinlock);
#endif
}

void MDS_SpinLockRelease(MDS_SpinLock_t *spinlock)
{
#if defined(CONFIG_MDS_KERNEL_SMP_CPUS) && (CONFIG_MDS_KERNEL_SMP_CPUS > 1)
// check curr cpu == acquire cpu
#else
    UNUSED(spinlock);
#endif
}

MDS_Lock_t MDS_CriticalLock(MDS_SpinLock_t *spinlock)
{
    MDS_Lock_t lock = MDS_CoreInterruptLock();

    MDS_SpinLockAcquire((spinlock != NULL) ? (spinlock) : (&g_sysSpinLock));

    return (lock);
}

void MDS_CriticalRestore(MDS_SpinLock_t *spinlock, MDS_Lock_t lock)
{
    MDS_SpinLockRelease((spinlock != NULL) ? (spinlock) : (&g_sysSpinLock));

    MDS_CoreInterruptRestore(lock);
}
