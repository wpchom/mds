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
MDS_Err_t MDS_SemaphoreInit(MDS_Semaphore_t *semaphore, const char *name, size_t init, size_t max)
{
    MDS_ASSERT(semaphore != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(semaphore->object), MDS_OBJECT_TYPE_SEMAPHORE, name);
    if (err == MDS_EOK) {
        semaphore->value = init;
        semaphore->max = max;
        MDS_KernelWaitQueueInit(&(semaphore->queueWait));
        MDS_SpinLockInit(&(semaphore->spinlock));
    }

    return (err);
}

MDS_Err_t MDS_SemaphoreDeInit(MDS_Semaphore_t *semaphore)
{
    MDS_ASSERT(semaphore != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(semaphore->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_Lock_t lock = MDS_CriticalLock(&(semaphore->spinlock));

    MDS_KernelWaitQueueDrain(&(semaphore->queueWait));
    MDS_Err_t err = MDS_ObjectDeInit(&(semaphore->object));

    MDS_CriticalRestore((err != MDS_EOK) ? (&(semaphore->spinlock)) : (NULL), lock);

    return (err);
}

MDS_Semaphore_t *MDS_SemaphoreCreate(const char *name, size_t init, size_t max)
{
    MDS_Semaphore_t *semaphore = (MDS_Semaphore_t *)MDS_ObjectCreate(sizeof(MDS_Semaphore_t),
                                                                     MDS_OBJECT_TYPE_SEMAPHORE,
                                                                     name);
    if (semaphore != NULL) {
        semaphore->value = init;
        semaphore->max = max;
        MDS_KernelWaitQueueInit(&(semaphore->queueWait));
        MDS_SpinLockInit(&(semaphore->spinlock));
    }

    return (semaphore);
}

MDS_Err_t MDS_SemaphoreDestroy(MDS_Semaphore_t *semaphore)
{
    MDS_ASSERT(semaphore != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(semaphore->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_Lock_t lock = MDS_CriticalLock(&(semaphore->spinlock));

    MDS_KernelWaitQueueDrain(&(semaphore->queueWait));
    MDS_Err_t err = MDS_ObjectDestroy(&(semaphore->object));

    MDS_CriticalRestore((err != MDS_EOK) ? (&(semaphore->spinlock)) : (NULL), lock);

    return (err);
}

MDS_Err_t MDS_SemaphoreAcquire(MDS_Semaphore_t *semaphore, MDS_Timeout_t timeout)
{
    MDS_ASSERT(semaphore != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(semaphore->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_Err_t err = MDS_EOK;
    bool reSchedule = false;
    MDS_Thread_t *thread = MDS_KernelCurrentThread();

    MDS_LOG_D("[semaphore] thread(%p) acquire semephore(%p) err:%d, value:%zu", thread, semaphore,
              err, semaphore->value);

    MDS_HOOK_CALL(KERNEL, semaphore,
                  (semaphore, MDS_KERNEL_TRACE_SEMAPHORE_TRY_ACQUIRE, err, timeout));

    MDS_Lock_t lock = MDS_CriticalLock(&(semaphore->spinlock));

    if (semaphore->value > 0) {
        semaphore->value -= 1;
    } else if (timeout.ticks == MDS_CLOCK_TICK_NO_WAIT) {
        err = thread->err = MDS_ETIMEOUT;
    } else if ((timeout.ticks < MDS_CLOCK_TICK_TIMER_MAX) && (thread != NULL)) {
        err = MDS_KernelWaitQueueSuspend(&(semaphore->queueWait), thread, timeout, true);
        if (err == MDS_EOK) {
            reSchedule = true;
        }
    } else {
        err = MDS_EINVAL;
    }

    MDS_CriticalRestore(&(semaphore->spinlock), lock);

    MDS_HOOK_CALL(KERNEL, semaphore,
                  (semaphore, MDS_KERNEL_TRACE_SEMAPHORE_HAS_ACQUIRE, MDS_EOK, timeout));

    if (reSchedule) {
        MDS_KernelSchedulerCheck();
        err = thread->err;
    }

    return (err);
}

MDS_Err_t MDS_SemaphoreRelease(MDS_Semaphore_t *semaphore)
{
    MDS_ASSERT(semaphore != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(semaphore->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_Err_t err = MDS_EOK;
    bool reSchedule = false;

    MDS_LOG_D("[semephore] release semephore(%p) value:%zu", semaphore, semaphore->value);

    MDS_Lock_t lock = MDS_CriticalLock(&(semaphore->spinlock));

    MDS_Thread_t *thread = MDS_KernelWaitQueueResume(&(semaphore->queueWait));
    if (thread == NULL) {
        if (semaphore->value < semaphore->max) {
            semaphore->value += 1;
        } else {
            err = MDS_ERANGE;
        }
    } else {
        reSchedule = true;
    }

    MDS_CriticalRestore(&(semaphore->spinlock), lock);

    MDS_HOOK_CALL(KERNEL, semaphore,
                  (semaphore, MDS_KERNEL_TRACE_SEMAPHORE_HAS_RELEASE, err, MDS_TIMEOUT_NO_WAIT));

    if (reSchedule) {
        MDS_KernelSchedulerCheck();
    }

    return (err);
}

size_t MDS_SemaphoreGetValue(const MDS_Semaphore_t *semaphore, size_t *max)
{
    MDS_ASSERT(semaphore != NULL);

    if (max != NULL) {
        *max = semaphore->max;
    }

    return (semaphore->value);
}
