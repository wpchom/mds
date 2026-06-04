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
#include "mds/sys.h"

/* Define ------------------------------------------------------------------ */
#ifndef CONFIG_MDS_TIMER_INDEPENDENT
#define CONFIG_MDS_TIMER_INDEPENDENT 0
#endif

/* Timer ------------------------------------------------------------------- */
#if (defined(CONFIG_MDS_TIMER_INDEPENDENT) && (CONFIG_MDS_TIMER_INDEPENDENT != 0))
static MDS_WorkQueue_t g_sysTimerQueue;

#if (defined(CONFIG_MDS_TIMER_SKIPLIST_LEVEL) && (CONFIG_MDS_TIMER_SKIPLIST_LEVEL > 0))
static int WORKQ_SkipListCompare(const MDS_DListNode_t *node, MDS_Arg_t arg)
{
    const MDS_WorkNode_t *workl = CONTAINER_OF(node, MDS_WorkNode_t, node);
    const MDS_WorkNode_t *workn = CONTAINER_OF(arg.ptr, MDS_WorkNode_t, node);

    MDS_Tick_t tickdiff = workl->tickout - workn->tickout;
    if ((tickdiff > 0) && (tickdiff < MDS_CLOCK_TICK_TIMER_MAX)) {
        return (1); // > 0 to break;
    } else {
        return (-1); // < 0 to continue;
    }
}

static inline void WORKQ_InitWorkQueue(MDS_WorkQueue_t *workq)
{
    MDS_SkipListInitNode(workq->list, ARRAY_SIZE(workq->list));
}

static inline void WORKQ_InitWorkNode(MDS_WorkNode_t *workn)
{
    MDS_SkipListInitNode(workn->node, ARRAY_SIZE(workn->node));
}

static inline void WORKQ_RemoveWorkNode(MDS_WorkNode_t *workn)
{
    MDS_SkipListRemoveNode(workn->node, ARRAY_SIZE(workn->node));
}

static void WORKQ_InsertWorkNode(MDS_WorkQueue_t *workq, MDS_WorkNode_t *workn, MDS_Tick_t tickout)
{
    static size_t skipRand = 0;

    MDS_Tick_t tickcurr = MDS_ClockGetTickCount();
    MDS_DListNode_t *skipNode[ARRAY_SIZE(workq->list)];

    workn->tickout = tickcurr + tickout;
    MDS_SkipListSearchNode(skipNode, workq->list, ARRAY_SIZE(workq->list), WORKQ_SkipListCompare,
                           MDS_ARG_WITH(&(workn->node)));

    skipRand = skipRand + tickcurr + 1;
    MDS_SkipListInsertNode(skipNode, workn->node, ARRAY_SIZE(workn->node), skipRand,
                           CONFIG_MDS_TIMER_SKIPLIST_SHIFT);
}

static MDS_WorkNode_t *WORKQ_PeekWorkNode(const MDS_WorkQueue_t *workq)
{
    MDS_WorkNode_t *workn = NULL;

    if (!MDS_DListIsEmpty(&(workq->list[ARRAY_SIZE(workq->list) - 1]))) {
        workn = CONTAINER_OF(workq->list[ARRAY_SIZE(workq->list) - 1].next, MDS_WorkNode_t,
                             node[ARRAY_SIZE(workq->list) - 1]);
    }

    return (workn);
}

void MDS_WorkQueueCheck(MDS_WorkQueue_t *workq, MDS_Lock_t *lock)
{
    if ((workq->list[0].next == NULL) || (workq->list[0].prev == NULL)) {
        MDS_SpinLockInit(&(workq->spinlock));
        WORKQ_InitWorkQueue(workq);
        return;
    }

    MDS_DListNode_t runList = MDS_DLIST_INIT(runList);

    while (!MDS_DListIsEmpty(&(workq->list[ARRAY_SIZE(workq->list) - 1]))) {
        MDS_WorkNode_t *workn = WORKQ_PeekWorkNode(workq);

        MDS_Tick_t tickcurr = MDS_ClockGetTickCount();
        if ((tickcurr - workn->tickout) >= MDS_CLOCK_TICK_TIMER_MAX) {
            break;
        }

        WORKQ_RemoveWorkNode(workn);
        MDS_DListInsertNodeNext(&runList, &(workn->node[ARRAY_SIZE(workn->node) - 1]));

        MDS_HOOK_CALL(KERNEL, timer, (workn, MDS_KERNEL_TRACE_TIMER_ENTER));
        if (workn->entry != NULL) {
            MDS_CriticalRestore(&(workq->spinlock), *lock);
            workn->entry(workn, workn->arg);
            *lock = MDS_CriticalLock(&(workq->spinlock));
        }
        MDS_HOOK_CALL(KERNEL, timer, (workn, MDS_KERNEL_TRACE_TIMER_EXIT));

        if (MDS_DListIsEmpty(&runList)) {
            continue;
        }

        MDS_DListRemoveNode(&(workn->node[ARRAY_SIZE(workn->node) - 1]));
        if ((workn->tperiod > MDS_CLOCK_TICK_NO_WAIT) &&
            (workn->tperiod < MDS_CLOCK_TICK_TIMER_MAX)) {
            WORKQ_InsertWorkNode(workq, workn, workn->tperiod);
        }
    }
}

#else
const uint8_t G_WORKQ_TIMINGWHEEL_TABLE[] = {CONFIG_MDS_TIMER_WHEEL_TABLE};
const size_t G_WORKQ_TIMINGWHEEL_SHIFT =
    MDS_ARGUMENT_FOREACH_ARGS(__ARGUMENT_ELEM, (+), 0, CONFIG_MDS_TIMER_WHEEL_TABLE);
static inline void WORKQ_InitWorkQueue(MDS_WorkQueue_t *workq)
{
    for (size_t i = 0; i < ARRAY_SIZE(workq->wheelList); i++) {
        MDS_DListInitNode(&(workq->wheelList[i]));
    }
    MDS_DListInitNode(&(workq->overList));
    workq->ticklast = MDS_ClockGetTickCount();
}

static inline void WORKQ_InitWorkNode(MDS_WorkNode_t *workn)
{
    MDS_DListInitNode(&(workn->node));
}

static inline void WORKQ_RemoveWorkNode(MDS_WorkNode_t *workn)
{
    MDS_DListRemoveNode(&(workn->node));
}

static void WORKQ_InsertWorkNode(MDS_WorkQueue_t *workq, MDS_WorkNode_t *workn, MDS_Tick_t tickout)
{
    MDS_Tick_t tickcurr = MDS_ClockGetTickCount();

    size_t base = 0;
    size_t slot = 0;
    size_t level = 0;

    workn->tickout = tickcurr + tickout;

    for (size_t shift = 0; level < ARRAY_SIZE(G_WORKQ_TIMINGWHEEL_TABLE); level++) {
        size_t mask = (1UL << (shift + G_WORKQ_TIMINGWHEEL_TABLE[level])) - 1;
        size_t curr = (workq->ticklast & mask) >> shift;

        slot = tickout >> shift;
        if (slot < ((1UL << G_WORKQ_TIMINGWHEEL_TABLE[level]) - curr)) {
            slot += curr;
            break;
        }

        tickout += curr << shift;
        shift += G_WORKQ_TIMINGWHEEL_TABLE[level];
        base += 1UL << G_WORKQ_TIMINGWHEEL_TABLE[level];
    }

    if (level >= ARRAY_SIZE(G_WORKQ_TIMINGWHEEL_TABLE)) {
        MDS_DListInsertNodePrev(&(workq->overList), &(workn->node));
    } else {
        MDS_DListInsertNodePrev(&(workq->wheelList[base + slot]), &(workn->node));
    }
}

static MDS_WorkNode_t *WORKQ_PeekWorkNode(const MDS_WorkQueue_t *workq)
{
    MDS_WorkNode_t *workn = NULL;

    for (size_t idx = workq->ticklast & ((1UL << G_WORKQ_TIMINGWHEEL_TABLE[0]) - 1);
         idx < ARRAY_SIZE(workq->wheelList); idx++) {
        if (MDS_DListIsEmpty(&(workq->wheelList[idx]))) {
            continue;
        }
        workn = CONTAINER_OF(workq->wheelList[idx].next, MDS_WorkNode_t, node);
        break;
    }

    if (workn == NULL) {
        MDS_WorkNode_t *find = NULL;
        MDS_WorkNode_t *iter = NULL;

        MDS_DLIST_CONTAINER_FOREACH_NEXT (iter, node, &(workq->overList)) {
            if ((find == NULL) ||
                ((iter->tickout - workq->ticklast) < (find->tickout - workq->ticklast))) {
                find = iter;
            }
        }

        workn = find;
    }

    return (workn);
}

static size_t MDS_WorkQueueWheel(MDS_WorkQueue_t *workq)
{
    size_t base = 0;
    size_t slot = 0;
    size_t level = 0;

    workq->ticklast++;

    for (size_t shift = 0; level < ARRAY_SIZE(G_WORKQ_TIMINGWHEEL_TABLE); level++) {
        size_t mask = (1UL << (shift + G_WORKQ_TIMINGWHEEL_TABLE[level])) - 1;

        slot = (workq->ticklast & mask) >> shift;
        if (slot != 0) {
            break;
        }

        shift += G_WORKQ_TIMINGWHEEL_TABLE[level];
        base += 1UL << G_WORKQ_TIMINGWHEEL_TABLE[level];
    }

    if (level > 0) {
        MDS_DListNode_t *list = (level < ARRAY_SIZE(G_WORKQ_TIMINGWHEEL_TABLE))
            ? (&workq->wheelList[base + slot])
            : (&workq->overList);

        while (!MDS_DListIsEmpty(list)) {
            MDS_WorkNode_t *workn = CONTAINER_OF(list->next, MDS_WorkNode_t, node);
            WORKQ_RemoveWorkNode(workn);
            MDS_Tick_t ticknext = workn->tickout - MDS_ClockGetTickCount();
            WORKQ_InsertWorkNode(workq, workn, ticknext);
        }

        slot = 0;
    }

    return (slot);
}

void MDS_WorkQueueCheck(MDS_WorkQueue_t *workq, MDS_Lock_t *lock)
{
    if ((workq->overList.next == NULL) || (workq->overList.prev == NULL)) {
        MDS_SpinLockInit(&(workq->spinlock));
        WORKQ_InitWorkQueue(workq);
        return;
    }

    MDS_DListNode_t runList = MDS_DLIST_INIT(runList);

    while ((MDS_ClockGetTickCount() - workq->ticklast) < MDS_CLOCK_TICK_TIMER_MAX) {
        size_t slot = MDS_WorkQueueWheel(workq);

        while (!MDS_DListIsEmpty(&(workq->wheelList[slot]))) {
            MDS_WorkNode_t *workn = CONTAINER_OF(workq->wheelList[slot].next, MDS_WorkNode_t, node);

            MDS_Tick_t tickcurr = MDS_ClockGetTickCount();
            if ((tickcurr - workn->tickout) >= MDS_CLOCK_TICK_TIMER_MAX) {
                break;
            }

            WORKQ_RemoveWorkNode(workn);
            MDS_DListInsertNodePrev(&runList, &(workn->node));

            MDS_HOOK_CALL(KERNEL, timer, (workn, MDS_KERNEL_TRACE_TIMER_ENTER));
            if (workn->entry != NULL) {
                MDS_CriticalRestore(&(workq->spinlock), *lock);
                workn->entry(workn, workn->arg);
                *lock = MDS_CriticalLock(&(workq->spinlock));
            }
            MDS_HOOK_CALL(KERNEL, timer, (workn, MDS_KERNEL_TRACE_TIMER_EXIT));

            if (MDS_DListIsEmpty(&runList)) {
                continue;
            }

            MDS_DListRemoveNode(&(workn->node));
            if ((workn->tperiod > MDS_CLOCK_TICK_NO_WAIT) &&
                (workn->tperiod < MDS_CLOCK_TICK_TIMER_MAX)) {
                WORKQ_InsertWorkNode(workq, workn, workn->tperiod);
            }
        }
    }
}
#endif

static MDS_Tick_t WORKQ_WorkQueueNextTick(MDS_WorkQueue_t *workq)
{
    MDS_Tick_t ticknext = MDS_CLOCK_TICK_FOREVER;

    MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));

    MDS_WorkNode_t *workn = WORKQ_PeekWorkNode(workq);
    if (workn != NULL) {
        ticknext = workn->tickout;
    }

    MDS_CriticalRestore(&(workq->spinlock), lock);

    return (ticknext);
}

MDS_Err_t MDS_TimerInit(MDS_Timer_t *timer, const char *name, MDS_TimerEntry_t entry,
                        MDS_TimerEntry_t stop, MDS_Arg_t arg)
{
    MDS_ASSERT(timer != NULL);

    if (timer->queue != NULL) {
        return (MDS_EAGAIN);
    }

    MDS_Err_t err = MDS_ObjectInit(&(timer->object), MDS_OBJECT_TYPE_WORKNODE, name);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        WORKQ_InitWorkNode(timer);
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

    MDS_Err_t err = MDS_WorkNodeCancle(timer);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_ObjectDeInit(&(timer->object));
    }

    return (err);
}

MDS_Timer_t *MDS_TimerCreate(const char *name, MDS_WorkEntry_t entry, MDS_WorkEntry_t stop,
                             MDS_Arg_t arg)
{
    MDS_Timer_t *timer =
        (MDS_Timer_t *)MDS_ObjectCreate(sizeof(MDS_Timer_t), MDS_OBJECT_TYPE_WORKNODE, name);
    if (timer != NULL) {
        WORKQ_InitWorkNode(timer);
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

    MDS_Err_t err = MDS_WorkNodeCancle(timer);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_ObjectDestroy(&(timer->object));
    }

    return (err);
}

MDS_Err_t MDS_TimerStart(MDS_Timer_t *timer, MDS_Timeout_t duration, MDS_Timeout_t period)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(timer->object)) == MDS_OBJECT_TYPE_WORKNODE);

    MDS_WorkQueue_t *workq = &g_sysTimerQueue;

    if (duration.ticks == MDS_CLOCK_TICK_FOREVER) {
        return (MDS_EOK);
    } else if (duration.ticks >= MDS_CLOCK_TICK_TIMER_MAX) {
        return (MDS_EINVAL);
    } else if (duration.ticks == MDS_CLOCK_TICK_NO_WAIT) {
        duration.ticks = 1;
    }

    MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));

    timer->tperiod = period.ticks;
    WORKQ_InsertWorkNode(workq, timer, duration.ticks);
    timer->queue = workq;

    MDS_CriticalRestore(&(workq->spinlock), lock);

    return (MDS_EOK);
}

MDS_Err_t MDS_TimerStop(MDS_Timer_t *timer)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(timer->object)) == MDS_OBJECT_TYPE_WORKNODE);

    MDS_WorkQueue_t *workq = &g_sysTimerQueue;

    if (timer->queue == NULL) {
        return (MDS_EAGAIN);
    }

    MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));

    timer->queue = NULL;
    WORKQ_RemoveWorkNode(timer);

    MDS_CriticalRestore(&(workq->spinlock), lock);

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
    MDS_WorkQueue_t *workq = &g_sysTimerQueue;

    MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));

    MDS_WorkQueueCheck(workq, &lock);

    MDS_CriticalRestore(&(workq->spinlock), lock);
#endif
}

MDS_Tick_t MDS_SysTimerNextTick(void)
{
#if (defined(CONFIG_MDS_TIMER_INDEPENDENT) && (CONFIG_MDS_TIMER_INDEPENDENT != 0))
    return (WORKQ_WorkQueueNextTick(&g_sysTimerQueue));
#else
    return (MDS_CLOCK_TICK_FOREVER);
#endif
}

/* Semaphore --------------------------------------------------------------- */
MDS_Err_t MDS_SemaphoreInit(MDS_Semaphore_t *semaphore, const char *name, size_t init, size_t max)
{
    MDS_ASSERT(semaphore != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(semaphore->object), MDS_OBJECT_TYPE_SEMAPHORE, name);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
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
    MDS_Semaphore_t *semaphore = (MDS_Semaphore_t *)MDS_ObjectCreate(
        sizeof(MDS_Semaphore_t), MDS_OBJECT_TYPE_SEMAPHORE, name);
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

    MDS_Err_t err = MDS_ETIME;
    MDS_Tick_t tickstart = MDS_ClockGetTickCount();

    for (;;) {
        MDS_Lock_t lock = MDS_CriticalLock(&(semaphore->spinlock));
        if (semaphore->value > 0) {
            semaphore->value -= 1;
            err = MDS_EOK;
        }
        MDS_CriticalRestore(&(semaphore->spinlock), lock);

        if ((MDS_ErrIsSame(err, MDS_EOK)) ||
            ((MDS_ClockGetTickCount() - tickstart) >= timeout.ticks)) {
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

    MDS_Lock_t lock = MDS_CriticalLock(&(semaphore->spinlock));
    if (semaphore->value < semaphore->max) {
        semaphore->value += 1;
    } else {
        err = MDS_ERANGE;
    }
    MDS_CriticalRestore(&(semaphore->spinlock), lock);

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
    return (MDS_SemaphoreInit(condition, name, 0, -1));
}

MDS_Err_t MDS_ConditionDeInit(MDS_Condition_t *condition)
{
    return (MDS_SemaphoreDeInit(condition));
}

MDS_Err_t MDS_ConditionBroadCast(MDS_Condition_t *condition)
{
    MDS_ASSERT(condition != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(condition->object)) == MDS_OBJECT_TYPE_SEMAPHORE);

    MDS_Err_t err;
    do {
        err = MDS_SemaphoreAcquire(condition, MDS_TIMEOUT_NOWAIT);
        if (MDS_ErrIsSame(err, MDS_ETIME) || (MDS_ErrIsSame(err, MDS_EOK))) {
            MDS_SemaphoreRelease(condition);
        }
    } while (MDS_ErrIsSame(err, MDS_ETIME));

    return (err);
}

MDS_Err_t MDS_ConditionWait(MDS_Condition_t *condition, MDS_Mutex_t *mutex, MDS_Timeout_t timeout)
{
    MDS_ASSERT(condition != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(condition->object)) == MDS_OBJECT_TYPE_SEMAPHORE);
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    MDS_Err_t err = MDS_EOK;

    MDS_Lock_t lock = MDS_CriticalLock(&(condition->spinlock));

    if (condition->value > 0) {
        condition->value -= 1;
    } else if (timeout.ticks == MDS_CLOCK_TICK_NO_WAIT) {
        err = MDS_ETIME;
    } else if (timeout.ticks < MDS_CLOCK_TICK_TIMER_MAX) {
        MDS_Tick_t tickstart = MDS_ClockGetTickCount();
        for (;;) {
            if ((MDS_ClockGetTickCount() - tickstart) >= timeout.ticks) {
                err = MDS_ETIME;
                break;
            }
        }

        MDS_MutexRelease(mutex);

        MDS_CriticalRestore(&(condition->spinlock), lock);

        MDS_MutexAcquire(mutex, MDS_TIMEOUT_FOREVER);
    } else {
        MDS_CriticalRestore(&(condition->spinlock), lock);

        err = MDS_EINVAL;
    }

    return (err);
}

/* Mutex ------------------------------------------------------------------- */
MDS_Err_t MDS_MutexInit(MDS_Mutex_t *mutex, const char *name)
{
    MDS_ASSERT(mutex != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(mutex->object), MDS_OBJECT_TYPE_MUTEX, name);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
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
    MDS_Mutex_t *mutex =
        (MDS_Mutex_t *)MDS_ObjectCreate(sizeof(MDS_Mutex_t), MDS_OBJECT_TYPE_MUTEX, name);
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

    MDS_Err_t err = MDS_ETIME;
    MDS_Tick_t tickstart = MDS_ClockGetTickCount();

    for (;;) {
        MDS_Lock_t lock = MDS_CriticalLock(&(mutex->spinlock));
        if (mutex->value > 0) {
            mutex->value = 0;
            err = MDS_EOK;
        }
        MDS_CriticalRestore(&(mutex->spinlock), lock);

        if ((MDS_ErrIsSame(err, MDS_EOK)) ||
            ((MDS_ClockGetTickCount() - tickstart) >= timeout.ticks)) {
            break;
        }
    }

    return (MDS_EOK);
}

MDS_Err_t MDS_MutexRelease(MDS_Mutex_t *mutex)
{
    MDS_ASSERT(mutex != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(mutex->object)) == MDS_OBJECT_TYPE_MUTEX);

    MDS_Lock_t lock = MDS_CriticalLock(&(mutex->spinlock));
    mutex->value = 1;
    MDS_CriticalRestore(&(mutex->spinlock), lock);

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
    if (MDS_ErrIsSame(err, MDS_EOK)) {
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
    MDS_Event_t *event =
        (MDS_Event_t *)MDS_ObjectCreate(sizeof(MDS_Event_t), MDS_OBJECT_TYPE_EVENT, name);
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

MDS_Err_t MDS_EventWait(MDS_Event_t *event, MDS_Mask_t wait, MDS_EventOpt_t opt, MDS_Mask_t *recv,
                        MDS_Timeout_t timeout)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);
    MDS_ASSERT((opt & (MDS_EVENT_OPT_AND | MDS_EVENT_OPT_OR)) !=
               (MDS_EVENT_OPT_AND | MDS_EVENT_OPT_OR));

    if ((wait.mask == 0U) || ((opt & (MDS_EVENT_OPT_AND | MDS_EVENT_OPT_OR)) == 0U)) {
        return (MDS_EINVAL);
    }

    MDS_Err_t err = MDS_ETIME;
    MDS_Tick_t tickstart = MDS_ClockGetTickCount();

    for (;;) {
        MDS_Lock_t lock = MDS_CriticalLock(&(event->spinlock));
        if ((((opt & MDS_EVENT_OPT_AND) != 0U) && ((event->value.mask & wait.mask) == wait.mask)) ||
            (((opt & MDS_EVENT_OPT_OR) != 0U) && ((event->value.mask & wait.mask) != 0U))) {
            if (recv != NULL) {
                *recv = event->value;
            }
            if ((opt & MDS_EVENT_OPT_NOCLR) == 0U) {
                event->value.mask &= (~wait.mask);
            }
            err = MDS_EOK;
        }
        MDS_CriticalRestore(&(event->spinlock), lock);

        if ((MDS_ErrIsSame(err, MDS_EOK)) ||
            ((MDS_ClockGetTickCount() - tickstart) >= timeout.ticks)) {
            break;
        }
    }

    return (err);
}

MDS_Err_t MDS_EventSet(MDS_Event_t *event, MDS_Mask_t set)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    MDS_Lock_t lock = MDS_CriticalLock(&(event->spinlock));
    event->value.mask |= set.mask;
    MDS_CriticalRestore(&(event->spinlock), lock);

    return (MDS_EOK);
}

MDS_Err_t MDS_EventClr(MDS_Event_t *event, MDS_Mask_t clr)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    MDS_Lock_t lock = MDS_CriticalLock(&(event->spinlock));
    event->value.mask &= (~clr.mask);
    MDS_CriticalRestore(&(event->spinlock), lock);

    return (MDS_EOK);
}

MDS_Mask_t MDS_EventGetValue(const MDS_Event_t *event)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    return (event->value);
}

/* Thread ------------------------------------------------------------------ */
MDS_Err_t MDS_ThreadDelay(MDS_Timeout_t delay)
{
    MDS_ClockDelayRaw(delay);

    return (MDS_EOK);
}

/* Kernel ------------------------------------------------------------------ */
static volatile int g_sysCriticalNest = 0;

__attribute__((weak)) void MDS_CoreIdleSleep(void)
{
}

MDS_Thread_t *MDS_KernelCurrentThread(void)
{
    return (NULL);
}

void MDS_KernelSchdulerLockAcquire(void)
{
    MDS_Lock_t lock = MDS_CriticalLock(NULL);

    g_sysCriticalNest += 1;

    MDS_CriticalRestore(NULL, lock);
}

void MDS_KernelSchdulerLockRelease(void)
{
    MDS_Lock_t lock = MDS_CriticalLock(NULL);

    g_sysCriticalNest -= 1;

    MDS_CriticalRestore(NULL, lock);
}

size_t MDS_KernelSchdulerLockLevel(void)
{
    return (g_sysCriticalNest);
}

MDS_Tick_t MDS_KernelGetSleepTick(void)
{
    MDS_Tick_t sleepTick = 0;

    MDS_Lock_t lock = MDS_CriticalLock(NULL);

    MDS_Tick_t nextTick = MDS_SysTimerNextTick();
    MDS_Tick_t currTick = MDS_ClockGetTickCount();
    sleepTick = (nextTick > currTick) ? (nextTick - currTick) : (0);

    MDS_CriticalRestore(NULL, lock);

    return (sleepTick);
}

void MDS_KernelCompensateTick(MDS_Tick_t ticks)
{
    MDS_Lock_t lock = MDS_CriticalLock(NULL);

    MDS_Tick_t currTick = MDS_ClockGetTickCount() + ticks;
    MDS_ClockIncTickCount(currTick);
    MDS_SysTimerCheck();

    MDS_CriticalRestore(NULL, lock);
}

/* Clock ------------------------------------------------------------------- */
static struct MDS_SysTick {
    volatile MDS_Tick_t tickcount;
    MDS_SpinLock_t spinlock;
} g_sysTick;

static struct MDS_UnixTime {
    int64_t ts;
    int8_t tz;
} g_unixTime;

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
