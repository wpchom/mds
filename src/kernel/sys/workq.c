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
static int WORKQ_SkipListCompare(const MDS_DListNode_t *node, const void *value)
{
    const MDS_WorkNode_t *workl = CONTAINER_OF(node, MDS_WorkNode_t, node);
    const MDS_WorkNode_t *workn = (const MDS_WorkNode_t *)value;

    MDS_Tick_t tickdiff = workl->tickout - workn->tickout;
    if ((tickdiff > 0) && (tickdiff < MDS_CLOCK_TICK_TIMER_MAX)) {
        return (1);  // > 0 to break;
    } else {
        return (-1);  // < 0 to continue;
    }
}

static MDS_WorkNode_t *WORKQ_SkipListPeek(const MDS_WorkQueue_t *workq)
{
    MDS_WorkNode_t *workn = NULL;

    if (!MDS_DListIsEmpty(&(workq->list[ARRAY_SIZE(workq->list) - 1]))) {
        workn = CONTAINER_OF(workq->list[ARRAY_SIZE(workq->list) - 1].next, MDS_WorkNode_t,
                             node[ARRAY_SIZE(workq->list) - 1]);
    }

    return (workn);
}

static void WORKQ_SkipListRemove(MDS_WorkNode_t *workn)
{
    MDS_SkipListRemoveNode(workn->node, ARRAY_SIZE(workn->node));
}

static void WORKQ_SkipListInsert(MDS_WorkQueue_t *workq, MDS_WorkNode_t *workn, MDS_Tick_t tickout)
{
    static size_t skipRand = 0;

    MDS_Tick_t tickcurr = MDS_ClockGetTickCount();
    MDS_DListNode_t *skipNode[ARRAY_SIZE(workq->list)];

    workn->tickout = tickcurr + tickout;
    MDS_SkipListSearchNode(skipNode, workq->list, ARRAY_SIZE(workq->list), workn,
                           WORKQ_SkipListCompare);

    skipRand = skipRand + tickcurr + 1;
    MDS_SkipListInsertNode(skipNode, workn->node, ARRAY_SIZE(workn->node), skipRand,
                           CONFIG_MDS_TIMER_SKIPLIST_SHIFT);
}

static void WORKQ_SkipListDrain(MDS_WorkQueue_t *workq, MDS_Lock_t *lock)
{
    if ((workq->list[0].next == NULL) || (workq->list[0].prev == NULL)) {
        return;
    }

    MDS_DListNode_t stopList = MDS_DLIST_INIT(stopList);

    while (!MDS_DListIsEmpty(&(workq->list[ARRAY_SIZE(workq->list) - 1]))) {
        MDS_WorkNode_t *workn = WORKQ_SkipListPeek(workq);

        MDS_HOOK_CALL(KERNEL, timer, (workn, MDS_KERNEL_TRACE_TIMER_STOP));

        WORKQ_SkipListRemove(workn);
        MDS_DListInsertNodeNext(&stopList, &(workn->node[ARRAY_SIZE(workn->node) - 1]));

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
        MDS_SkipListInitNode(workq->list, ARRAY_SIZE(workq->list));
        return;
    }

    MDS_DListNode_t runList = MDS_DLIST_INIT(runList);

    while (!MDS_DListIsEmpty(&(workq->list[ARRAY_SIZE(workq->list) - 1]))) {
        MDS_WorkNode_t *workn = WORKQ_SkipListPeek(workq);
        MDS_Tick_t tickcurr = MDS_ClockGetTickCount();

        if ((tickcurr - workn->tickout) >= MDS_CLOCK_TICK_TIMER_MAX) {
            break;
        }

        MDS_HOOK_CALL(KERNEL, timer, (workn, MDS_KERNEL_TRACE_TIMER_ENTER));

        WORKQ_SkipListRemove(workn);
        MDS_DListInsertNodeNext(&runList, &(workn->node[ARRAY_SIZE(workn->node) - 1]));

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
        if (workn->tperiod > MDS_CLOCK_TICK_NO_WAIT) {
            WORKQ_SkipListInsert(workq, workn, workn->tperiod);
        }
    }
}

static void WorkQueueThreadEntry(MDS_Arg_t *arg)
{
    MDS_WorkQueue_t *workq = (MDS_WorkQueue_t *)arg;

    for (;;) {
        MDS_Tick_t nextTick = 0;

        MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));
        do {
            MDS_WorkQueueCheck(workq, &lock);

            MDS_WorkNode_t *workn = WORKQ_SkipListPeek(workq);
            nextTick = (workn != NULL) ? (workn->tickout) : (MDS_CLOCK_TICK_FOREVER);

            MDS_LOG_D("[workq] thread take next tick check:%lu", (unsigned long)(nextTick));

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
                            MDS_Timeout_t timeout)
{
    MDS_ASSERT(workq != NULL);

    MDS_Err_t err = MDS_ThreadInit(thread, name, WorkQueueThreadEntry, (MDS_Arg_t *)workq,
                                   stackPool, stackSize, priority, timeout);
    if (err == MDS_EOK) {
        err = MDS_ObjectInit(&(workq->object), MDS_OBJECT_TYPE_WORKQUEUE, name);
        if (err == MDS_EOK) {
            MDS_SpinLockInit(&(workq->spinlock));
            MDS_SkipListInitNode(workq->list, ARRAY_SIZE(workq->list));
            workq->thread = thread;
        } else {
            MDS_ThreadDeInit(thread);
        }
    }

    return (err);
}

MDS_Err_t MDS_WorkQueueDeInit(MDS_WorkQueue_t *workq)
{
    MDS_ASSERT(workq != NULL);

    MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));

    WORKQ_SkipListDrain(workq, &lock);
    MDS_Err_t err = MDS_ThreadDeInit(workq->thread);
    if (err == MDS_EOK) {
        workq->thread = NULL;
        err = MDS_ObjectDeInit(&(workq->object));
    }

    MDS_CriticalRestore((err != MDS_EOK) ? (&(workq->spinlock)) : (NULL), lock);

    return (err);
}

MDS_WorkQueue_t *MDS_WorkQueueCreate(const char *name, size_t stackSize,
                                     MDS_ThreadPriority_t priority, MDS_Timeout_t timeout)
{
    MDS_WorkQueue_t *workq = (MDS_WorkQueue_t *)MDS_ObjectCreate(sizeof(MDS_WorkQueue_t),
                                                                 MDS_OBJECT_TYPE_WORKQUEUE, name);
    if (workq == NULL) {
        return (NULL);
    }

    workq->thread = MDS_ThreadCreate(name, WorkQueueThreadEntry, (MDS_Arg_t *)workq, stackSize,
                                     priority, timeout);
    if (workq->thread == NULL) {
        MDS_ObjectDestroy(&(workq->object));
        workq = NULL;
    } else {
        MDS_SpinLockInit(&(workq->spinlock));
        MDS_SkipListInitNode(workq->list, ARRAY_SIZE(workq->list));
    }

    return (workq);
}

MDS_Err_t MDS_WorkQueueDestroy(MDS_WorkQueue_t *workq)
{
    MDS_ASSERT(workq != NULL);

    MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));

    WORKQ_SkipListDrain(workq, &lock);
    MDS_Err_t err = MDS_ThreadDestroy(workq->thread);
    if (err == MDS_EOK) {
        workq->thread = NULL;
        err = MDS_ObjectDestroy(&(workq->object));
    }

    MDS_CriticalRestore((err != MDS_EOK) ? (&(workq->spinlock)) : (NULL), lock);

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
        if (err == MDS_EOK) {
            WORKQ_SkipListDrain(workq, &lock);
        }
    }

    MDS_CriticalRestore(&(workq->spinlock), lock);

    return (err);
}

MDS_Tick_t MDS_WorkQueueNextTick(MDS_WorkQueue_t *workq)
{
    MDS_Tick_t ticknext = MDS_CLOCK_TICK_FOREVER;

    MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));

    MDS_WorkNode_t *workn = WORKQ_SkipListPeek(workq);
    if (workn != NULL) {
        ticknext = workn->tickout;
    }

    MDS_CriticalRestore(&(workq->spinlock), lock);

    return (ticknext);
}

/* Work -------------------------------------------------------------------- */
MDS_Err_t MDS_WorkNodeInit(MDS_WorkNode_t *workn, const char *name, MDS_WorkEntry_t entry,
                           MDS_WorkEntry_t stop, MDS_Arg_t *arg)
{
    MDS_ASSERT(workn != NULL);

    if (workn->queue != NULL) {
        return (MDS_EAGAIN);
    }

    MDS_Err_t err = MDS_ObjectInit(&(workn->object), MDS_OBJECT_TYPE_WORKNODE, name);
    if (err == MDS_EOK) {
        MDS_SkipListInitNode(workn->node, ARRAY_SIZE(workn->node));
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
    if (err == MDS_EOK) {
        MDS_ObjectDeInit(&(workn->object));
    }

    return (err);
}

MDS_WorkNode_t *MDS_WorkNodeCreate(const char *name, MDS_WorkEntry_t entry, MDS_WorkEntry_t stop,
                                   MDS_Arg_t *arg)
{
    MDS_WorkNode_t *workn = (MDS_WorkNode_t *)MDS_ObjectCreate(sizeof(MDS_WorkNode_t),
                                                               MDS_OBJECT_TYPE_WORKNODE, name);
    if (workn != NULL) {
        MDS_SkipListInitNode(workn->node, ARRAY_SIZE(workn->node));
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
    if (err == MDS_EOK) {
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

    if ((duration.ticks >= MDS_CLOCK_TICK_TIMER_MAX) ||
        (period.ticks >= MDS_CLOCK_TICK_TIMER_MAX)) {
        return (MDS_EINVAL);
    }

    MDS_Err_t err = MDS_EACCES;

    MDS_Lock_t lock = MDS_CriticalLock(&(workq->spinlock));

    workn->tperiod = period.ticks;
    WORKQ_SkipListInsert(workq, workn, duration.ticks);
    workn->queue = workq;

    if ((workq->thread != NULL) && (workq->thread != MDS_KernelCurrentThread()) &&
        (MDS_ThreadGetState(workq->thread) == MDS_THREAD_STATE_SUSPENDED)) {
        err = MDS_ThreadResume(workq->thread);
    }

    MDS_CriticalRestore(&(workq->spinlock), lock);

    if (err == MDS_EOK) {
        MDS_KernelSchedulerCheck();
    }

    return (err);
}

MDS_Err_t MDS_WorkNodeCancle(MDS_WorkNode_t *workn)
{
    MDS_ASSERT(workn != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(workn->object)) == MDS_OBJECT_TYPE_WORKNODE);

    if (workn->queue == NULL) {
        return (MDS_EAGAIN);
    }

    workn->queue = NULL;
    WORKQ_SkipListRemove(workn);

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
