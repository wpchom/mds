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

/* Define ------------------------------------------------------------------ */
MDS_LOG_MODULE_DECLARE(kernel, CONFIG_MDS_KERNEL_LOG_LEVEL);

/* WorkQueue --------------------------------------------------------------- */
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

static void WORKQ_DrainWorkQueue(MDS_WorkQueue_t *workq, MDS_Lock_t *lock)
{
    if ((workq->list[0].next == NULL) || (workq->list[0].prev == NULL)) {
        return;
    }

    MDS_DListNode_t stopList = MDS_DLIST_INIT(stopList);

    while (!MDS_DListIsEmpty(&(workq->list[ARRAY_SIZE(workq->list) - 1]))) {
        MDS_WorkNode_t *workn = WORKQ_PeekWorkNode(workq);

        WORKQ_RemoveWorkNode(workn);
        MDS_DListInsertNodeNext(&stopList, &(workn->node[ARRAY_SIZE(workn->node) - 1]));

        MDS_HOOK_CALL(KERNEL, timer, (workn, MDS_KERNEL_TRACE_TIMER_STOP));
        if (workn->stop != NULL) {
            MDS_CriticalRestore(&(workq->spinlock), *lock);
            workn->stop(workn, workn->arg);
            *lock = MDS_CriticalLock(&(workq->spinlock));
        }

        if (MDS_DListIsEmpty(&stopList)) {
            continue;
        }

        MDS_DListRemoveNode(&(workn->node[ARRAY_SIZE(workn->node) - 1]));
    }
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

static void WORKQ_WorkListStop(MDS_DListNode_t *list, MDS_SpinLock_t *spinlock, MDS_Lock_t *lock)
{
    MDS_DListNode_t stopList = MDS_DLIST_INIT(stopList);

    while (!MDS_DListIsEmpty(list)) {
        MDS_WorkNode_t *workn = CONTAINER_OF(list->next, MDS_WorkNode_t, node);

        MDS_HOOK_CALL(KERNEL, timer, (workn, MDS_KERNEL_TRACE_TIMER_STOP));

        WORKQ_RemoveWorkNode(workn);
        MDS_DListInsertNodeNext(&stopList, &(workn->node));

        if (workn->stop != NULL) {
            MDS_CriticalRestore(spinlock, *lock);
            workn->stop(workn, workn->arg);
            *lock = MDS_CriticalLock(spinlock);
        }

        if (MDS_DListIsEmpty(&stopList)) {
            continue;
        }

        MDS_DListRemoveNode(&(workn->node));
    }
}

static void WORKQ_DrainWorkQueue(MDS_WorkQueue_t *workq, MDS_Lock_t *lock)
{
    if ((workq->overList.next == NULL) || (workq->overList.prev == NULL)) {
        return;
    }

    for (size_t idx = 0; idx < ARRAY_SIZE(workq->wheelList); idx++) {
        WORKQ_WorkListStop(&(workq->wheelList[idx]), &(workq->spinlock), lock);
    }

    WORKQ_WorkListStop(&(workq->overList), &(workq->spinlock), lock);
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

static void WORKQ_ThreadEntry(MDS_Arg_t arg)
{
    MDS_WorkQueue_t *workq = (MDS_WorkQueue_t *)(arg.ptr);

    for (;;) {
        MDS_Tick_t nextTick = 0;

        MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));
        do {
            MDS_WorkQueueCheck(workq, &lock);

            MDS_WorkNode_t *workn = WORKQ_PeekWorkNode(workq);
            nextTick = (workn != NULL) ? (workn->tickout) : (MDS_CLOCK_TICK_FOREVER);

            MDS_LOG_D("[workq] thread take next tick check:%" PRIdTICK, nextTick);

            if (nextTick == MDS_CLOCK_TICK_FOREVER) {
                MDS_ThreadSuspend(workq->thread);
                break;
            }

            MDS_Tick_t currTick = MDS_ClockGetTickCount();
            if ((nextTick - currTick) < MDS_CLOCK_TICK_TIMER_MAX) {
                nextTick = nextTick - currTick;
                break;
            }
        } while (0);
        MDS_CriticalRestore(&(workq->spinlock), lock);

        if (nextTick == MDS_CLOCK_TICK_FOREVER) {
            MDS_KernelSchedulerCheck();
        } else if (nextTick > 0) {
            MDS_ThreadDelay(MDS_TIMEOUT_TICKS(nextTick));
        }
    }
}

MDS_Err_t MDS_WorkQueueInit(MDS_WorkQueue_t *workq, const char *name, MDS_Thread_t *thread,
                            void *stackPool, size_t stackSize, MDS_ThreadPriority_t priority,
                            MDS_Timeout_t timeslice)
{
    MDS_ASSERT(workq != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(workq->object), MDS_OBJECT_TYPE_WORKQUEUE, name);

    do {
        if (!MDS_ErrIsSame(err, MDS_EOK)) {
            break;
        }

        MDS_SpinLockInit(&(workq->spinlock));
        WORKQ_InitWorkQueue(workq);
        workq->thread = thread;

        if ((thread != NULL) && (stackPool != NULL) && (stackSize > 0)) {
            err = MDS_ThreadInit(thread, name, WORKQ_ThreadEntry, MDS_ARG_WITH(workq), stackPool,
                                 stackSize, priority, timeslice);
            if (!MDS_ErrIsSame(err, MDS_EOK)) {
                MDS_ObjectDeInit(&(workq->object));
            }
        }
    } while (0);

    return (err);
}

MDS_Err_t MDS_WorkQueueDeInit(MDS_WorkQueue_t *workq)
{
    MDS_ASSERT(workq != NULL);

    MDS_Err_t err = MDS_EOK;

    MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));

    WORKQ_DrainWorkQueue(workq, &lock);

    if (workq->thread != NULL) {
        err = MDS_ThreadDeInit(workq->thread);
    }

    if (MDS_ErrIsSame(err, MDS_EOK)) {
        workq->thread = NULL;
        err = MDS_ObjectDeInit(&(workq->object));
        MDS_CriticalRestore(NULL, lock);
    } else {
        MDS_CriticalRestore(&(workq->spinlock), lock);
    }

    return (err);
}

MDS_WorkQueue_t *MDS_WorkQueueCreate(const char *name, size_t stackSize,
                                     MDS_ThreadPriority_t priority, MDS_Timeout_t timeslice)
{
    MDS_WorkQueue_t *workq = (MDS_WorkQueue_t *)MDS_ObjectCreate(sizeof(MDS_WorkQueue_t),
                                                                 MDS_OBJECT_TYPE_WORKQUEUE, name);
    if (workq == NULL) {
        return (NULL);
    }

    MDS_Thread_t *thread = NULL;
    if (stackSize > 0) {
        thread = MDS_ThreadCreate(name, WORKQ_ThreadEntry, MDS_ARG_WITH(workq), stackSize, priority,
                                  timeslice);
        if (thread == NULL) {
            MDS_ObjectDestroy(&(workq->object));
            workq = NULL;
        }
    }

    if (workq != NULL) {
        MDS_SpinLockInit(&(workq->spinlock));
        WORKQ_InitWorkQueue(workq);
        workq->thread = thread;
    }

    return (workq);
}

MDS_Err_t MDS_WorkQueueDestroy(MDS_WorkQueue_t *workq)
{
    MDS_ASSERT(workq != NULL);

    MDS_Err_t err = MDS_EOK;

    MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));

    WORKQ_DrainWorkQueue(workq, &lock);

    if (workq->thread != NULL) {
        err = MDS_ThreadDestroy(workq->thread);
    }

    if (MDS_ErrIsSame(err, MDS_EOK)) {
        workq->thread = NULL;
        err = MDS_ObjectDestroy(&(workq->object));
        MDS_CriticalRestore(NULL, lock);
    } else {
        MDS_CriticalRestore(&(workq->spinlock), lock);
    }

    return (err);
}

MDS_Err_t MDS_WorkQueueStart(MDS_WorkQueue_t *workq)
{
    MDS_ASSERT(workq != NULL);

    MDS_Err_t err = MDS_ENOENT;

    MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));

    if (workq->thread != NULL) {
        MDS_ThreadState_t state = MDS_ThreadGetState(workq->thread);
        if (state == MDS_THREAD_STATE_INACTIVED) {
            err = MDS_ThreadStartup(workq->thread);
        } else {
            err = MDS_ThreadResume(workq->thread);
        }
    }

    MDS_CriticalRestore(&(workq->spinlock), lock);

    MDS_KernelSchedulerCheck();

    return (err);
}

MDS_Err_t MDS_WorkQueueStop(MDS_WorkQueue_t *workq)
{
    MDS_ASSERT(workq != NULL);

    MDS_Err_t err = MDS_ENOENT;

    MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));

    if (workq->thread != NULL) {
        err = MDS_ThreadSuspend(workq->thread);
        if (MDS_ErrIsSame(err, MDS_EOK)) {
            WORKQ_DrainWorkQueue(workq, &lock);
        }
    }

    MDS_CriticalRestore(&(workq->spinlock), lock);

    return (err);
}

MDS_Tick_t MDS_WorkQueueNextTick(MDS_WorkQueue_t *workq)
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

/* Work -------------------------------------------------------------------- */
MDS_Err_t MDS_WorkNodeInit(MDS_WorkNode_t *workn, const char *name, MDS_WorkEntry_t entry,
                           MDS_WorkEntry_t stop, MDS_Arg_t arg)
{
    MDS_ASSERT(workn != NULL);

    if (workn->queue != NULL) {
        return (MDS_EAGAIN);
    }

    MDS_Err_t err = MDS_ObjectInit(&(workn->object), MDS_OBJECT_TYPE_WORKNODE, name);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        WORKQ_InitWorkNode(workn);
        workn->entry = entry;
        workn->stop = stop;
        workn->arg = arg;
    }

    return (err);
}

MDS_Err_t MDS_WorkNodeDeInit(MDS_WorkNode_t *workn)
{
    MDS_ASSERT(workn != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(workn->object)) == MDS_OBJECT_TYPE_WORKNODE);

    MDS_Err_t err = MDS_WorkNodeCancle(workn);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_ObjectDeInit(&(workn->object));
    }

    return (err);
}

MDS_WorkNode_t *MDS_WorkNodeCreate(const char *name, MDS_WorkEntry_t entry, MDS_WorkEntry_t stop,
                                   MDS_Arg_t arg)
{
    MDS_WorkNode_t *workn =
        (MDS_WorkNode_t *)MDS_ObjectCreate(sizeof(MDS_WorkNode_t), MDS_OBJECT_TYPE_WORKNODE, name);
    if (workn != NULL) {
        WORKQ_InitWorkNode(workn);
        workn->entry = entry;
        workn->stop = stop;
        workn->arg = arg;
    }

    return (workn);
}

MDS_Err_t MDS_WorkNodeDestroy(MDS_WorkNode_t *workn)
{
    MDS_ASSERT(workn != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(workn->object)) == MDS_OBJECT_TYPE_WORKNODE);

    MDS_Err_t err = MDS_WorkNodeCancle(workn);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_ObjectDestroy(&(workn->object));
    }

    return (err);
}

MDS_Err_t MDS_WorkNodeSubmit(MDS_WorkQueue_t *workq, MDS_WorkNode_t *workn, MDS_Timeout_t duration,
                             MDS_Timeout_t period)
{
    MDS_ASSERT(workq != NULL);
    MDS_ASSERT(workn != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(workn->object)) == MDS_OBJECT_TYPE_WORKNODE);

    if (duration.ticks == MDS_CLOCK_TICK_FOREVER) {
        return (MDS_EOK);
    } else if (duration.ticks >= MDS_CLOCK_TICK_TIMER_MAX) {
        return (MDS_EINVAL);
    } else if (duration.ticks == MDS_CLOCK_TICK_NO_WAIT) {
        duration.ticks = 1;
    }

    MDS_Err_t err = MDS_EACCES;

    MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));

    workn->tperiod = period.ticks;
    WORKQ_InsertWorkNode(workq, workn, duration.ticks);
    workn->queue = workq;

    if ((workq->thread != NULL) && (workq->thread != MDS_KernelCurrentThread()) &&
        (MDS_ThreadGetState(workq->thread) == MDS_THREAD_STATE_SUSPENDED)) {
        err = MDS_ThreadResume(workq->thread);
    }

    MDS_CriticalRestore(&(workq->spinlock), lock);

    if (MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_KernelSchedulerCheck();
    }

    return (err);
}

MDS_Err_t MDS_WorkNodeCancle(MDS_WorkNode_t *workn)
{
    MDS_ASSERT(workn != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(workn->object)) == MDS_OBJECT_TYPE_WORKNODE);

    MDS_WorkQueue_t *workq = workn->queue;
    if (workq == NULL) {
        return (MDS_EAGAIN);
    }

    MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));

    workn->queue = NULL;
    WORKQ_RemoveWorkNode(workn);

    MDS_CriticalRestore(&(workq->spinlock), lock);

    if (workn->stop != NULL) {
        workn->stop(workn, workn->arg);
    }

    return (MDS_EOK);
}

bool MDS_WorkNodeIsSumbit(MDS_WorkNode_t *workn)
{
    MDS_ASSERT(workn != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(workn->object)) == MDS_OBJECT_TYPE_WORKNODE);

    return (workn->queue != NULL);
}
