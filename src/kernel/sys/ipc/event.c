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
MDS_Err_t MDS_EventInit(MDS_Event_t *event, const char *name)
{
    MDS_ASSERT(event != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(event->object), MDS_OBJECT_TYPE_EVENT, name);
    if (err == MDS_EOK) {
        event->value.mask = 0U;
        MDS_KernelWaitQueueInit(&(event->queueWait));
        MDS_SpinLockInit(&(event->spinlock));
    }

    return (err);
}

MDS_Err_t MDS_EventDeInit(MDS_Event_t *event)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    MDS_Lock_t lock = MDS_CriticalLock(&(event->spinlock));

    MDS_KernelWaitQueueDrain(&(event->queueWait));
    MDS_Err_t err = MDS_ObjectDeInit(&(event->object));

    MDS_CriticalRestore((err != MDS_EOK) ? (&(event->spinlock)) : (NULL), lock);

    return (err);
}

MDS_Event_t *MDS_EventCreate(const char *name)
{
    MDS_Event_t *event = (MDS_Event_t *)MDS_ObjectCreate(sizeof(MDS_Event_t),
                                                         MDS_OBJECT_TYPE_EVENT, name);
    if (event != NULL) {
        event->value.mask = 0U;
        MDS_KernelWaitQueueInit(&(event->queueWait));
        MDS_SpinLockInit(&(event->spinlock));
    }

    return (event);
}

MDS_Err_t MDS_EventDestroy(MDS_Event_t *event)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    MDS_Lock_t lock = MDS_CriticalLock(&(event->spinlock));

    MDS_KernelWaitQueueDrain(&(event->queueWait));
    MDS_Err_t err = MDS_ObjectDestroy(&(event->object));

    MDS_CriticalRestore((err != MDS_EOK) ? (&(event->spinlock)) : (NULL), lock);

    return (err);
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

    MDS_Err_t err = MDS_EOK;
    bool reSchedule = false;
    MDS_Thread_t *thread = MDS_KernelCurrentThread();

    MDS_LOG_D("[event] thread(%p) wait event(%p) which value:%zx mask:%zx opt:%x", thread, event,
              event->value.mask, wait.mask, opt);

    MDS_HOOK_CALL(KERNEL, event, (event, MDS_KERNEL_TRACE_EVENT_TRY_ACQUIRE, err, timeout));

    MDS_Lock_t lock = MDS_CriticalLock(&(event->spinlock));

    if ((((opt & MDS_EVENT_OPT_AND) != 0U) && ((event->value.mask & wait.mask) == wait.mask)) ||
        (((opt & MDS_EVENT_OPT_OR) != 0U) && ((event->value.mask & wait.mask) != 0U))) {
        if (recv != NULL) {
            *recv = thread->eventMask;
        }
        thread->eventMask.mask = event->value.mask & wait.mask;
        thread->eventOpt = opt;
        if ((opt & MDS_EVENT_OPT_NOCLR) == 0U) {
            event->value.mask &= ~(wait.mask);
        }
    } else if (timeout.ticks == MDS_CLOCK_TICK_NO_WAIT) {
        err = thread->err = MDS_ETIMEOUT;
    } else if ((timeout.ticks < MDS_CLOCK_TICK_TIMER_MAX) && (thread != NULL)) {
        thread->eventMask.mask = wait.mask;
        thread->eventOpt = opt;
        err = MDS_KernelWaitQueueSuspend(&(event->queueWait), thread, timeout, true);
        if (err == MDS_EOK) {
            reSchedule = true;
        }
    } else {
        err = MDS_EINVAL;
    }

    MDS_CriticalRestore(&(event->spinlock), lock);

    MDS_HOOK_CALL(KERNEL, event, (event, MDS_KERNEL_TRACE_EVENT_HAS_ACQUIRE, err, timeout));

    if (reSchedule) {
        MDS_KernelSchedulerCheck();
        err = thread->err;
        if (recv != NULL) {
            *recv = thread->eventMask;
        }
    }

    return (err);
}

MDS_Err_t MDS_EventSet(MDS_Event_t *event, MDS_Mask_t set)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    MDS_Err_t err = MDS_EOK;
    bool reSchedule = false;

    MDS_LOG_D("[event] event(%p) which value:%zx set mask:%zx", event, event->value.mask,
              set.mask);

    MDS_HOOK_CALL(KERNEL, event,
                  (event, MDS_KERNEL_TRACE_EVENT_HAS_SET, err, MDS_TIMEOUT_TICKS(mask)));

    MDS_Lock_t lock = MDS_CriticalLock(&(event->spinlock));

    event->value.mask |= set.mask;
    MDS_Thread_t *iter = NULL;
    MDS_LIST_FOREACH_NEXT (iter, nodeWait.node, &(event->queueWait.list)) {
        err = MDS_EINVAL;
        if ((iter->eventOpt & MDS_EVENT_OPT_AND) != 0U) {
            if ((iter->eventMask.mask & event->value.mask) == iter->eventMask.mask) {
                err = MDS_EOK;
            }
        } else if ((iter->eventOpt & MDS_EVENT_OPT_OR) != 0U) {
            if ((iter->eventMask.mask & event->value.mask) != 0U) {
                iter->eventMask.mask &= event->value.mask;
                err = MDS_EOK;
            }
        } else {
            break;
        }
        if (err == MDS_EOK) {
            if ((iter->eventOpt & MDS_EVENT_OPT_NOCLR) == 0U) {
                event->value.mask &= ~iter->eventMask.mask;
            }
            MDS_KernelWaitQueueResume(&(event->queueWait));
            reSchedule = true;
            break;
        }
    }

    MDS_CriticalRestore(&(event->spinlock), lock);

    if (reSchedule) {
        MDS_KernelSchedulerCheck();
    }

    return (err);
}

MDS_Err_t MDS_EventClr(MDS_Event_t *event, MDS_Mask_t clr)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    MDS_Err_t err = MDS_EOK;

    MDS_LOG_D("[event] event(%p) which value:%zx clr mask:%zx", event, event->value.mask,
              clr.mask);

    MDS_HOOK_CALL(KERNEL, event,
                  (event, MDS_KERNEL_TRACE_EVENT_HAS_CLR, err, (MDS_Timeout_t) {.ticks = mask}));

    MDS_Lock_t lock = MDS_CriticalLock(&(event->spinlock));
    event->value.mask &= ~(clr.mask);
    MDS_CriticalRestore(&(event->spinlock), lock);

    return (err);
}

MDS_Mask_t MDS_EventGetValue(const MDS_Event_t *event)
{
    MDS_ASSERT(event != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(event->object)) == MDS_OBJECT_TYPE_EVENT);

    return (event->value);
}
