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
#include "../kernel.h"

/* Define ------------------------------------------------------------------ */
MDS_LOG_MODULE_DECLARE(kernel, CONFIG_MDS_KERNEL_LOG_LEVEL);

/* Function ---------------------------------------------------------------- */
MDS_Err_t MDS_RwLockInit(MDS_RwLock_t *rwlock, const char *name)
{
    MDS_ASSERT(rwlock != NULL);

    MDS_Err_t err = MDS_MutexInit(&(rwlock->mutex), name);
    if (err == MDS_EOK) {
        MDS_ConditionInit(&(rwlock->condRd), name);
        MDS_ConditionInit(&(rwlock->condWr), name);
        rwlock->readers = 0;
    }

    return (err);
}

MDS_Err_t MDS_RwLockDeInit(MDS_RwLock_t *rwlock)
{
    MDS_ASSERT(rwlock != NULL);

    MDS_Err_t err = MDS_MutexAcquire(&(rwlock->mutex), MDS_TIMEOUT_NO_WAIT);
    if (err != MDS_EOK) {
        return (err);
    }

    if ((rwlock->readers != 0) || (MDS_KernelWaitQueuePeek(&(rwlock->condWr.queueWait)) != NULL) ||
        (MDS_KernelWaitQueuePeek(&(rwlock->condRd.queueWait)) != NULL)) {
        err = MDS_EBUSY;
    } else {
        err = MDS_SemaphoreAcquire(&(rwlock->condRd), MDS_TIMEOUT_NO_WAIT);
        if (err == MDS_EOK) {
            err = MDS_SemaphoreAcquire(&(rwlock->condWr), MDS_TIMEOUT_NO_WAIT);
            if (err == MDS_EOK) {
                MDS_SemaphoreRelease(&(rwlock->condRd));
                MDS_SemaphoreRelease(&(rwlock->condWr));

                MDS_SemaphoreDeInit(&(rwlock->condRd));
                MDS_SemaphoreDeInit(&(rwlock->condWr));
            } else {
                MDS_SemaphoreRelease(&(rwlock->condRd));
                err = MDS_EBUSY;
            }
        } else {
            err = MDS_EBUSY;
        }
    }
    MDS_MutexRelease(&(rwlock->mutex));

    if (err == MDS_EOK) {
        MDS_MutexDeInit(&(rwlock->mutex));
    }

    return (err);
}

MDS_Err_t MDS_RwLockAcquireRead(MDS_RwLock_t *rwlock, MDS_Timeout_t timeout)
{
    MDS_ASSERT(rwlock != NULL);

    MDS_Tick_t startTick = MDS_ClockGetTickCount();
    MDS_Err_t err = MDS_MutexAcquire(&(rwlock->mutex), timeout);
    if (err != MDS_EOK) {
        return (err);
    }

    while ((rwlock->readers < 0) ||
           (MDS_KernelWaitQueuePeek(&(rwlock->condWr.queueWait)) != NULL)) {
        if (timeout.ticks != MDS_CLOCK_TICK_FOREVER) {
            MDS_Tick_t elapsedTick = MDS_ClockGetTickCount() - startTick;
            timeout.ticks = (elapsedTick <= timeout.ticks) ? (timeout.ticks - elapsedTick)
                                                           : (MDS_CLOCK_TICK_NO_WAIT);
        }
        err = MDS_ConditionWait(&(rwlock->condRd), &(rwlock->mutex), timeout);
        if (err != MDS_EOK) {
            break;
        }
    }

    if (err == MDS_EOK) {
        rwlock->readers += 1;
    }

    MDS_MutexRelease(&(rwlock->mutex));

    return (err);
}

MDS_Err_t MDS_RwLockAcquireWrite(MDS_RwLock_t *rwlock, MDS_Timeout_t timeout)
{
    MDS_ASSERT(rwlock != NULL);

    MDS_Tick_t startTick = MDS_ClockGetTickCount();
    MDS_Err_t err = MDS_MutexAcquire(&(rwlock->mutex), timeout);
    if (err != MDS_EOK) {
        return (err);
    }

    while (rwlock->readers != 0) {
        if (timeout.ticks != MDS_CLOCK_TICK_FOREVER) {
            MDS_Tick_t elapsedTick = MDS_ClockGetTickCount() - startTick;
            timeout.ticks = (elapsedTick <= timeout.ticks) ? (timeout.ticks - elapsedTick)
                                                           : (MDS_CLOCK_TICK_NO_WAIT);
        }
        err = MDS_ConditionWait(&(rwlock->condWr), &(rwlock->mutex), timeout);
        if (err != MDS_EOK) {
            break;
        }
    }

    if (err == MDS_EOK) {
        rwlock->readers -= 1;
    }

    MDS_MutexRelease(&(rwlock->mutex));

    return (err);
}

MDS_Err_t MDS_RwLockRelease(MDS_RwLock_t *rwlock)
{
    MDS_ASSERT(rwlock != NULL);

    MDS_Err_t err = MDS_MutexAcquire(&(rwlock->mutex), MDS_TIMEOUT_FOREVER);
    if (err != MDS_EOK) {
        return (err);
    }

    if (rwlock->readers > 0) {
        rwlock->readers -= 1;
    } else if (rwlock->readers == -1) {
        rwlock->readers = 0;
    }

    if (MDS_KernelWaitQueuePeek(&(rwlock->condWr.queueWait)) != NULL) {
        if (rwlock->readers == 0) {
            err = MDS_ConditionSignal(&(rwlock->condWr));
        }
    } else if (MDS_KernelWaitQueuePeek(&(rwlock->condRd.queueWait)) != NULL) {
        err = MDS_ConditionBroadCast(&(rwlock->condRd));
    }

    MDS_MutexRelease(&(rwlock->mutex));

    return (err);
}
