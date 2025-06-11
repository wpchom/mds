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
MDS_Err_t MDS_MutexInit(MDS_Mutex_t *mutex, const char *name)
{
    MDS_ASSERT(mutex != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(mutex->object), MDS_OBJECT_TYPE_MUTEX, name);
    if (err == MDS_EOK) {
        mutex->owner = NULL;
        mutex->priority = MDS_THREAD_PRIORITY(CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX);
        mutex->value = 1;
        mutex->nest = 0;
        MDS_KernelWaitQueueInit(&(mutex->queueWait));
        MDS_SpinLockInit(&(mutex->spinlock));
    }

    return (err);
}

MDS_Err_t MDS_MutexDeInit(MDS_Mutex_t *mutex)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(mutex->spinlock));
    if (err == MDS_EOK) {
        MDS_KernelWaitQueueDrain(&(mutex->queueWait));
        err = MDS_ObjectDeInit(&(mutex)->object);
    }
    if (err != MDS_EOK) {
        MDS_SpinLockRelease(&(mutex->spinlock));
    }
    MDS_CoreInterruptRestore(lock);

    return (err);
}

MDS_Mutex_t *MDS_MutexCreate(const char *name)
{
    MDS_Mutex_t *mutex = (MDS_Mutex_t *)MDS_ObjectCreate(sizeof(MDS_Mutex_t),
                                                         MDS_OBJECT_TYPE_MUTEX, name);
    if (mutex != NULL) {
        mutex->owner = NULL;
        mutex->priority = MDS_THREAD_PRIORITY(CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX);
        mutex->value = 1;
        mutex->nest = 0;
        MDS_KernelWaitQueueInit(&(mutex->queueWait));
        MDS_SpinLockInit(&(mutex->spinlock));
    }

    return (mutex);
}

MDS_Err_t MDS_MutexDestroy(MDS_Mutex_t *mutex)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(mutex->spinlock));
    if (err == MDS_EOK) {
        MDS_KernelWaitQueueDrain(&(mutex->queueWait));
        err = MDS_ObjectDestroy(&(mutex->object));
    }
    if (err != MDS_EOK) {
        MDS_SpinLockRelease(&(mutex->spinlock));
    }

    MDS_CoreInterruptRestore(lock);

    return (err);
}

MDS_Err_t MDS_MutexAcquire(MDS_Mutex_t *mutex, MDS_Timeout_t timeout)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    MDS_HOOK_CALL(KERNEL, mutex, (mutex, MDS_KERNEL_TRACE_MUTEX_TRY_ACQUIRE, MDS_EOK, timeout));

    MDS_Thread_t *thread = MDS_KernelCurrentThread();
    if (thread == NULL) {
        MDS_LOG_W("[mutex] thread is null try to acquire mutex");
        return (MDS_EFAULT);
    }

    MDS_LOG_D("[mutex] thread(%p) entry:%p acquire mutex(%p) value:%u nest:%u onwer:%p", thread,
              thread->entry, mutex, mutex->value, mutex->nest, mutex->owner);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(mutex->spinlock));
    if (err == MDS_EOK) {
        if (thread == mutex->owner) {
            if (mutex->nest < (__typeof__(mutex->nest))(-1)) {
                mutex->nest += 1;
            } else {
                err = MDS_ERANGE;
            }
        } else if (mutex->value > 0) {
            mutex->value -= 1;
            mutex->owner = thread;
            mutex->priority = thread->currPrio;
            if (mutex->nest < (__typeof__(mutex->nest))(-1)) {
                mutex->nest += 1;
            } else {
                err = MDS_ERANGE;
            }
        } else if (timeout.ticks == MDS_CLOCK_TICK_NO_WAIT) {
            err = thread->err = MDS_ETIMEOUT;
        } else if (timeout.ticks < MDS_CLOCK_TICK_TIMER_MAX) {
            MDS_ThreadPriority_t tempPrio = mutex->owner->currPrio;
            if (thread->currPrio.priority < mutex->owner->currPrio.priority) {
                MDS_ThreadSetPriority(mutex->owner, thread->currPrio);
            }
            err = MDS_KernelWaitQueueSuspend(&(mutex->queueWait), thread, timeout, true);
            if (err != MDS_EOK) {
                MDS_ThreadSetPriority(mutex->owner, tempPrio);
            } else {
                err = MDS_ENODEF;
            }
        } else {
            err = MDS_EINVAL;
        }
    }
    MDS_SpinLockRelease(&(mutex->spinlock));
    MDS_CoreInterruptRestore(lock);

    MDS_HOOK_CALL(KERNEL, mutex, (mutex, MDS_KERNEL_TRACE_MUTEX_HAS_ACQUIRE, err, timeout));

    if (err == MDS_ENODEF) {
        MDS_KernelSchedulerCheck();
        err = thread->err;
    }

    return (err);
}

MDS_Err_t MDS_MutexRelease(MDS_Mutex_t *mutex)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    MDS_Thread_t *thread = MDS_KernelCurrentThread();
    if (thread == NULL) {
        MDS_LOG_W("[mutex] thread is null try to release mutex");
        return (MDS_EFAULT);
    }

    MDS_LOG_D("[mutex] thread(%p) entry:%p release mutex(%p) value:%u nest:%u onwer:%p", thread,
              thread->entry, mutex, mutex->value, mutex->nest, mutex->owner);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Err_t err = MDS_SpinLockAcquire(&(mutex->spinlock));
    do {
        if (err != MDS_EOK) {
            break;
        }

        if (thread != mutex->owner) {
            err = thread->err = MDS_EACCES;
            break;
        }

        MDS_HOOK_CALL(KERNEL, mutex,
                      (mutex, MDS_KERNEL_TRACE_MUTEX_HAS_RELEASE, MDS_EOK, MDS_TIMEOUT_NO_WAIT));

        mutex->nest -= 1;
        if (mutex->nest != 0) {
            break;
        }

        if (mutex->priority.priority != mutex->owner->currPrio.priority) {
            MDS_ThreadSetPriority(mutex->owner, mutex->priority);
        }

        thread = MDS_KernelWaitQueueResume(&(mutex->queueWait));
        if (thread == NULL) {
            mutex->owner = NULL;
            mutex->priority = MDS_THREAD_PRIORITY(CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX);
            if (mutex->value < (__typeof__(mutex->value))(-1)) {
                mutex->value += 1;
            }
        } else {
            err = MDS_ENODEF;
            mutex->owner = thread;
            mutex->priority = mutex->owner->currPrio;
            if (mutex->nest < (__typeof__(mutex->nest))(-1)) {
                mutex->nest += 1;
            }
        }
    } while (0);
    MDS_SpinLockRelease(&(mutex->spinlock));
    MDS_CoreInterruptRestore(lock);

    if (err == MDS_ENODEF) {
        MDS_KernelSchedulerCheck();
        err = MDS_EOK;
    }

    return (err);
}

MDS_Thread_t *MDS_MutexGetOwner(const MDS_Mutex_t *mutex)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    return (mutex->owner);
}
