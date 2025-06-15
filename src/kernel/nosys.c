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
#include "sys/kernel.h"

/* Semaphore --------------------------------------------------------------- */
MDS_Err_t MDS_SemaphoreInit(MDS_Semaphore_t *semaphore, const char *name, size_t init, size_t max)
{
    MDS_ASSERT(semaphore != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(semaphore->object), MDS_OBJECT_TYPE_SEMAPHORE, name);
    if (err == MDS_EOK) {
        semaphore->value = init;
        semaphore->max = max;
    }

    return (err);
}

MDS_Err_t MDS_SemaphoreDeInit(MDS_Semaphore_t *semaphore)
{
    MDS_ASSERT(semaphore != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(semaphore->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    return (MDS_ObjectDeInit(&(semaphore->object)));
}

MDS_Semaphore_t *MDS_SemaphoreCreate(const char *name, size_t init, size_t max)
{
    MDS_Semaphore_t *semaphore = (MDS_Semaphore_t *)MDS_ObjectCreate(sizeof(MDS_Semaphore_t),
                                                                     MDS_OBJECT_TYPE_SEMAPHORE,
                                                                     name);
    if (semaphore != NULL) {
        semaphore->value = init;
        semaphore->max = max;
    }

    return (semaphore);
}

MDS_Err_t MDS_SemaphoreDestroy(MDS_Semaphore_t *semaphore)
{
    MDS_ASSERT(semaphore != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(semaphore->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    return (MDS_ObjectDestroy(&(semaphore->object)));
}

MDS_Err_t MDS_SemaphoreAcquire(MDS_Semaphore_t *semaphore, MDS_Timeout_t timeout)
{
    MDS_ASSERT(semaphore != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(semaphore->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_Err_t err = MDS_ETIMEOUT;
    MDS_Tick_t tickstart = MDS_ClockGetTickCount();

    MDS_LOOP {
        MDS_Lock_t lock = MDS_CoreInterruptLock();
        if (semaphore->value > 0) {
            semaphore->value -= 1;
            err = MDS_EOK;
        }
        MDS_CoreInterruptRestore(lock);

        if ((err == MDS_EOK) || ((MDS_ClockGetTickCount() - tickstart) >= timeout.ticks)) {
            break;
        }
    }

    return (err);
}

MDS_Err_t MDS_SemaphoreRelease(MDS_Semaphore_t *semaphore)
{
    MDS_ASSERT(semaphore != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(semaphore->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_Err_t err = MDS_EOK;

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    if (semaphore->value < semaphore->max) {
        semaphore->value += 1;
    } else {
        err = MDS_ERANGE;
    }
    MDS_CoreInterruptRestore(lock);

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

/* Mutex ------------------------------------------------------------------- */
MDS_Err_t MDS_MutexInit(MDS_Mutex_t *mutex, const char *name)
{
    MDS_ASSERT(mutex != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(mutex->object), MDS_OBJECT_TYPE_MUTEX, name);
    if (err == MDS_EOK) {
        mutex->owner = NULL;
        mutex->priority.priority = CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX;
        mutex->value = 1;
        mutex->nest = 0;
    }

    return (err);
}

MDS_Err_t MDS_MutexDeInit(MDS_Mutex_t *mutex)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    return (MDS_ObjectDeInit(&(mutex->object)));
}

MDS_Mutex_t *MDS_MutexCreate(const char *name)
{
    MDS_Mutex_t *mutex = (MDS_Mutex_t *)MDS_ObjectCreate(sizeof(MDS_Mutex_t),
                                                         MDS_OBJECT_TYPE_MUTEX, name);
    if (mutex != NULL) {
        mutex->owner = NULL;
        mutex->priority.priority = CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX;
        mutex->value = 1;
        mutex->nest = 0;
    }

    return (mutex);
}

MDS_Err_t MDS_MutexDestroy(MDS_Mutex_t *mutex)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    return (MDS_ObjectDestroy(&(mutex->object)));
}

MDS_Err_t MDS_MutexAcquire(MDS_Mutex_t *mutex, MDS_Timeout_t timeout)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    MDS_Err_t err = MDS_ETIMEOUT;
    MDS_Tick_t tickstart = MDS_ClockGetTickCount();

    MDS_LOOP {
        MDS_Lock_t lock = MDS_CoreInterruptLock();
        if (mutex->value > 0) {
            mutex->value = 0;
            err = MDS_EOK;
        }
        MDS_CoreInterruptRestore(lock);

        if ((err == MDS_EOK) || ((MDS_ClockGetTickCount() - tickstart) >= timeout.ticks)) {
            break;
        }
    }

    return (MDS_EOK);
}

MDS_Err_t MDS_MutexRelease(MDS_Mutex_t *mutex)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    mutex->value = 1;
    MDS_CoreInterruptRestore(lock);

    return (MDS_EOK);
}

MDS_Thread_t *MDS_MutexGetOwner(const MDS_Mutex_t *mutex)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    return (mutex->owner);
}

/* Event ------------------------------------------------------------------- */
MDS_Err_t MDS_EventInit(MDS_Event_t *event, const char *name)
{
    MDS_ASSERT(event != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(event->object), MDS_OBJECT_TYPE_EVENT, name);
    if (err == MDS_EOK) {
        event->value.mask = 0U;
    }

    return (err);
}

MDS_Err_t MDS_EventDeInit(MDS_Event_t *event)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    return (MDS_ObjectDeInit(&(event->object)));
}

MDS_Event_t *MDS_EventCreate(const char *name)
{
    MDS_Event_t *event = (MDS_Event_t *)MDS_ObjectCreate(sizeof(MDS_Event_t),
                                                         MDS_OBJECT_TYPE_EVENT, name);
    if (event != NULL) {
        event->value.mask = 0U;
    }

    return (event);
}

MDS_Err_t MDS_EventDestroy(MDS_Event_t *event)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    return (MDS_ObjectDestroy(&(event->object)));
}

MDS_Err_t MDS_EventWait(MDS_Event_t *event, MDS_Mask_t mask, MDS_EventOpt_t opt, MDS_Mask_t *recv,
                        MDS_Timeout_t timeout)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);
    MDS_ASSERT((opt & (MDS_EVENT_OPT_AND | MDS_EVENT_OPT_OR)) !=
               (MDS_EVENT_OPT_AND | MDS_EVENT_OPT_OR));

    if ((mask.mask == 0U) || ((opt & (MDS_EVENT_OPT_AND | MDS_EVENT_OPT_OR)) == 0U)) {
        return (MDS_EINVAL);
    }

    MDS_Err_t err = MDS_ETIMEOUT;
    MDS_Tick_t tickstart = MDS_ClockGetTickCount();

    MDS_LOOP {
        MDS_Lock_t lock = MDS_CoreInterruptLock();
        if ((((opt & MDS_EVENT_OPT_AND) != 0U) &&
             ((event->value.mask & mask.mask) == mask.mask)) ||
            (((opt & MDS_EVENT_OPT_OR) != 0U) && ((event->value.mask & mask.mask) != 0U))) {
            if (recv != NULL) {
                *recv = event->value;
            }
            if ((opt & MDS_EVENT_OPT_NOCLR) == 0U) {
                event->value.mask &= (~mask.mask);
            }
            err = MDS_EOK;
        }
        MDS_CoreInterruptRestore(lock);

        if ((err == MDS_EOK) || ((MDS_ClockGetTickCount() - tickstart) >= timeout.ticks)) {
            break;
        }
    }

    return (err);
}

MDS_Err_t MDS_EventSet(MDS_Event_t *event, MDS_Mask_t mask)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    event->value.mask |= mask.mask;
    MDS_CoreInterruptRestore(lock);

    return (MDS_EOK);
}

MDS_Err_t MDS_EventClr(MDS_Event_t *event, MDS_Mask_t mask)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    event->value.mask &= (~mask.mask);
    MDS_CoreInterruptRestore(lock);

    return (MDS_EOK);
}

MDS_Mask_t MDS_EventGetValue(const MDS_Event_t *event)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    return (event->value);
}

/* Timer ------------------------------------------------------------------- */
#if (defined(CONFIG_MDS_TIMER_INDEPENDENT) && (CONFIG_MDS_TIMER_INDEPENDENT != 0))
static MDS_WorkQueue_t g_sysTimerQueue;

static int TIMER_SkipListCompare(const MDS_DListNode_t *node, const void *value)
{
    const MDS_Timer_t *workl = CONTAINER_OF(node, MDS_Timer_t, node);
    const MDS_Timer_t *workn = (const MDS_Timer_t *)value;

    MDS_Tick_t tickdiff = workl->tickout - workn->tickout;
    if ((tickdiff > 0) && (tickdiff < MDS_CLOCK_TICK_TIMER_MAX)) {
        return (1);  // > 0 to break;
    } else {
        return (-1);  // < 0 to continue;
    }
}

static MDS_Timer_t *TIMER_SkipListPeek(const MDS_WorkQueue_t *workq)
{
    MDS_Timer_t *timer = NULL;

    if (!MDS_DListIsEmpty(&(workq->list[ARRAY_SIZE(timer->node) - 1]))) {
        timer = CONTAINER_OF(workq->list[ARRAY_SIZE(timer->node) - 1].next, MDS_Timer_t,
                             node[ARRAY_SIZE(timer->node) - 1]);
    }

    return (timer);
}

static void TIMER_SkipListRemove(MDS_Timer_t *timer)
{
    MDS_SkipListRemoveNode(timer->node, ARRAY_SIZE(timer->node));
}

static void TIMER_SkipListInsert(MDS_WorkQueue_t *workq, MDS_Timer_t *timer, MDS_Tick_t tickout)
{
    static size_t skipRand = 0;

    MDS_Tick_t tickcurr = MDS_ClockGetTickCount();
    MDS_DListNode_t *skipNode[ARRAY_SIZE(timer->node)];

    timer->tickout = tickcurr + tickout;
    MDS_SkipListSearchNode(skipNode, workq->list, ARRAY_SIZE(workq->list), timer,
                           TIMER_SkipListCompare);

    skipRand = skipRand + tickcurr + 1;
    MDS_SkipListInsertNode(skipNode, timer->node, ARRAY_SIZE(timer->node), skipRand,
                           CONFIG_MDS_TIMER_SKIPLIST_SHIFT);
}

static MDS_Err_t TIMER_SkipListCheck(MDS_WorkQueue_t *workq, MDS_Lock_t *lock)
{
    MDS_Err_t err = MDS_EOK;

    if ((workq->list[0].next == NULL) || (workq->list[0].prev == NULL)) {
        return (MDS_ENOENT);
    }

    MDS_DListNode_t runList = MDS_DLIST_INIT(runList);

    while (!MDS_DListIsEmpty(&(workq->list[ARRAY_SIZE(workq->list) - 1]))) {
        MDS_Timer_t *timer = TIMER_SkipListPeek(workq);
        MDS_Tick_t tickcurr = MDS_ClockGetTickCount();

        if ((tickcurr - timer->tickout) >= MDS_CLOCK_TICK_TIMER_MAX) {
            break;
        }

        TIMER_SkipListRemove(timer);
        MDS_DListInsertNodeNext(&runList, &(timer->node[ARRAY_SIZE(timer->node) - 1]));

        if (timer->entry != NULL) {
            MDS_CriticalRestore(NULL, *lock);

            timer->entry(timer, timer->arg);

            *lock = MDS_CoreInterruptLock();
        }

        if (MDS_DListIsEmpty(&runList)) {
            continue;
        }

        MDS_DListRemoveNode(&(timer->node[ARRAY_SIZE(timer->node) - 1]));
        if (timer->tperiod > MDS_CLOCK_TICK_NO_WAIT) {
            TIMER_SkipListInsert(workq, timer, timer->tperiod);
        }
    }

    return (err);
}

MDS_Tick_t TIMER_SkipListNextTick(MDS_WorkQueue_t *workq)
{
    MDS_Tick_t ticknext = MDS_CLOCK_TICK_FOREVER;

    MDS_Lock_t lock = MDS_CoreInterruptLock();
    MDS_Timer_t *timer = TIMER_SkipListPeek(workq);
    if (timer != NULL) {
        ticknext = timer->tickout;
    }
    MDS_CoreInterruptRestore(lock);

    return (ticknext);
}

MDS_Err_t MDS_TimerInit(MDS_Timer_t *timer, const char *name, MDS_TimerEntry_t entry,
                        MDS_TimerEntry_t stop, MDS_Arg_t *arg)
{
    MDS_ASSERT(timer != NULL);

    if (timer->queue != NULL) {
        return (MDS_EAGAIN);
    }

    MDS_Err_t err = MDS_ObjectInit(&(timer->object), MDS_OBJECT_TYPE_WORKNODE, name);
    if (err == MDS_EOK) {
        MDS_SkipListInitNode(timer->node, ARRAY_SIZE(timer->node));
        timer->entry = entry;
        timer->stop = stop;
        timer->arg = arg;
    }

    return (err);
}

MDS_Err_t MDS_TimerDeInit(MDS_Timer_t *timer)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(timer->object)) == MDS_OBJECT_TYPE_WORKNODE);

    MDS_Err_t err = MDS_TimerStop(timer);
    if (err == MDS_EOK) {
        MDS_ObjectDeInit(&(timer->object));
    }

    return (err);
}

MDS_Timer_t *MDS_TimerCreate(const char *name, MDS_WorkEntry_t entry, MDS_WorkEntry_t stop,
                             MDS_Arg_t *arg)
{
    MDS_Timer_t *timer = (MDS_Timer_t *)MDS_ObjectCreate(sizeof(MDS_Timer_t),
                                                         MDS_OBJECT_TYPE_WORKNODE, name);
    if (timer != NULL) {
        MDS_SkipListInitNode(timer->node, ARRAY_SIZE(timer->node));
        timer->entry = entry;
        timer->stop = stop;
        timer->arg = arg;
    }

    return (timer);
}

MDS_Err_t MDS_TimerDestroy(MDS_Timer_t *timer)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(timer->object)) == MDS_OBJECT_TYPE_WORKNODE);

    MDS_Err_t err = MDS_TimerStop(timer);
    if (err == MDS_EOK) {
        MDS_ObjectDestroy(&(timer->object));
    }

    return (err);
}

MDS_Err_t MDS_TimerStart(MDS_Timer_t *timer, MDS_Timeout_t duration, MDS_Timeout_t period)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(timer->object)) == MDS_OBJECT_TYPE_WORKNODE);

    if ((duration.ticks >= MDS_CLOCK_TICK_TIMER_MAX) ||
        (period.ticks >= MDS_CLOCK_TICK_TIMER_MAX)) {
        return (MDS_EINVAL);
    }

    MDS_Lock_t lock = MDS_CoreInterruptLock();

    timer->tperiod = period.ticks;
    TIMER_SkipListInsert(&g_sysTimerQueue, timer, duration.ticks);
    timer->queue = &g_sysTimerQueue;

    MDS_CoreInterruptRestore(lock);

    return (MDS_EOK);
}

MDS_Err_t MDS_TimerStop(MDS_Timer_t *timer)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(timer->object)) == MDS_OBJECT_TYPE_WORKNODE);

    if (timer->queue == NULL) {
        return (MDS_EAGAIN);
    }

    timer->queue = NULL;
    TIMER_SkipListRemove(timer);

    if (timer->stop != NULL) {
        timer->stop(timer, timer->arg);
    }

    return (MDS_EOK);
}

bool MDS_TimerIsActive(const MDS_Timer_t *timer)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(timer->object)) == MDS_OBJECT_TYPE_WORKNODE);

    return (timer->queue != NULL);
}
#endif

void MDS_SysTimerCheck(void)
{
#if (defined(CONFIG_MDS_TIMER_INDEPENDENT) && (CONFIG_MDS_TIMER_INDEPENDENT != 0))
    MDS_Lock_t lock = MDS_CoreInterruptLock();

    TIMER_SkipListCheck(&g_sysTimerQueue, &lock);

    MDS_CoreInterruptRestore(lock);
#endif
}

MDS_Tick_t MDS_SysTimerNextTick(void)
{
#if (defined(CONFIG_MDS_TIMER_INDEPENDENT) && (CONFIG_MDS_TIMER_INDEPENDENT != 0))
    return (TIMER_SkipListNextTick(&g_sysTimerQueue));
#else
    return (MDS_CLOCK_TICK_FOREVER);
#endif
}

/* Thread ------------------------------------------------------------------ */
MDS_Err_t MDS_ThreadDelayTick(MDS_Timeout_t delay)
{
    MDS_ClockDelayRawTick(delay);

    return (MDS_EOK);
}

/* Kernel ------------------------------------------------------------------ */
static volatile int g_sysCriticalNest = 0;

MDS_Thread_t *MDS_KernelCurrentThread(void)
{
    return (NULL);
}

void MDS_KernelEnterCritical(void)
{
    MDS_Lock_t lock = MDS_CoreInterruptLock();

    g_sysCriticalNest += 1;

    MDS_CoreInterruptRestore(lock);
}

void MDS_KernelExitCritical(void)
{
    MDS_Lock_t lock = MDS_CoreInterruptLock();

    g_sysCriticalNest -= 1;

    MDS_CoreInterruptRestore(lock);
}

size_t MDS_KernelGetCritical(void)
{
    return (g_sysCriticalNest);
}

MDS_Tick_t MDS_KernelGetSleepTick(void)
{
    MDS_Tick_t sleepTick = 0;

    MDS_Lock_t lock = MDS_CoreInterruptLock();

    MDS_Tick_t nextTick = MDS_SysTimerNextTick();
    MDS_Tick_t currTick = MDS_ClockGetTickCount();
    sleepTick = (nextTick > currTick) ? (nextTick - currTick) : (0);

    MDS_CoreInterruptRestore(lock);

    return (sleepTick);
}

void MDS_KernelCompensateTick(MDS_Tick_t ticks)
{
    MDS_Lock_t lock = MDS_CoreInterruptLock();

    MDS_Tick_t currTick = MDS_ClockGetTickCount() + ticks;
    MDS_ClockIncTickCount(currTick);
    MDS_SysTimerCheck();

    MDS_CoreInterruptRestore(lock);
}

/* Clock ------------------------------------------------------------------- */
static volatile MDS_Tick_t g_sysClockTickCount = 0U;

void MDS_SysTickHandler(void)
{
    MDS_ClockIncTickCount(1);
}

MDS_Tick_t MDS_ClockGetTickCount(void)
{
    return (g_sysClockTickCount);
}

void MDS_ClockIncTickCount(MDS_Tick_t ticks)
{
    MDS_Lock_t lock = MDS_CoreInterruptLock();

    // atomic_add
    g_sysClockTickCount += ticks;

    MDS_CoreInterruptRestore(lock);

    MDS_SysTimerCheck();
}
