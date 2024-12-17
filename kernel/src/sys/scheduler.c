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
#include "mds_log.h"

/* MLFQ Scheduler ---------------------------------------------------------- */
#if (MDS_THREAD_PRIORITY_MAX > __WORDSIZE)
#error "kernel mlfq scheduler supported max priority same as wordsize 32-bit / 64-bit"
#endif

/* Variable ---------------------------------------------------------------- */
static volatile MDS_Mask_t g_sysThreadPrioMask = 0x00U;
static MDS_ListNode_t g_sysSchedulerTable[MDS_THREAD_PRIORITY_MAX];

/* Function ---------------------------------------------------------------- */
__attribute__((weak)) size_t MDS_SchedulerFFS(size_t value)
{
#if defined(__GNUC__)
    return (__builtin_ffs(value));
#else
    size_t num = 0;
#if __SIZE_MAX__ == __UINT64_MAX__
    if ((value & 0xFFFFFFFF) == 0) {
        num += 0x20;
        value >>= 0x20;
    }
#endif
    if ((value & 0xFFFF) == 0) {
        num += 0x10;
        value >>= 0x10;
    }
    if ((value & 0xFF) == 0) {
        num += 0x8;
        value >>= 0x8;
    }
    if ((value & 0xF) == 0) {
        num += 0x4;
        value >>= 0x4;
    }
    if ((value & 0x3) == 0) {
        num += 0x2;
        value >>= 0x2;
    }
    if ((value & 0x1) == 0) {
        num += 0x1;
    }
    return (num);

#endif
}

void MDS_SchedulerInit(void)
{
    MDS_SCHEDULER_DEBUG("scheduler init with max priority:%u", ARRAY_SIZE(g_sysSchedulerTable));

    g_sysThreadPrioMask = 0x00U;
    MDS_SkipListInitNode(g_sysSchedulerTable, ARRAY_SIZE(g_sysSchedulerTable));
}

void MDS_SchedulerInsertThread(MDS_Thread_t *thread)
{
    if (MDS_KernelCurrentThread() == thread) {
        thread->state = (thread->state & ~MDS_THREAD_STATE_MASK) | MDS_THREAD_STATE_RUNNING;
        return;
    }

    thread->state = (thread->state & ~MDS_THREAD_STATE_MASK) | MDS_THREAD_STATE_READY;

    MDS_ListRemoveNode(&(thread->node));

    if (thread->currPrio < MDS_THREAD_PRIORITY_MAX) {
        if ((thread->state & MDS_THREAD_FLAG_YIELD) != 0U) {
            MDS_ListInsertNodePrev(&(g_sysSchedulerTable[thread->currPrio]), &(thread->node));
        } else {
            MDS_ListInsertNodeNext(&(g_sysSchedulerTable[thread->currPrio]), &(thread->node));
        }
        g_sysThreadPrioMask |= (1UL << thread->currPrio);
    }

    MDS_SCHEDULER_DEBUG("scheduler insert thread(%p) entry:%p sp:%u priority:%u", thread, thread->entry,
                        thread->stackPoint, thread->currPrio);
}

void MDS_SchedulerRemoveThread(MDS_Thread_t *thread)
{
    MDS_SCHEDULER_DEBUG("scheduler remove thread(%p) entry:%p sp:%u priority:%u", thread, thread->entry,
                        thread->stackPoint, thread->currPrio);

    MDS_ListRemoveNode(&(thread->node));

    if ((thread->currPrio < MDS_THREAD_PRIORITY_MAX) &&
        (MDS_ListIsEmpty(&(g_sysSchedulerTable[thread->currPrio])))) {
        g_sysThreadPrioMask &= ~(1UL << thread->currPrio);
    }
}

MDS_Thread_t *MDS_SchedulerHighestPriorityThread(void)
{
    MDS_Thread_t *thread = NULL;
    size_t highestPrio = MDS_SchedulerFFS(g_sysThreadPrioMask);
    if (highestPrio != 0U) {
        thread = CONTAINER_OF(g_sysSchedulerTable[highestPrio - 1].next, MDS_Thread_t, node);
    }

    return (thread);
}
