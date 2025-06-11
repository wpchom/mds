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
MDS_Err_t MDS_ConditionInit(MDS_Condition_t *condition, const char *name)
{
    MDS_ASSERT(condition != NULL);

    return (MDS_SemaphoreInit(condition, name, 0, -1));
}

MDS_Err_t MDS_ConditionDeInit(MDS_Condition_t *condition)
{
    MDS_ASSERT(condition != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(condition->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(condition->spinlock));
    if (err == MDS_EOK) {
        if (MDS_KernelWaitQueuePeek(&(condition->queueWait)) != NULL) {
            err = MDS_EBUSY;
        } else {
            while (MDS_SemaphoreAcquire(condition, MDS_TIMEOUT_NO_WAIT) == MDS_EBUSY) {
                MDS_ConditionBroadCast(condition);
            }

            MDS_KernelWaitQueueDrain(&(condition->queueWait));
            err = MDS_ObjectDeInit(&(condition->object));
        }
    }
    if (err != MDS_EOK) {
        MDS_SpinLockRelease(&(condition->spinlock));
    }
    MDS_CoreInterruptRestore(lock);

    return (err);
}

MDS_Condition_t *MDS_ConditionCreate(const char *name)
{
    return (MDS_SemaphoreCreate(name, 0, -1));
}

MDS_Err_t MDS_ConditionDestroy(MDS_Condition_t *condition)
{
    MDS_ASSERT(condition != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(condition->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(condition->spinlock));
    if (err == MDS_EOK) {
        if (MDS_KernelWaitQueuePeek(&(condition->queueWait)) != NULL) {
            err = MDS_EBUSY;
        } else {
            while (MDS_SemaphoreAcquire(condition, MDS_TIMEOUT_NO_WAIT) == MDS_EBUSY) {
                MDS_ConditionBroadCast(condition);
            }

            MDS_KernelWaitQueueDrain(&(condition->queueWait));
            err = MDS_ObjectDestroy(&(condition->object));
        }
    }
    if (err != MDS_EOK) {
        MDS_SpinLockRelease(&(condition->spinlock));
    }
    MDS_CoreInterruptRestore(lock);

    return (err);
}

MDS_Err_t MDS_ConditionBroadCast(MDS_Condition_t *condition)
{
    MDS_ASSERT(condition != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(condition->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_Err_t err;
    do {
        err = MDS_SemaphoreAcquire(condition, MDS_TIMEOUT_NO_WAIT);
        if ((err == MDS_ETIMEOUT) || (err == MDS_EOK)) {
            MDS_SemaphoreRelease(condition);
        }
    } while (err == MDS_ETIMEOUT);

    return (err);
}

MDS_Err_t MDS_ConditionSignal(MDS_Condition_t *condition)
{
    MDS_ASSERT(condition != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(condition->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_Thread_t *thread = NULL;

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(condition->spinlock));
    if (err == MDS_EOK) {
        thread = MDS_KernelWaitQueuePeek(&(condition->queueWait));
    }
    MDS_SpinLockRelease(&(condition->spinlock));
    MDS_CoreInterruptRestore(lock);

    if (thread != NULL) {
        err = MDS_SemaphoreRelease(condition);
    }

    return (err);
}

MDS_Err_t MDS_ConditionWait(MDS_Condition_t *condition, MDS_Mutex_t *mutex, MDS_Timeout_t timeout)
{
    MDS_ASSERT(condition != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(condition->object)) == MDS_OBJECT_TYPE_SEMAPHORE);
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    MDS_Thread_t *thread = MDS_KernelCurrentThread();
    if ((thread == NULL) || (thread != mutex->owner)) {
        return (MDS_EACCES);
    }

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(condition->spinlock));
    if (err == MDS_EOK) {
        if (condition->value > 0) {
            condition->value -= 1;
        } else if (timeout.ticks == MDS_CLOCK_TICK_NO_WAIT) {
            err = thread->err = MDS_ETIMEOUT;
        } else if ((timeout.ticks < MDS_CLOCK_TICK_TIMER_MAX) && (thread != NULL)) {
            err = MDS_KernelWaitQueueSuspend(&(condition->queueWait), thread, timeout, false);
            MDS_MutexRelease(mutex);

            MDS_SpinLockRelease(&(condition->spinlock));
            MDS_CoreInterruptRestore(lock);

            if (err == MDS_EOK) {
                MDS_KernelSchedulerCheck();
                err = thread->err;
            }
            MDS_MutexAcquire(mutex, MDS_TIMEOUT_FOREVER);

            return (err);
        } else {
            err = MDS_EINVAL;
        }
    }
    MDS_SpinLockRelease(&(condition->spinlock));
    MDS_CoreInterruptRestore(lock);

    return (err);
}
