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
__attribute__((weak)) size_t MDS_SchedulerFFS(uint32_t value)
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
    MDS_LOG_D("[scheduler] init with max priority:%zu", ARRAY_SIZE(g_sysSchedulerTable));

    g_sysThreadPrioMask = 0U;
    MDS_SkipListInitNode(g_sysSchedulerTable, ARRAY_SIZE(g_sysSchedulerTable));
}

void MDS_SchedulerInsertThread(MDS_Thread_t *thread)
{
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

    MDS_LOG_D("[scheduler] insert thread(%p) entry:%p sp:%p priority:%u", thread, thread->entry,
              thread->stackPoint, thread->currPrio.priority);
}

void MDS_SchedulerRemoveThread(MDS_Thread_t *thread)
{
    MDS_LOG_D("[scheduler] remove thread(%p) entry:%p sp:%p priority:%u", thread, thread->entry,
              thread->stackPoint, thread->currPrio.priority);

    MDS_DListRemoveNode(&(thread->nodeWait.node));

    if ((thread->currPrio.priority < CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX) &&
        (MDS_DListIsEmpty(&(g_sysSchedulerTable[thread->currPrio.priority])))) {
        g_sysThreadPrioMask &= ~(1UL << thread->currPrio.priority);
    }
}

MDS_Thread_t *MDS_SchedulerPeekThread(void)
{
    MDS_Thread_t *thread = NULL;

    size_t highestPrio = MDS_SchedulerFFS(g_sysThreadPrioMask);
    if (highestPrio != 0U) {
        thread = CONTAINER_OF(g_sysSchedulerTable[highestPrio - 1].next, MDS_Thread_t,
                              nodeWait.node);
    } else {
        thread = MDS_KernelIdleThread();
    }

    return (thread);
}
