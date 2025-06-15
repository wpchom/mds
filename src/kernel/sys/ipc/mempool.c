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
union MDS_MemPoolHeader {
    union MDS_MemPoolHeader *next;
    MDS_MemPool_t *memPool;
};

static void MDS_MemPoolListInit(MDS_MemPool_t *memPool, size_t blkNums)
{
    memPool->lfree = memPool->memBuff;

    for (size_t idx = 0; idx < blkNums; idx++) {
        union MDS_MemPoolHeader *list = (union MDS_MemPoolHeader *)(&(
            ((uint8_t *)(memPool->memBuff))[idx * (sizeof(union MDS_MemPoolHeader) +
                                                   memPool->blkSize)]));
        list->next = (union MDS_MemPoolHeader *)(memPool->lfree);
        memPool->lfree = list;
    }
}

MDS_Err_t MDS_MemPoolInit(MDS_MemPool_t *memPool, const char *name, void *memBuff, size_t bufSize,
                          size_t blkSize)
{
    MDS_ASSERT(memPool != NULL);
    MDS_ASSERT(bufSize > (sizeof(union MDS_MemPoolHeader) + blkSize));

    MDS_Err_t err = MDS_ObjectInit(&(memPool->object), MDS_OBJECT_TYPE_MEMPOOL, name);
    if (err == MDS_EOK) {
        memPool->memBuff = memBuff;
        memPool->blkSize = VALUE_ALIGN(blkSize + MDS_SYSMEM_ALIGN_SIZE - 1, MDS_SYSMEM_ALIGN_SIZE);
        MDS_MemPoolListInit(memPool,
                            bufSize / (memPool->blkSize + sizeof(union MDS_MemPoolHeader)));
        MDS_KernelWaitQueueInit(&(memPool->queueWait));
        MDS_SpinLockInit(&(memPool->spinlock));
    }

    return (err);
}

MDS_Err_t MDS_MemPoolDeInit(MDS_MemPool_t *memPool)
{
    MDS_ASSERT(memPool != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memPool->object)) == MDS_OBJECT_TYPE_MEMPOOL);

    MDS_Lock_t lock = MDS_CriticalLock(&(memPool->spinlock));

    MDS_KernelWaitQueueDrain(&(memPool->queueWait));
    MDS_Err_t err = MDS_ObjectDeInit(&(memPool->object));

    MDS_CriticalRestore((err != MDS_EOK) ? (&(memPool->spinlock)) : (NULL), lock);

    return (err);
}

MDS_MemPool_t *MDS_MemPoolCreate(const char *name, size_t blkSize, size_t blkNums)
{
    MDS_MemPool_t *memPool = (MDS_MemPool_t *)MDS_ObjectCreate(sizeof(MDS_MemPool_t),
                                                               MDS_OBJECT_TYPE_MEMPOOL, name);

    if (memPool != NULL) {
        memPool->blkSize = VALUE_ALIGN(blkSize + MDS_SYSMEM_ALIGN_SIZE - 1, MDS_SYSMEM_ALIGN_SIZE);
        memPool->memBuff = MDS_SysMemAlloc((memPool->blkSize + sizeof(union MDS_MemPoolHeader)) *
                                           blkNums);
        if (memPool->memBuff == NULL) {
            MDS_ObjectDestroy(&(memPool->object));
            return (NULL);
        }
        MDS_MemPoolListInit(memPool, blkNums);
        MDS_KernelWaitQueueInit(&(memPool->queueWait));
        MDS_SpinLockInit(&(memPool->spinlock));
    }

    return (memPool);
}

MDS_Err_t MDS_MemPoolDestroy(MDS_MemPool_t *memPool)
{
    MDS_ASSERT(memPool != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memPool->object)) == MDS_OBJECT_TYPE_MEMPOOL);

    MDS_Lock_t lock = MDS_CriticalLock(&(memPool->spinlock));

    void *memBuff = memPool->memBuff;
    MDS_KernelWaitQueueDrain(&(memPool->queueWait));
    MDS_Err_t err = MDS_ObjectDestroy(&(memPool->object));
    if (err == MDS_EOK) {
        MDS_SysMemFree(memBuff);
    }

    MDS_CriticalRestore((err != MDS_EOK) ? (&(memPool->spinlock)) : (NULL), lock);

    return (err);
}

void *MDS_MemPoolAlloc(MDS_MemPool_t *memPool, MDS_Timeout_t timeout)
{
    MDS_ASSERT(memPool != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memPool->object)) == MDS_OBJECT_TYPE_MEMPOOL);

    MDS_Err_t err = MDS_EOK;
    union MDS_MemPoolHeader *blk = NULL;
    MDS_Thread_t *thread = MDS_KernelCurrentThread();

    MDS_HOOK_CALL(KERNEL, mempool,
                  (memPool, MDS_KERNEL_TRACE_MEMPOOL_TRY_ALLOC, MDS_EOK, timeout, NULL));

    MDS_Lock_t lock = MDS_CriticalLock(&(memPool->spinlock));

    if (memPool->lfree == NULL) {
        if (timeout.ticks == MDS_CLOCK_TICK_NO_WAIT) {
            err = MDS_ETIMEOUT;
        } else if (thread == NULL) {
            MDS_LOG_W("[mempool] thread is null try alloc mempool");
            err = MDS_EACCES;
        } else {
            MDS_LOG_D(
                "[mempool] mempool(%p) alloc blocksize:%zu suspend thread(%p) entry:%p timer wait:%lu",
                memPool, memPool->blkSize, thread, thread->entry, (unsigned long)timeout.ticks);
        }
    }

    while ((err == MDS_EOK) && (memPool->lfree == NULL)) {
        err = MDS_KernelWaitQueueUntil(&(memPool->queueWait), thread, timeout, true, &lock,
                                       &(memPool->spinlock));
    }

    if (err == MDS_EOK) {
        blk = (union MDS_MemPoolHeader *)(memPool->lfree);
        memPool->lfree = (void *)(blk->next);
        blk->memPool = memPool;
    }

    MDS_CriticalRestore(&(memPool->spinlock), lock);

    MDS_HOOK_CALL(KERNEL, mempool,
                  (memPool, MDS_KERNEL_TRACE_MEMPOOL_HAS_ALLOC, err, timeout, blk));

    return (((err == MDS_EOK) && (blk != NULL)) ? ((void *)(blk + 1)) : (NULL));
}

static void MDS_MemPoolFreeBlk(MDS_MemPool_t *memPool, union MDS_MemPoolHeader *blk)
{
    MDS_ASSERT(MDS_ObjectGetType(&(memPool->object)) == MDS_OBJECT_TYPE_MEMPOOL);

    MDS_Err_t err = MDS_EOK;
    MDS_Thread_t *thread = NULL;

    MDS_Lock_t lock = MDS_CriticalLock(&(memPool->spinlock));

    blk->next = memPool->lfree;
    memPool->lfree = blk;
    thread = MDS_KernelWaitQueueResume(&(memPool->queueWait));

    MDS_CriticalRestore(&(memPool->spinlock), lock);

    MDS_LOG_D("[mempool] free block to mempool(%p) which blksize:%zu", memPool, memPool->blkSize);

    MDS_HOOK_CALL(KERNEL, mempool,
                  (memPool, MDS_KERNEL_TRACE_MEMPOOL_HAS_FREE, MDS_EOK, MDS_TIMEOUT_NO_WAIT, blk));

    if (thread != NULL) {
        MDS_KernelSchedulerCheck();
    }
}

void MDS_MemPoolFree(void *blkPtr)
{
    if (blkPtr == NULL) {
        return;
    }

    union MDS_MemPoolHeader *blk = (union MDS_MemPoolHeader *)((uint8_t *)blkPtr -
                                                               sizeof(union MDS_MemPoolHeader));
    if (blk == NULL) {
        return;
    }

    MDS_MemPool_t *memPool = blk->memPool;
    if (memPool != NULL) {
        MDS_MemPoolFreeBlk(memPool, blk);
    }
}

size_t MDS_MemPoolGetBlkSize(MDS_MemPool_t *memPool)
{
    MDS_ASSERT(memPool != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memPool->object)) == MDS_OBJECT_TYPE_MEMPOOL);

    return (memPool->blkSize);
}

size_t MDS_MemPoolGetBlkFree(MDS_MemPool_t *memPool)
{
    MDS_ASSERT(memPool != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memPool->object)) == MDS_OBJECT_TYPE_MEMPOOL);

    size_t cnt = 0;

    MDS_Lock_t lock = MDS_CriticalLock(&(memPool->spinlock));

    for (union MDS_MemPoolHeader *blk = (union MDS_MemPoolHeader *)(memPool->lfree);
         (blk != NULL) || (blk != memPool->memBuff); blk = blk->next) {
        cnt += 1;
    }

    MDS_CriticalRestore(&(memPool->spinlock), lock);

    return (cnt);
}
