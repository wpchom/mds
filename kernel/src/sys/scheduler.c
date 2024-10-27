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

/* MLFQ Scheduler ---------------------------------------------------------- */
#if (MDS_THREAD_PRIORITY_NUMS > 32)
#error "kernel mlfq scheduler supported max priority level:32 [0-31]"
#endif

static volatile MDS_Mask_t g_sysThreadPrioMask = 0x00U;
static MDS_ListNode_t g_sysSchedulerTable[MDS_THREAD_PRIORITY_NUMS];

void MDS_SchedulerInit(void)
{
    MDS_SCHEDULER_PRINT("scheduler init with max priority:%u", ARRAY_SIZE(g_sysSchedulerTable));

    g_sysThreadPrioMask = 0x00U;
    MDS_SkipListInitNode(g_sysSchedulerTable, ARRAY_SIZE(g_sysSchedulerTable));
}

void MDS_SchedulerInsertThread(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread->currPrio < MDS_THREAD_PRIORITY_NUMS);

    if (MDS_KernelCurrentThread() == thread) {
        thread->state = (thread->state & ~MDS_THREAD_STATE_MASK) | MDS_THREAD_STATE_RUNNING;
    } else {
        thread->state = (thread->state & ~MDS_THREAD_STATE_MASK) | MDS_THREAD_STATE_READY;

        MDS_ListRemoveNode(&(thread->node));

        if ((thread->state & MDS_THREAD_STATE_YIELD) != 0U) {
            MDS_ListInsertNodePrev(&(g_sysSchedulerTable[thread->currPrio]), &(thread->node));
        } else {
            MDS_ListInsertNodeNext(&(g_sysSchedulerTable[thread->currPrio]), &(thread->node));
        }

        g_sysThreadPrioMask |= (1UL << thread->currPrio);

        MDS_SCHEDULER_PRINT("scheduler insert thread(%p) entry:%p sp:%u priority:%u", thread, thread->entry,
                            thread->stackPoint, thread->currPrio);
    }
}

void MDS_SchedulerRemoveThread(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread->currPrio < MDS_THREAD_PRIORITY_NUMS);

    MDS_SCHEDULER_PRINT("scheduler remove thread(%p) entry:%p sp:%u priority:%u", thread, thread->entry,
                        thread->stackPoint, thread->currPrio);

    MDS_ListRemoveNode(&(thread->node));

    if (MDS_ListIsEmpty(&(g_sysSchedulerTable[thread->currPrio]))) {
        g_sysThreadPrioMask &= ~(1UL << thread->currPrio);
    }
}

void MDS_SchedulerRemainThread(void)
{
    MDS_Thread_t *thread = MDS_KernelCurrentThread();
    if (thread == NULL) {
        return;
    }

    if (thread->remainTick > 0) {
        thread->remainTick -= 1;
    }

    if (thread->remainTick == 0) {
        thread->remainTick = thread->initTick;
        thread->state |= MDS_THREAD_STATE_YIELD;
        MDS_KernelSchedulerCheck();
    }
}

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

MDS_Thread_t *MDS_SchedulerGetHighestPriorityThread(void)
{
    MDS_Thread_t *thread = NULL;
    size_t highestPrio   = MDS_SchedulerFFS(g_sysThreadPrioMask);
    if (highestPrio != 0U) {
        thread = CONTAINER_OF(g_sysSchedulerTable[highestPrio - 1].next, MDS_Thread_t, node);
    }

    return (thread);
}
