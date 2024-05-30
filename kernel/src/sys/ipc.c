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

/* Hook -------------------------------------------------------------------- */
MDS_HOOK_INIT(SEMAPHORE_TRY_ACQUIRE, MDS_Semaphore_t *semaphore, MDS_Tick_t timeout);
MDS_HOOK_INIT(SEMAPHORE_HAS_ACQUIRE, MDS_Semaphore_t *semaphore, MDS_Err_t err);
MDS_HOOK_INIT(SEMAPHORE_HAS_RELEASE, MDS_Semaphore_t *semaphore);
MDS_HOOK_INIT(MUTEX_TRY_ACQUIRE, MDS_Mutex_t *mutex, MDS_Tick_t timeout);
MDS_HOOK_INIT(MUTEX_HAS_ACQUIRE, MDS_Mutex_t *mutex, MDS_Err_t err);
MDS_HOOK_INIT(MUTEX_HAS_RELEASE, MDS_Mutex_t *mutex);
MDS_HOOK_INIT(EVENT_TRY_ACQUIRE, MDS_Event_t *event, MDS_Tick_t timeout);
MDS_HOOK_INIT(EVENT_HAS_ACQUIRE, MDS_Event_t *event, MDS_Err_t err);
MDS_HOOK_INIT(EVENT_HAS_SET, MDS_Event_t *event, MDS_Mask_t mask);
MDS_HOOK_INIT(EVENT_HAS_CLR, MDS_Event_t *event, MDS_Mask_t mask);
MDS_HOOK_INIT(MSGQUEUE_TRY_RECV, MDS_MsgQueue_t *msgQueue, MDS_Tick_t timeout);
MDS_HOOK_INIT(MSGQUEUE_HAS_RECV, MDS_MsgQueue_t *msgQueue, MDS_Err_t err);
MDS_HOOK_INIT(MSGQUEUE_TRY_SEND, MDS_MsgQueue_t *msgQueue, MDS_Tick_t timeout);
MDS_HOOK_INIT(MSGQUEUE_HAS_SEND, MDS_MsgQueue_t *msgQueue, MDS_Err_t err);
MDS_HOOK_INIT(MEMPOOL_TRY_ALLOC, MDS_MemPool_t *memPool, MDS_Tick_t timeout);
MDS_HOOK_INIT(MEMPOOL_HAS_ALLOC, MDS_MemPool_t *memPool, void *ptr);
MDS_HOOK_INIT(MEMPOOL_HAS_FREE, MDS_MemPool_t *memPool, void *ptr);

/* Define ------------------------------------------------------------------ */
#if (defined(MDS_DEBUG_IPC) && (MDS_DEBUG_IPC > 0))
#define MDS_IPC_PRINT(fmt, ...) MDS_LOG_D("[IPC]" fmt, ##__VA_ARGS__)
#else
#define MDS_IPC_PRINT(fmt, ...)
#endif

/* IPC thread -------------------------------------------------------------- */
static void IPC_ListSuspendThread(MDS_ListNode_t *list, MDS_Thread_t *thread, bool isPrio, MDS_Tick_t timeout)
{
    register MDS_Thread_t *iter = NULL;

    MDS_ThreadSuspend(thread);

    if (isPrio) {
        MDS_LIST_FOREACH_NEXT (iter, node, list) {
            if (thread->currPrio < iter->currPrio) {
                MDS_ListInsertNodePrev(&(iter->node), &(thread->node));
                break;
            }
        }
    }

    if ((iter == NULL) || (&(iter->node) == list)) {
        MDS_ListInsertNodePrev(list, &(thread->node));
    }

    if (timeout < MDS_TIMER_TICK_MAX) {
        MDS_TimerStart(&(thread->timer), timeout);
    }
}

static MDS_Err_t IPC_ListSuspendWait(MDS_Item_t *lock, MDS_ListNode_t *list, MDS_Thread_t *thread, MDS_Tick_t timeout)
{
    MDS_Tick_t deltaTick = MDS_SysTickGetCount();

    IPC_ListSuspendThread(list, thread, true, timeout);
    MDS_CoreInterruptRestore(*lock);
    MDS_SchedulerCheck();
    if (thread->err != MDS_EOK) {
        return (thread->err);
    }

    *lock = MDS_CoreInterruptLock();
    deltaTick = MDS_SysTickGetCount() - deltaTick;
    if (timeout > deltaTick) {
        timeout -= deltaTick;

        return (MDS_EOK);
    } else {
        return (MDS_ETIME);
    }
}

static void IPC_ListResumeThread(MDS_ListNode_t *list)
{
    MDS_Thread_t *thread = CONTAINER_OF(list->next, MDS_Thread_t, node);

    MDS_ThreadResume(thread);
}

static void IPC_ListResumeAllThread(MDS_ListNode_t *list)
{
    MDS_Thread_t *thread = NULL;

    while (!MDS_ListIsEmpty(list)) {
        register MDS_Item_t lock = MDS_CoreInterruptLock();

        thread = CONTAINER_OF(list->next, MDS_Thread_t, node);
        thread->err = MDS_EAGAIN;

        MDS_ThreadResume(thread);

        MDS_CoreInterruptRestore(lock);
    }
}

/* Semaphore --------------------------------------------------------------- */
MDS_Err_t MDS_SemaphoreInit(MDS_Semaphore_t *semaphore, const char *name, size_t init, size_t max)
{
    MDS_ASSERT(semaphore != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(semaphore->object), MDS_OBJECT_TYPE_SEMAPHORE, name);
    if (err == MDS_EOK) {
        semaphore->value = init;
        semaphore->max = max;
        MDS_ListInitNode(&(semaphore->list));
    }

    return (err);
}

MDS_Err_t MDS_SemaphoreDeInit(MDS_Semaphore_t *semaphore)
{
    MDS_ASSERT(semaphore != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(semaphore->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    IPC_ListResumeAllThread(&(semaphore->list));

    return (MDS_ObjectDeInit(&(semaphore->object)));
}

MDS_Semaphore_t *MDS_SemaphoreCreate(const char *name, size_t init, size_t max)
{
    MDS_Semaphore_t *semaphore = (MDS_Semaphore_t *)MDS_ObjectCreate(sizeof(MDS_Semaphore_t), MDS_OBJECT_TYPE_SEMAPHORE,
                                                                     name);
    if (semaphore != NULL) {
        semaphore->value = init;
        semaphore->max = max;
        MDS_ListInitNode(&(semaphore->list));
    }

    return (semaphore);
}

MDS_Err_t MDS_SemaphoreDestroy(MDS_Semaphore_t *semaphore)
{
    MDS_ASSERT(semaphore != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(semaphore->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    IPC_ListResumeAllThread(&(semaphore->list));

    return (MDS_ObjectDestory(&(semaphore->object)));
}

MDS_Err_t MDS_SemaphoreAcquire(MDS_Semaphore_t *semaphore, MDS_Tick_t timeout)
{
    MDS_ASSERT(semaphore != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(semaphore->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_Thread_t *thread = MDS_KernelCurrentThread();

    MDS_HOOK_CALL(SEMAPHORE_TRY_ACQUIRE, semaphore, timeout);

    MDS_IPC_PRINT("thread(%p) acquire semephore(%p), value:%u", thread, semaphore, semaphore->value);

    MDS_Err_t err = MDS_EOK;
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    if (semaphore->value > 0) {
        semaphore->value -= 1;
        MDS_CoreInterruptRestore(lock);
    } else if (timeout == 0) {
        thread->err = MDS_ETIME;
        MDS_CoreInterruptRestore(lock);
        err = MDS_ETIME;
    } else {
        MDS_ASSERT(thread != NULL);

        MDS_IPC_PRINT("semaphore suspend thread(%p) entry:%p timer wait:%u", thread, thread->entry, timeout);

        IPC_ListSuspendThread(&(semaphore->list), thread, true, timeout);
        MDS_CoreInterruptRestore(lock);
        MDS_SchedulerCheck();
        err = thread->err;
    }

    MDS_HOOK_CALL(SEMAPHORE_HAS_ACQUIRE, semaphore, err);

    return (err);
}

MDS_Err_t MDS_SemaphoreRelease(MDS_Semaphore_t *semaphore)
{
    MDS_ASSERT(semaphore != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(semaphore->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_IPC_PRINT("release semephore(%p) value:%u", semaphore, semaphore, semaphore->value);

    MDS_Err_t err = MDS_EOK;
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    MDS_HOOK_CALL(SEMAPHORE_HAS_RELEASE, semaphore);

    if (!MDS_ListIsEmpty(&(semaphore->list))) {
        IPC_ListResumeThread(&(semaphore->list));
        MDS_CoreInterruptRestore(lock);
        MDS_SchedulerCheck();
    } else {
        if (semaphore->value < semaphore->max) {
            semaphore->value += 1;
        } else {
            err = MDS_ERANGE;
        }
        MDS_CoreInterruptRestore(lock);
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

/* Condition --------------------------------------------------------------- */
MDS_Err_t MDS_ConditionInit(MDS_Condition_t *condition, const char *name)
{
    MDS_ASSERT(condition != NULL);

    return (MDS_SemaphoreInit(condition, name, 0, -1));
}

MDS_Err_t MDS_ConditionDeInit(MDS_Condition_t *condition)
{
    MDS_ASSERT(condition != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(condition->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    if (!MDS_ListIsEmpty(&(condition->list))) {
        return (MDS_EBUSY);
    }

    while (MDS_SemaphoreAcquire(condition, 0) == MDS_EBUSY) {
        MDS_ConditionBroadCast(condition);
    }

    MDS_SemaphoreDeInit(condition);

    return (MDS_EOK);
}

MDS_Err_t MDS_ConditionBroadCast(MDS_Condition_t *condition)
{
    MDS_ASSERT(condition != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(condition->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_Err_t err;
    do {
        err = MDS_SemaphoreAcquire(condition, 0);
        if ((err == MDS_ETIME) || (err == MDS_EOK)) {
            MDS_SemaphoreRelease(condition);
        }
    } while (err == MDS_ETIME);

    return (err);
}

MDS_Err_t MDS_ConditionSignal(MDS_Condition_t *condition)
{
    MDS_ASSERT(condition != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(condition->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_Err_t err = MDS_EOK;
    register MDS_Item_t lock = MDS_CoreInterruptLock();
    if (MDS_ListIsEmpty(&(condition->list))) {
        MDS_CoreInterruptRestore(lock);
    } else {
        MDS_CoreInterruptRestore(lock);
        err = MDS_SemaphoreRelease(condition);
    }

    return (err);
}

MDS_Err_t MDS_ConditionWait(MDS_Condition_t *condition, MDS_Mutex_t *mutex, MDS_Tick_t timeout)
{
    MDS_ASSERT(condition != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(condition->object)) == MDS_OBJECT_TYPE_SEMAPHORE);
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    MDS_Err_t err = MDS_EOK;
    MDS_Thread_t *thread = MDS_KernelCurrentThread();
    if (thread != mutex->owner) {
        return (MDS_EACCES);
    }

    register MDS_Item_t lock = MDS_CoreInterruptLock();
    if (condition->value > 0) {
        condition->value -= 1;
        MDS_CoreInterruptRestore(lock);
    } else if (timeout == 0) {
        MDS_CoreInterruptRestore(lock);
        err = MDS_ETIME;
    } else {
        MDS_ASSERT(thread != NULL);

        IPC_ListSuspendThread(&(condition->list), thread, false, timeout);
        MDS_MutexRelease(mutex);
        MDS_CoreInterruptRestore(lock);
        MDS_SchedulerCheck();

        err = thread->err;
        MDS_MutexAcquire(mutex, MDS_TICK_FOREVER);
    }

    return (err);
}

/* Mutex ------------------------------------------------------------------- */
MDS_Err_t MDS_MutexInit(MDS_Mutex_t *mutex, const char *name)
{
    MDS_ASSERT(mutex != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(mutex->object), MDS_OBJECT_TYPE_MUTEX, name);
    if (err == MDS_EOK) {
        mutex->owner = NULL;
        mutex->priority = MDS_THREAD_PRIORITY_MAX - 1;
        mutex->value = 1;
        mutex->nest = 0;
        MDS_ListInitNode(&(mutex->list));
    }

    return (err);
}

MDS_Err_t MDS_MutexDeInit(MDS_Mutex_t *mutex)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    IPC_ListResumeAllThread(&(mutex->list));

    return (MDS_ObjectDeInit(&(mutex->object)));
}

MDS_Mutex_t *MDS_MutexCreate(const char *name)
{
    MDS_Mutex_t *mutex = (MDS_Mutex_t *)MDS_ObjectCreate(sizeof(MDS_Mutex_t), MDS_OBJECT_TYPE_MUTEX, name);
    if (mutex != NULL) {
        mutex->owner = NULL;
        mutex->priority = MDS_THREAD_PRIORITY_MAX - 1;
        mutex->value = 1;
        mutex->nest = 0;
        MDS_ListInitNode(&(mutex->list));
    }

    return (mutex);
}

MDS_Err_t MDS_MutexDestroy(MDS_Mutex_t *mutex)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    IPC_ListResumeAllThread(&(mutex->list));

    return (MDS_ObjectDestory(&(mutex->object)));
}

MDS_Err_t MDS_MutexAcquire(MDS_Mutex_t *mutex, MDS_Tick_t timeout)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    MDS_Thread_t *thread = MDS_KernelCurrentThread();
    MDS_ASSERT(thread != NULL);

    MDS_HOOK_CALL(MUTEX_TRY_ACQUIRE, mutex, timeout);

    MDS_IPC_PRINT("thread(%p) entry:%p acquire mutex(%p) value:%u nest:%u onwer:%p", thread, thread->entry, mutex,
                  mutex->value, mutex->nest, mutex->owner);

    MDS_Err_t err = MDS_EOK;
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    if (thread == mutex->owner) {
        if (mutex->nest < (__typeof__(mutex->nest))(-1)) {
            mutex->nest += 1;
        } else {
            err = MDS_ERANGE;
        }
        MDS_CoreInterruptRestore(lock);
    } else if (mutex->value > 0) {
        mutex->value -= 1;
        mutex->owner = thread;
        mutex->priority = thread->currPrio;
        if (mutex->nest < (__typeof__(mutex->nest))(-1)) {
            mutex->nest += 1;
        } else {
            err = MDS_ERANGE;
        }
        MDS_CoreInterruptRestore(lock);
    } else if (timeout == 0) {
        thread->err = MDS_ETIME;
        MDS_CoreInterruptRestore(lock);
        err = MDS_ETIME;
    } else {
        MDS_IPC_PRINT("mutex suspend thread(%p) entry:%p timer wait:%u", thread, thread->entry, timeout);

        if (thread->currPrio < mutex->owner->currPrio) {
            MDS_ThreadChangePriority(mutex->owner, thread->currPrio);
        }
        IPC_ListSuspendThread(&(mutex->list), thread, true, timeout);
        MDS_CoreInterruptRestore(lock);
        MDS_SchedulerCheck();
        err = thread->err;
    }

    MDS_HOOK_CALL(MUTEX_HAS_ACQUIRE, mutex, err);

    return (err);
}

MDS_Err_t MDS_MutexRelease(MDS_Mutex_t *mutex)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    MDS_Thread_t *thread = MDS_KernelCurrentThread();
    MDS_ASSERT(thread != NULL);

    MDS_IPC_PRINT("thread(%p) entry:%p release mutex(%p) value:%u nest:%u onwer:%p", thread, thread->entry, mutex,
                  mutex->value, mutex->nest, mutex->owner);

    register MDS_Item_t lock = MDS_CoreInterruptLock();

    if (thread != mutex->owner) {
        thread->err = MDS_EACCES;
        MDS_CoreInterruptRestore(lock);
        return (MDS_EACCES);
    }

    MDS_HOOK_CALL(MUTEX_HAS_RELEASE, mutex);

    mutex->nest -= 1;
    if (mutex->nest == 0) {
        if (mutex->priority != mutex->owner->currPrio) {
            MDS_ThreadChangePriority(mutex->owner, mutex->priority);
        }
        if (!MDS_ListIsEmpty(&(mutex->list))) {
            mutex->owner = CONTAINER_OF(mutex->list.next, MDS_Thread_t, node);
            mutex->priority = mutex->owner->currPrio;
            if (mutex->nest < (__typeof__(mutex->nest))(-1)) {
                mutex->nest += 1;
            } else {
                MDS_CoreInterruptRestore(lock);
                return (MDS_ERANGE);
            }
            IPC_ListResumeThread(&(mutex->list));
            MDS_CoreInterruptRestore(lock);
            MDS_SchedulerCheck();
            return (MDS_EOK);
        } else {
            mutex->owner = NULL;
            mutex->priority = MDS_THREAD_PRIORITY_MAX - 1;
            if (mutex->value < (__typeof__(mutex->value))(-1)) {
                mutex->value += 1;
            } else {
                MDS_CoreInterruptRestore(lock);
                return (MDS_ERANGE);
            }
        }
    }

    MDS_CoreInterruptRestore(lock);

    return (MDS_EOK);
}

MDS_Thread_t *MDS_MutexGetOwner(const MDS_Mutex_t *mutex)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    return (mutex->owner);
}

/* RwLock ------------------------------------------------------------------ */
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

    MDS_Err_t err = MDS_MutexAcquire(&(rwlock->mutex), 0);
    if (err != MDS_EOK) {
        return (err);
    }

    if ((rwlock->readers != 0) || (!MDS_ListIsEmpty(&(rwlock->condWr.list))) ||
        (!MDS_ListIsEmpty(&(rwlock->condRd.list)))) {
        err = MDS_EBUSY;
    } else {
        err = MDS_SemaphoreAcquire(&(rwlock->condRd), 0);
        if (err == MDS_EOK) {
            err = MDS_SemaphoreAcquire(&(rwlock->condWr), 0);
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

MDS_Err_t MDS_RwLockAcquireRead(MDS_RwLock_t *rwlock, MDS_Tick_t timeout)
{
    MDS_ASSERT(rwlock != NULL);

    MDS_Tick_t startTick = MDS_SysTickGetCount();
    MDS_Err_t err = MDS_MutexAcquire(&(rwlock->mutex), timeout);
    if (err != MDS_EOK) {
        return (err);
    }

    while ((rwlock->readers < 0) || (!MDS_ListIsEmpty(&(rwlock->condWr.list)))) {
        if (timeout != MDS_TICK_FOREVER) {
            MDS_Tick_t elapsedTick = MDS_SysTickGetCount() - startTick;
            timeout = (elapsedTick <= timeout) ? (timeout - elapsedTick) : (0);
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

MDS_Err_t MDS_RwLockAcquireWrite(MDS_RwLock_t *rwlock, MDS_Tick_t timeout)
{
    MDS_ASSERT(rwlock != NULL);

    MDS_Tick_t startTick = MDS_SysTickGetCount();
    MDS_Err_t err = MDS_MutexAcquire(&(rwlock->mutex), timeout);
    if (err != MDS_EOK) {
        return (err);
    }

    while (rwlock->readers != 0) {
        if (timeout != MDS_TICK_FOREVER) {
            MDS_Tick_t elapsedTick = MDS_SysTickGetCount() - startTick;
            timeout = (elapsedTick <= timeout) ? (timeout - elapsedTick) : (0);
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

    MDS_Err_t err = MDS_MutexAcquire(&(rwlock->mutex), MDS_TICK_FOREVER);
    if (err != MDS_EOK) {
        return (err);
    }

    if (rwlock->readers > 0) {
        rwlock->readers -= 1;
    } else if (rwlock->readers == -1) {
        rwlock->readers = 0;
    }

    if (!MDS_ListIsEmpty(&(rwlock->condWr.list))) {
        if (rwlock->readers == 0) {
            err = MDS_ConditionSignal(&(rwlock->condWr));
        }
    } else if (!MDS_ListIsEmpty(&(rwlock->condRd.list))) {
        err = MDS_ConditionBroadCast(&(rwlock->condRd));
    }

    MDS_MutexRelease(&(rwlock->mutex));

    return (err);
}

/* Event ------------------------------------------------------------------- */
MDS_Err_t MDS_EventInit(MDS_Event_t *event, const char *name)
{
    MDS_ASSERT(event != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(event->object), MDS_OBJECT_TYPE_EVENT, name);
    if (err == MDS_EOK) {
        event->value = 0U;
        MDS_ListInitNode(&(event->list));
    }

    return (err);
}

MDS_Err_t MDS_EventDeInit(MDS_Event_t *event)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    IPC_ListResumeAllThread(&(event->list));

    return (MDS_ObjectDeInit(&(event->object)));
}

MDS_Event_t *MDS_EventCreate(const char *name)
{
    MDS_Event_t *event = (MDS_Event_t *)MDS_ObjectCreate(sizeof(MDS_Event_t), MDS_OBJECT_TYPE_EVENT, name);
    if (event != NULL) {
        event->value = 0U;
        MDS_ListInitNode(&(event->list));
    }

    return (event);
}

MDS_Err_t MDS_EventDestroy(MDS_Event_t *event)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    IPC_ListResumeAllThread(&(event->list));

    return (MDS_ObjectDestory(&(event->object)));
}

MDS_Err_t MDS_EventWait(MDS_Event_t *event, MDS_Mask_t mask, MDS_Mask_t opt, MDS_Mask_t *recv, MDS_Tick_t timeout)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);
    MDS_ASSERT((opt & (MDS_EVENT_OPT_AND | MDS_EVENT_OPT_OR)) != (MDS_EVENT_OPT_AND | MDS_EVENT_OPT_OR));

    if ((mask == 0U) || ((opt & (MDS_EVENT_OPT_AND | MDS_EVENT_OPT_OR)) == 0U)) {
        return (MDS_EINVAL);
    }

    MDS_Thread_t *thread = MDS_KernelCurrentThread();

    MDS_HOOK_CALL(EVENT_TRY_ACQUIRE, event, timeout);

    MDS_IPC_PRINT("thread(%p) wait event(%p) which value:%x mask:%x opt:%x", thread, event, event->value, mask, opt);

    MDS_Err_t err = MDS_EOK;
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    if ((((opt & MDS_EVENT_OPT_AND) != 0U) && ((event->value & mask) == mask)) ||
        (((opt & MDS_EVENT_OPT_OR) != 0U) && ((event->value & mask) != 0U))) {
        if (recv != NULL) {
            *recv = thread->eventMask;
        }
        thread->eventMask = event->value & mask;
        thread->eventOpt = opt;
        if ((opt & MDS_EVENT_OPT_NOCLR) == 0U) {
            event->value &= (MDS_Mask_t)(~mask);
        }
        MDS_CoreInterruptRestore(lock);
    } else if (timeout == 0) {
        thread->err = MDS_ETIME;
        MDS_CoreInterruptRestore(lock);
        err = MDS_ETIME;
    } else {
        MDS_ASSERT(thread != NULL);

        MDS_IPC_PRINT("event suspend thread(%p) entry:%p timer wait:%u", thread, thread->entry, timeout);

        thread->eventMask = mask;
        thread->eventOpt = opt;
        IPC_ListSuspendThread(&(event->list), thread, true, timeout);
        MDS_CoreInterruptRestore(lock);
        MDS_SchedulerCheck();
        err = thread->err;

        if (recv != NULL) {
            *recv = thread->eventMask;
        }
    }

    MDS_HOOK_CALL(EVENT_HAS_ACQUIRE, event, err);

    return (err);
}

MDS_Err_t MDS_EventSet(MDS_Event_t *event, MDS_Mask_t mask)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    MDS_IPC_PRINT("event(%p) which value:%x set mask:%x", event, event->value, mask);

    MDS_Thread_t *iter = NULL;
    MDS_Err_t err = MDS_EOK;
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    MDS_HOOK_CALL(EVENT_HAS_SET, event, mask);

    event->value |= mask;
    MDS_LIST_FOREACH_NEXT (iter, node, &(event->list)) {
        err = MDS_EINVAL;
        if ((iter->eventOpt & MDS_EVENT_OPT_AND) != 0U) {
            if ((iter->eventMask & event->value) == iter->eventMask) {
                err = MDS_EOK;
            }
        } else if ((iter->eventOpt & MDS_EVENT_OPT_OR) != 0U) {
            if ((iter->eventMask & event->value) != 0U) {
                iter->eventMask &= event->value;
                err = MDS_EOK;
            }
        } else {
            break;
        }
        if (err == MDS_EOK) {
            if ((iter->eventOpt & MDS_EVENT_OPT_NOCLR) == 0U) {
                event->value &= ~iter->eventMask;
            }
            IPC_ListResumeThread(&(event->list));
            MDS_CoreInterruptRestore(lock);
            MDS_SchedulerCheck();
            return (MDS_EOK);
        }
    }

    MDS_CoreInterruptRestore(lock);

    return (err);
}

MDS_Err_t MDS_EventClr(MDS_Event_t *event, MDS_Mask_t mask)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    MDS_IPC_PRINT("event(%p) which value:%x clr mask:%x", event, event->value, mask);

    register MDS_Item_t lock = MDS_CoreInterruptLock();

    MDS_HOOK_CALL(EVENT_HAS_CLR, event, mask);

    event->value &= (MDS_Mask_t)(~mask);

    MDS_CoreInterruptRestore(lock);

    return (MDS_EOK);
}

MDS_Mask_t MDS_EventGetValue(const MDS_Event_t *event)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    return (event->value);
}

/* MsgQueue ---------------------------------------------------------------- */
#ifdef MDS_MSGQUEUE_SIZE_TYPE
typedef MDS_MSGQUEUE_SIZE_TYPE MDS_MsgQueueSize_t;
#else
typedef size_t MDS_MsgQueueSize_t;
#endif

typedef struct MDS_MsgQueueHeader {
    struct MDS_MsgQueueHeader *next;
    MDS_MsgQueueSize_t len;
} __attribute__((packed)) MDS_MsgQueueHeader_t;

static void MDS_MsgQueueListInit(MDS_MsgQueue_t *msgQueue, size_t msgNums)
{
    msgQueue->lfree = NULL;
    msgQueue->lhead = NULL;
    msgQueue->ltail = NULL;

    for (size_t idx = 0; idx < msgNums; idx++) {
        MDS_MsgQueueHeader_t *list = (MDS_MsgQueueHeader_t *)(&(
            ((uint8_t *)(msgQueue->queBuff))[idx * (sizeof(MDS_MsgQueueHeader_t) + msgQueue->msgSize)]));
        list->next = (MDS_MsgQueueHeader_t *)(msgQueue->lfree);
        msgQueue->lfree = list;
    }
}

MDS_Err_t MDS_MsgQueueInit(MDS_MsgQueue_t *msgQueue, const char *name, void *queBuff, size_t bufSize, size_t msgSize)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(bufSize > (sizeof(MDS_MsgQueueHeader_t) + msgSize));
    MDS_ASSERT(msgSize <= ((__typeof__(((MDS_MsgQueueHeader_t *)0)->len))(-1)));

    MDS_Err_t err = MDS_ObjectInit(&(msgQueue->object), MDS_OBJECT_TYPE_MSGQUEUE, name);
    if (err == MDS_EOK) {
        msgQueue->queBuff = queBuff;
        msgQueue->msgSize = VALUE_ALIGN(msgSize + MDS_SYSMEM_ALIGN_SIZE - 1, MDS_SYSMEM_ALIGN_SIZE);
        MDS_ListInitNode(&(msgQueue->listRecv));
        MDS_ListInitNode(&(msgQueue->listSend));
        MDS_MsgQueueListInit(msgQueue, bufSize / (msgQueue->msgSize + sizeof(MDS_MsgQueueHeader_t)));
    }

    return (err);
}

MDS_Err_t MDS_MsgQueueDeInit(MDS_MsgQueue_t *msgQueue)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);

    IPC_ListResumeAllThread(&(msgQueue->listRecv));
    IPC_ListResumeAllThread(&(msgQueue->listSend));

    return (MDS_ObjectDeInit(&(msgQueue->object)));
}

MDS_MsgQueue_t *MDS_MsgQueueCreate(const char *name, size_t msgSize, size_t msgNums)
{
    MDS_ASSERT(msgSize <= ((__typeof__(((MDS_MsgQueueHeader_t *)0)->len))(-1)));

    MDS_MsgQueue_t *msgQueue = (MDS_MsgQueue_t *)MDS_ObjectCreate(sizeof(MDS_MsgQueue_t), MDS_OBJECT_TYPE_MSGQUEUE,
                                                                  name);

    if (msgQueue != NULL) {
        msgQueue->msgSize = VALUE_ALIGN(msgSize + MDS_SYSMEM_ALIGN_SIZE - 1, MDS_SYSMEM_ALIGN_SIZE);
        msgQueue->queBuff = MDS_SysMemAlloc((msgQueue->msgSize + sizeof(MDS_MsgQueueHeader_t)) * msgNums);
        if (msgQueue->queBuff == NULL) {
            MDS_ObjectDestory(&(msgQueue->object));
            return (NULL);
        }
        MDS_ListInitNode(&(msgQueue->listRecv));
        MDS_ListInitNode(&(msgQueue->listSend));
        MDS_MsgQueueListInit(msgQueue, msgNums);
    }

    return (msgQueue);
}

MDS_Err_t MDS_MsgQueueDestroy(MDS_MsgQueue_t *msgQueue)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);

    IPC_ListResumeAllThread(&(msgQueue->listRecv));
    IPC_ListResumeAllThread(&(msgQueue->listSend));

    MDS_SysMemFree(msgQueue->queBuff);

    return (MDS_ObjectDestory(&(msgQueue->object)));
}

MDS_Err_t MDS_MsgQueueRecvAcquire(MDS_MsgQueue_t *msgQueue, void *recv, size_t *len, MDS_Tick_t timeout)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);

    MDS_Thread_t *thread = MDS_KernelCurrentThread();

    MDS_HOOK_CALL(MSGQUEUE_TRY_RECV, msgQueue, timeout);

    MDS_Err_t err = MDS_EOK;
    MDS_MsgQueueHeader_t *msg = NULL;
    MDS_Item_t lock = MDS_CoreInterruptLock();
    do {
        if (msgQueue->lhead == NULL) {
            if (timeout == 0) {
                err = MDS_ETIME;
                break;
            }

            MDS_ASSERT(thread != NULL);

            MDS_IPC_PRINT("msgqueue(%p) recv suspend thread(%p) entry:%p timer wait:%u", msgQueue, thread,
                          thread->entry, timeout);
        }

        while (msgQueue->lhead == NULL) {
            err = IPC_ListSuspendWait(&lock, &(msgQueue->listRecv), thread, timeout);
            if (err != MDS_EOK) {
                break;
            }
        }

        if (err == MDS_EOK) {
            msg = (MDS_MsgQueueHeader_t *)(msgQueue->lhead);
            msgQueue->lhead = (void *)(msg->next);
            if (msgQueue->lhead == NULL) {
                msgQueue->ltail = NULL;
            }
        }
    } while (0);
    MDS_CoreInterruptRestore(lock);

    MDS_HOOK_CALL(MSGQUEUE_HAS_RECV, msgQueue, err);

    if (err == MDS_EOK) {
        if (recv != NULL) {
            *((uintptr_t *)recv) = (uintptr_t)(msg + 1);
        }
        if (len != NULL) {
            *len = msg->len;
        }

        MDS_IPC_PRINT("thread[(%p) recv message from msgqueue(%p) len:%u", thread, msgQueue, msg->len);
    }

    return (err);
}

MDS_Err_t MDS_MsgQueueRecvRelease(MDS_MsgQueue_t *msgQueue, void *recv)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);

    if (recv == NULL) {
        return (MDS_EINVAL);
    }

    MDS_MsgQueueHeader_t *msg = ((MDS_MsgQueueHeader_t *)(*(uintptr_t *)recv)) - 1;
    if ((((uintptr_t)(msg) - (uintptr_t)(msgQueue->queBuff)) % (sizeof(MDS_MsgQueueHeader_t) + msgQueue->msgSize)) !=
        0) {
        return (MDS_EINVAL);
    }

    MDS_Item_t lock = MDS_CoreInterruptLock();

    msg->next = (MDS_MsgQueueHeader_t *)(msgQueue->lfree);
    msgQueue->lfree = (void *)msg;

    if (!MDS_ListIsEmpty(&(msgQueue->listSend))) {
        IPC_ListResumeThread(&(msgQueue->listSend));
        MDS_CoreInterruptRestore(lock);
        MDS_SchedulerCheck();
    } else {
        MDS_CoreInterruptRestore(lock);
    }

    return (MDS_EOK);
}

MDS_Err_t MDS_MsgQueueRecvCopy(MDS_MsgQueue_t *msgQueue, void *buff, size_t size, size_t *len, MDS_Tick_t timeout)
{
    MDS_ASSERT(buff != NULL);
    MDS_ASSERT(size > 0);

    void *recv = NULL;
    size_t rlen = 0;

    MDS_Err_t err = MDS_MsgQueueRecvAcquire(msgQueue, &recv, &rlen, timeout);
    if (err == MDS_EOK) {
        MDS_MemBuffCopy(buff, size, recv, rlen);
        err = MDS_MsgQueueRecvRelease(msgQueue, &recv);
        if (len != NULL) {
            *len = rlen;
        }
    }

    return (err);
}

MDS_Err_t MDS_MsgQueueSendMsg(MDS_MsgQueue_t *msgQueue, const MDS_MsgList_t *msgList, MDS_Tick_t timeout)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);
    MDS_ASSERT(msgList != NULL);

    size_t len = MDS_MsgListGetLength(msgList);
    if ((len == 0) || (len > msgQueue->msgSize)) {
        return (MDS_EINVAL);
    }

    MDS_Thread_t *thread = MDS_KernelCurrentThread();

    MDS_HOOK_CALL(MSGQUEUE_TRY_SEND, msgQueue, timeout);

    MDS_Err_t err = MDS_EOK;
    MDS_MsgQueueHeader_t *msg = NULL;
    MDS_Item_t lock = MDS_CoreInterruptLock();
    do {
        if (msgQueue->lfree == NULL) {
            if (timeout == 0) {
                err = MDS_ERANGE;
                break;
            }

            MDS_ASSERT(thread != NULL);

            MDS_IPC_PRINT("msgqueue(%p) send suspend thread(%p) entry:%p timer wait:%u", msgQueue, thread,
                          thread->entry, timeout);
        }

        while (msgQueue->lfree == NULL) {
            err = IPC_ListSuspendWait(&lock, &(msgQueue->listSend), thread, timeout);
            if (err != MDS_EOK) {
                break;
            }
        }

        if (err == MDS_EOK) {
            msg = (MDS_MsgQueueHeader_t *)(msgQueue->lfree);
            msgQueue->lfree = (void *)(msg->next);
            msg->next = NULL;
        }
    } while (0);
    MDS_CoreInterruptRestore(lock);

    MDS_HOOK_CALL(MSGQUEUE_HAS_SEND, msgQueue, err);

    if (err != MDS_EOK) {
        return ((err == MDS_ETIME) ? (MDS_ERANGE) : (err));
    }

    msg->len = len;
    MDS_MsgListCopyBuff(msg + 1, msgQueue->msgSize, msgList);

    MDS_IPC_PRINT("send message to msgqueue(%p) len:%u", msgQueue, len);

    lock = MDS_CoreInterruptLock();

    if (msgQueue->ltail != NULL) {
        ((MDS_MsgQueueHeader_t *)(msgQueue->ltail))->next = msg;
    }
    msgQueue->ltail = msg;
    if (msgQueue->lhead == NULL) {
        msgQueue->lhead = msg;
    }

    if (!MDS_ListIsEmpty(&(msgQueue->listRecv))) {
        IPC_ListResumeThread(&(msgQueue->listRecv));
        MDS_CoreInterruptRestore(lock);
        MDS_SchedulerCheck();
    } else {
        MDS_CoreInterruptRestore(lock);
    }

    return (MDS_EOK);
}

MDS_Err_t MDS_MsgQueueSend(MDS_MsgQueue_t *msgQueue, const void *buff, size_t len, MDS_Tick_t timeout)
{
    MDS_ASSERT(buff != NULL);

    const MDS_MsgList_t msgList = {.buff = buff, .len = len, .next = NULL};

    return (MDS_MsgQueueSendMsg(msgQueue, &msgList, timeout));
}

MDS_Err_t MDS_MsgQueueUrgentMsg(MDS_MsgQueue_t *msgQueue, const MDS_MsgList_t *msgList)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);
    MDS_ASSERT(msgList != NULL);

    size_t len = MDS_MsgListGetLength(msgList);
    if ((len == 0) || (len > msgQueue->msgSize)) {
        return (MDS_EINVAL);
    }

    MDS_HOOK_CALL(MSGQUEUE_TRY_SEND, msgQueue, 0);

    MDS_Err_t err = MDS_EOK;
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    MDS_MsgQueueHeader_t *msg = (MDS_MsgQueueHeader_t *)(msgQueue->lfree);
    if (msg == NULL) {
        err = MDS_ERANGE;
    } else {
        msgQueue->lfree = (void *)(msg->next);
    }

    MDS_CoreInterruptRestore(lock);

    MDS_HOOK_CALL(MSGQUEUE_HAS_SEND, msgQueue, err);

    if (err != MDS_EOK) {
        return (err);
    }

    msg->len = len;
    MDS_MsgListCopyBuff(msg + 1, msgQueue->msgSize, msgList);

    MDS_IPC_PRINT("send urgent message to msgqueue(%p) len:%u", msgQueue, len);

    lock = MDS_CoreInterruptLock();

    msg->next = (MDS_MsgQueueHeader_t *)(msgQueue->lhead);
    msgQueue->lhead = msg;
    if (msgQueue->ltail == NULL) {
        msgQueue->ltail = msg;
    }

    if (!MDS_ListIsEmpty(&(msgQueue->listRecv))) {
        IPC_ListResumeThread(&(msgQueue->listRecv));
        MDS_CoreInterruptRestore(lock);
        MDS_SchedulerCheck();
    } else {
        MDS_CoreInterruptRestore(lock);
    }

    return (MDS_EOK);
}

MDS_Err_t MDS_MsgQueueUrgent(MDS_MsgQueue_t *msgQueue, const void *buff, size_t len)
{
    MDS_ASSERT(buff == NULL);

    const MDS_MsgList_t msgList = {.buff = buff, .len = len, .next = NULL};

    return (MDS_MsgQueueUrgentMsg(msgQueue, &msgList));
}

size_t MDS_MsgQueueGetMsgSize(const MDS_MsgQueue_t *msgQueue)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);

    return (msgQueue->msgSize);
}

size_t MDS_MsgQueueGetMsgCount(const MDS_MsgQueue_t *msgQueue)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);

    size_t cnt = 0;
    register MDS_Item_t lock = MDS_CoreInterruptLock();
    MDS_MsgQueueHeader_t *msg = (MDS_MsgQueueHeader_t *)(msgQueue->lhead);
    for (; msg != NULL; msg = msg->next) {
        cnt += 1;
    }
    MDS_CoreInterruptRestore(lock);

    return (cnt);
}

size_t MDS_MsgQueueGetMsgFree(const MDS_MsgQueue_t *msgQueue)
{
    MDS_ASSERT(msgQueue != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(msgQueue->object)) == MDS_OBJECT_TYPE_MSGQUEUE);

    size_t cnt = 0;
    register MDS_Item_t lock = MDS_CoreInterruptLock();
    MDS_MsgQueueHeader_t *msg = (MDS_MsgQueueHeader_t *)(msgQueue->lfree);
    for (; msg != NULL; msg = msg->next) {
        cnt += 1;
    }
    MDS_CoreInterruptRestore(lock);

    return (cnt);
}

/* MemPool --------------------------------------------------------------- */
union MDS_MemPoolHeader {
    union MDS_MemPoolHeader *next;
    MDS_MemPool_t *memPool;
};

static void MDS_MemPoolListInit(MDS_MemPool_t *memPool, size_t blkNums)
{
    memPool->lfree = memPool->memBuff;

    for (size_t idx = 0; idx < blkNums; idx++) {
        union MDS_MemPoolHeader *list = (union MDS_MemPoolHeader *)(&(
            ((uint8_t *)(memPool->memBuff))[idx * (sizeof(union MDS_MemPoolHeader) + memPool->blkSize)]));
        list->next = (union MDS_MemPoolHeader *)(memPool->lfree);
        memPool->lfree = list;
    }
}

MDS_Err_t MDS_MemPoolInit(MDS_MemPool_t *memPool, const char *name, void *memBuff, size_t bufSize, size_t blkSize)
{
    MDS_ASSERT(memPool != NULL);
    MDS_ASSERT(bufSize > (sizeof(union MDS_MemPoolHeader) + blkSize));

    MDS_Err_t err = MDS_ObjectInit(&(memPool->object), MDS_OBJECT_TYPE_MEMPOOL, name);
    if (err == MDS_EOK) {
        MDS_ListInitNode(&(memPool->list));
        memPool->memBuff = memBuff;
        memPool->blkSize = VALUE_ALIGN(blkSize + MDS_SYSMEM_ALIGN_SIZE - 1, MDS_SYSMEM_ALIGN_SIZE);
        MDS_MemPoolListInit(memPool, bufSize / (memPool->blkSize + sizeof(union MDS_MemPoolHeader)));
    }

    return (err);
}

MDS_Err_t MDS_MemPoolDeInit(MDS_MemPool_t *memPool)
{
    MDS_ASSERT(memPool != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memPool->object)) == MDS_OBJECT_TYPE_MEMPOOL);

    IPC_ListResumeAllThread(&(memPool->list));

    return (MDS_ObjectDeInit(&(memPool->object)));
}

MDS_MemPool_t *MDS_MemPoolCreate(const char *name, size_t blkSize, size_t blkNums)
{
    MDS_MemPool_t *memPool = (MDS_MemPool_t *)MDS_ObjectCreate(sizeof(MDS_MemPool_t), MDS_OBJECT_TYPE_MEMPOOL, name);

    if (memPool != NULL) {
        memPool->blkSize = VALUE_ALIGN(blkSize + MDS_SYSMEM_ALIGN_SIZE - 1, MDS_SYSMEM_ALIGN_SIZE);
        memPool->memBuff = MDS_SysMemAlloc((memPool->blkSize + sizeof(MDS_MsgQueueHeader_t)) * blkNums);
        if (memPool->memBuff == NULL) {
            MDS_ObjectDestory(&(memPool->object));
            return (NULL);
        }
        MDS_ListInitNode(&(memPool->list));
        MDS_MemPoolListInit(memPool, blkNums);
    }

    return (memPool);
}

MDS_Err_t MDS_MemPoolDestroy(MDS_MemPool_t *memPool)
{
    MDS_ASSERT(memPool != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memPool->object)) == MDS_OBJECT_TYPE_MEMPOOL);

    IPC_ListResumeAllThread(&(memPool->list));

    MDS_SysMemFree(memPool->memBuff);

    return (MDS_ObjectDestory(&(memPool->object)));
}

void *MDS_MemPoolAlloc(MDS_MemPool_t *memPool, MDS_Tick_t timeout)
{
    MDS_ASSERT(memPool != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memPool->object)) == MDS_OBJECT_TYPE_MEMPOOL);

    MDS_HOOK_CALL(MEMPOOL_TRY_ALLOC, memPool, timeout);

    MDS_Thread_t *thread = MDS_KernelCurrentThread();

    MDS_Err_t err = MDS_EOK;
    union MDS_MemPoolHeader *blk = NULL;
    MDS_Item_t lock = MDS_CoreInterruptLock();
    do {
        if (memPool->lfree == NULL) {
            if (timeout == 0) {
                err = MDS_ETIME;
                break;
            }

            MDS_ASSERT(thread != NULL);

            MDS_IPC_PRINT("mempool(%p) alloc blocksize:%u suspend thread(%p) entry:%p timer wait:%u", memPool,
                          memPool->blkSize, thread, thread->entry, timeout);
        }

        while (memPool->lfree == NULL) {
            err = IPC_ListSuspendWait(&lock, &(memPool->list), thread, timeout);
            if (err != MDS_EOK) {
                break;
            }
        }

        if (err == MDS_EOK) {
            blk = (union MDS_MemPoolHeader *)(memPool->lfree);
            memPool->lfree = (void *)(blk->next);
            blk->memPool = memPool;
        }
    } while (0);
    MDS_CoreInterruptRestore(lock);

    MDS_HOOK_CALL(MEMPOOL_HAS_ALLOC, memPool, blk);

    return (((err == MDS_EOK) && (blk != NULL)) ? ((void *)(blk + 1)) : (NULL));
}

static void MDS_MemPoolFreeBlk(union MDS_MemPoolHeader *blk)
{
    MDS_MemPool_t *memPool = blk->memPool;
    if (memPool == NULL) {
        return;
    }

    MDS_ASSERT(MDS_ObjectGetType(&(memPool->object)) == MDS_OBJECT_TYPE_MEMPOOL);

    register MDS_Item_t lock = MDS_CoreInterruptLock();

    blk->next = memPool->lfree;
    memPool->lfree = blk;

    if (!MDS_ListIsEmpty(&(memPool->list))) {
        IPC_ListResumeThread(&(memPool->list));
        MDS_CoreInterruptRestore(lock);
        MDS_SchedulerCheck();
    } else {
        MDS_CoreInterruptRestore(lock);
    }

    MDS_HOOK_CALL(MEMPOOL_HAS_FREE, memPool, blk);

    MDS_IPC_PRINT("free block to mempool(%p) which blksize:%u", memPool, memPool->blkSize);
}

void MDS_MemPoolFree(void *blkPtr)
{
    if (blkPtr == NULL) {
        return;
    }

    union MDS_MemPoolHeader *blk = (union MDS_MemPoolHeader *)((uint8_t *)blkPtr - sizeof(union MDS_MemPoolHeader));
    if (blk != NULL) {
        MDS_MemPoolFreeBlk(blk);
    }
}

size_t MDS_MemPoolGetBlkSize(const MDS_MemPool_t *memPool)
{
    MDS_ASSERT(memPool != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memPool->object)) == MDS_OBJECT_TYPE_MEMPOOL);

    return (memPool->blkSize);
}

size_t MDS_MemPoolGetBlkFree(const MDS_MemPool_t *memPool)
{
    MDS_ASSERT(memPool != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memPool->object)) == MDS_OBJECT_TYPE_MEMPOOL);

    size_t cnt = 0;
    register MDS_Item_t lock = MDS_CoreInterruptLock();
    union MDS_MemPoolHeader *blk = (union MDS_MemPoolHeader *)(memPool->lfree);
    for (; blk != NULL; blk = blk->next) {
        cnt += 1;
    }
    MDS_CoreInterruptRestore(lock);

    return (cnt);
}
