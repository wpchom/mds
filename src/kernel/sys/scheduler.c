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

/* MLFQ Scheduler ---------------------------------------------------------- */
#if ((CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX <= 0) || (CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX > 32))
#error "kernel mlfq scheduler supported max priority 32-bit / 64-bit"
#endif

/* Variable ---------------------------------------------------------------- */
static volatile uint32_t g_sysThreadPrioMask = 0x00U;
static MDS_DListNode_t g_sysSchedulerTable[CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX];

/* Function ---------------------------------------------------------------- */
__attribute__((weak)) size_t MDS_CoreSchedulerFFS(register size_t value)
{
    if (value == 0) {
        return (0);
    }

    size_t shift = __CHAR_BIT__ * sizeof(size_t) >> 1;
    size_t mask = __SIZE_MAX__ >> shift;

    size_t num = 1;
    do {
        if ((value & mask) == 0) {
            num += shift;
            value >>= shift;
        }
        shift >>= 1;
        mask >>= shift;
    } while (shift > 0);

    return (num);
}

void MDS_SchedulerInit(void)
{
    if ((g_sysSchedulerTable[0].next == NULL) || (g_sysSchedulerTable[0].prev == NULL)) {
        g_sysThreadPrioMask = 0U;
        for (size_t idx = 0; idx < ARRAY_SIZE(g_sysSchedulerTable); ++idx) {
            MDS_DListInitNode(&(g_sysSchedulerTable[idx]));
        }
        MDS_LOG_D("[scheduler] init with max priority:%" PRIuPTR, ARRAY_SIZE(g_sysSchedulerTable));
    }
}

void MDS_SchedulerInsertThread(MDS_Thread_t *thread)
{
    MDS_SchedulerInit();

    MDS_DListRemoveNode(&(thread->nodeWait.node));

    if (thread->currPrio.priority < CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX) {
        if (MDS_ThreadIsYield(thread)) {
            MDS_DListInsertNodePrev(&(g_sysSchedulerTable[thread->currPrio.priority]),
                                    &(thread->nodeWait.node));
        } else {
            MDS_DListInsertNodeNext(&(g_sysSchedulerTable[thread->currPrio.priority]),
                                    &(thread->nodeWait.node));
        }
        g_sysThreadPrioMask |= (1UL << thread->currPrio.priority);
    }
}

void MDS_SchedulerRemoveThread(MDS_Thread_t *thread)
{
    MDS_DListRemoveNode(&(thread->nodeWait.node));

    if ((thread->currPrio.priority < CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX) &&
        (MDS_DListIsEmpty(&(g_sysSchedulerTable[thread->currPrio.priority])))) {
        g_sysThreadPrioMask &= ~(1UL << thread->currPrio.priority);
    }
}

MDS_Thread_t *MDS_SchedulerPeekThread(void)
{
    MDS_Thread_t *thread = NULL;

    do {
        size_t highestPrio = MDS_CoreSchedulerFFS(g_sysThreadPrioMask);
        if (highestPrio == 0U) {
            break;
        }

        MDS_DListNode_t *list = &(g_sysSchedulerTable[highestPrio - 1]);
        if (MDS_DListIsEmpty(list)) {
            MDS_LOG_E("[scheduler] priority list empty, mask may be inconsistent");
            break;
        }

        thread = CONTAINER_OF(list->next, MDS_Thread_t, nodeWait.node);
    } while (0);

    if (thread == NULL) {
        thread = MDS_KernelIdleThread();
    }

    return (thread);
}
