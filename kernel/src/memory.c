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
#include "mds_sys.h"

/* Hook -------------------------------------------------------------------- */
MDS_HOOK_INIT(MEMHEAP_INIT, MDS_MemHeap_t *memheap, void *heapBegin, void *heapLimit, size_t metaSize);
MDS_HOOK_INIT(MEMHEAP_ALLOC, MDS_MemHeap_t *memheap, void *ptr, size_t size);
MDS_HOOK_INIT(MEMHEAP_FREE, MDS_MemHeap_t *memheap, void *ptr);
MDS_HOOK_INIT(MEMHEAP_REALLOC, MDS_MemHeap_t *memheap, void *old, void *new, size_t size);

/* Typedef ----------------------------------------------------------------- */
typedef struct MemHeapNode {
    uintptr_t baseptr;
    struct MemHeapNode *prev, *next;
} MemHeapNode_t;

/* Define ------------------------------------------------------------------ */
#if (defined(MDS_DEBUG_MEMORY) && (MDS_DEBUG_MEMORY != 0))
#define MDS_MEMORY_PRINT(fmt, ...) MDS_LOG_D("[MEMORY]" fmt, ##__VA_ARGS__)
#else
#define MDS_MEMORY_PRINT(fmt, ...)
#endif

/* Variable ---------------------------------------------------------------- */
static const uintptr_t MDS_SYS_MEMHEAP_BASE = (UINTPTR_MAX ^ 1U);
static const uintptr_t MDS_SYS_MEMHEAP_USED = (1U);
static const size_t MDS_MEMHEAP_NODE_MINSIZE = VALUE_ALIGN(sizeof(MemHeapNode_t) + MDS_SYSMEM_ALIGN_SIZE - 1,
                                                           MDS_SYSMEM_ALIGN_SIZE);

/* Function ---------------------------------------------------------------- */
static bool MemHeapNodeIsUsed(MemHeapNode_t *node)
{
    return ((node->baseptr & MDS_SYS_MEMHEAP_USED) == MDS_SYS_MEMHEAP_USED);
}

static void MemHeapNodeCombine(MemHeapNode_t *node)
{
    node->next->next->prev = node;
    node->next = node->next->next;
}

static void MemHeapNodeSplit(MemHeapNode_t *node, MemHeapNode_t *next)
{
    next->prev = node;
    next->next = node->next;

    node->next->prev = next;
    node->next = next;
}

static size_t MemHeapTotalSize(MDS_MemHeap_t *memheap)
{
    MemHeapNode_t *limit = (MemHeapNode_t *)(memheap->limit);

    return (memheap->limit - (uintptr_t)(limit->next));
}

static bool MemHeapIsPtrInside(MDS_MemHeap_t *memheap, void *ptr)
{
    MemHeapNode_t *limit = (MemHeapNode_t *)(memheap->limit);

    return (((uintptr_t)(limit->next) < (uintptr_t)(ptr)) && ((uintptr_t)(ptr) < memheap->limit));
}

static void MemHeapRelistFree(MDS_MemHeap_t *memheap, MemHeapNode_t *node)
{
    MemHeapNode_t *lfree = node;

    while (MemHeapNodeIsUsed(lfree) && ((uintptr_t)(lfree->next) != memheap->limit)) {
        lfree = lfree->next;
    }

    memheap->lfree = (uintptr_t)lfree;
}

MDS_Err_t MDS_MemHeapInit(MDS_MemHeap_t *memheap, const char *name, void *heapBegin, void *heapLimit)
{
    MDS_ASSERT(memheap != NULL);

    uintptr_t alignBegin = VALUE_ALIGN((uintptr_t)heapBegin + MDS_SYSMEM_ALIGN_SIZE - 1, MDS_SYSMEM_ALIGN_SIZE);
    uintptr_t alignLimit = VALUE_ALIGN((uintptr_t)heapLimit, MDS_SYSMEM_ALIGN_SIZE);
    if ((alignLimit - alignBegin) < (sizeof(MemHeapNode_t) + sizeof(MemHeapNode_t))) {
        return (MDS_ENOMEM);
    }

    MDS_Err_t err = MDS_ObjectInit(&(memheap->object), MDS_OBJECT_TYPE_MEMHEAP, name);
    if (err != MDS_EOK) {
        return (err);
    }

    memheap->lfree = alignBegin;
    memheap->limit = alignLimit - sizeof(MemHeapNode_t);

    MemHeapNode_t *lfree = (MemHeapNode_t *)(memheap->lfree);
    MemHeapNode_t *limit = (MemHeapNode_t *)(memheap->limit);

    lfree->baseptr = ((uintptr_t)memheap) & MDS_SYS_MEMHEAP_BASE;
    lfree->prev = limit;
    lfree->next = limit;

    limit->baseptr = ((uintptr_t)memheap) | MDS_SYS_MEMHEAP_USED;
    limit->prev = lfree;
    limit->next = lfree;

#if (defined(MDS_MEMHEAP_STATS) && (MDS_MEMHEAP_STATS > 0))
    memheap->max = 0;
    memheap->cur = 0;
#endif

    MDS_HOOK_CALL(MEMHEAP_INIT, memheap, (void *)alignBegin, (void *)alignLimit, sizeof(MemHeapNode_t));

    MDS_MEMORY_PRINT("memheap(%p) init begin:%p limit:%p size:%u", memheap, alignBegin, alignLimit,
                     alignLimit - alignBegin);

    return (MDS_SemaphoreInit(&(memheap->sem), memheap->object.name, 1, 1));
}

MDS_Err_t MDS_MemHeapDeInit(MDS_MemHeap_t *memheap)
{
    MDS_ASSERT(memheap != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memheap->object)) == MDS_OBJECT_TYPE_MEMHEAP);

    MDS_SemaphoreDeInit(&(memheap->sem));
    MDS_ObjectDeInit(&(memheap->object));

    return (MDS_EOK);
}

static MemHeapNode_t *MDS_MemHeapNodeAlloc(MDS_MemHeap_t *memheap, size_t alignSize)
{
    size_t hopeSize = sizeof(MemHeapNode_t) + alignSize;
    MemHeapNode_t *lfree = (MemHeapNode_t *)(memheap->lfree);
    MemHeapNode_t *limit = (MemHeapNode_t *)(memheap->limit);

    for (MemHeapNode_t *node = lfree; node != limit; node = node->next) {
        if (MemHeapNodeIsUsed(node) || (((uintptr_t)(node->next) - (uintptr_t)(node)) < hopeSize)) {
            continue;
        }

        node->baseptr = ((uintptr_t)memheap) | MDS_SYS_MEMHEAP_USED;

        MemHeapNode_t *next = (MemHeapNode_t *)((uintptr_t)(node) + hopeSize);
        if (((uintptr_t)(node->next) - (uintptr_t)(next)) >= MDS_MEMHEAP_NODE_MINSIZE) {
            next->baseptr = (uintptr_t)memheap;
            MemHeapNodeSplit(node, next);
        }

#if (defined(MDS_MEMHEAP_STATS) && (MDS_MEMHEAP_STATS > 0))
        memheap->cur += (uintptr_t)(node->next) - (uintptr_t)(node);
        if (memheap->cur > memheap->max) {
            memheap->max = memheap->cur;
        }
#endif

        MemHeapRelistFree(memheap, lfree);

        MDS_HOOK_CALL(MEMHEAP_ALLOC, memheap, (uint8_t *)node + sizeof(MemHeapNode_t), alignSize);

        MDS_MEMORY_PRINT("memheap(%p) first:%p alloc node:%p size:%u", memheap, limit->next, node, alignSize);

        return (node);
    }

    MDS_MEMORY_PRINT("memheap(%p) first:%p alloc size:%u no memory", memheap, limit->next, alignSize);

    return (NULL);
}

static void MemHeapNodeFree(MDS_MemHeap_t *memheap, MemHeapNode_t *node)
{
#if (defined(MDS_MEMHEAP_STATS) && (MDS_MEMHEAP_STATS > 0))
    memheap->cur -= (uintptr_t)(node->next) - (uintptr_t)(node);
#endif

    node->baseptr &= ~MDS_SYS_MEMHEAP_USED;
    if (!MemHeapNodeIsUsed(node->next)) {
        MemHeapNodeCombine(node);
    }
    if (!MemHeapNodeIsUsed(node->prev)) {
        MemHeapNodeCombine(node->prev);
        node = node->prev;
    }

    if ((uintptr_t)node < memheap->lfree) {
        memheap->lfree = (uintptr_t)node;
    }

    MDS_HOOK_CALL(MEMHEAP_FREE, memheap, (uint8_t *)node);

    MDS_MEMORY_PRINT("memheap(%p) free node:%p size:%u", memheap, node,
                     (uintptr_t)(node->next) - (uintptr_t)(node) - sizeof(MemHeapNode_t));
}

static MemHeapNode_t *MDS_MemHeapNodeRealloc(MDS_MemHeap_t *memheap, MemHeapNode_t *node, size_t alignSize)
{
    MemHeapNode_t *next = NULL;

    if (MDS_SemaphoreAcquire(&(memheap->sem), MDS_TICK_FOREVER) != MDS_EOK) {
        return (NULL);
    }

    size_t hopeSize = sizeof(MemHeapNode_t) + alignSize;
    size_t nodeSize = (uintptr_t)(node->next) - (uintptr_t)(node);
    if (hopeSize > nodeSize) {
        if (MemHeapNodeIsUsed(node->next) || (hopeSize > ((uintptr_t)(node->next->next) - (uintptr_t)(node)))) {
            next = MDS_MemHeapNodeAlloc(memheap, alignSize);
            if (next != NULL) {
                MDS_MemBuffCopy((uint8_t *)next + sizeof(MemHeapNode_t), alignSize,
                                (uint8_t *)node + sizeof(MemHeapNode_t), nodeSize - sizeof(MemHeapNode_t));
                MemHeapNodeFree(memheap, node);
            }

            MDS_HOOK_CALL(MEMHEAP_REALLOC, memheap, (uint8_t *)node + sizeof(MemHeapNode_t),
                          (uint8_t *)next + sizeof(MemHeapNode_t), alignSize);

            MDS_SemaphoreRelease(&(memheap->sem));

            return (next);
        } else {
            MemHeapNodeCombine(node);

            MDS_HOOK_CALL(MEMHEAP_REALLOC, memheap, (uint8_t *)node + sizeof(MemHeapNode_t),
                          (uint8_t *)node + sizeof(MemHeapNode_t), alignSize);

            MDS_MEMORY_PRINT("memheap(%p) realloc node:%p extend size:%u->%u", memheap, node, nodeSize, hopeSize);
        }
    } else {
        MDS_HOOK_CALL(MEMHEAP_REALLOC, memheap, (uint8_t *)node + sizeof(MemHeapNode_t),
                      (uint8_t *)node + sizeof(MemHeapNode_t), alignSize);

        MDS_MEMORY_PRINT("memheap(%p) realloc node:%p reduce size:%u->%u", memheap, node, nodeSize, hopeSize);
    }

    next = (MemHeapNode_t *)((uintptr_t)(node) + hopeSize);
    if (((uintptr_t)(node->next) - (uintptr_t)(next)) >= MDS_MEMHEAP_NODE_MINSIZE) {
        next->baseptr = (uintptr_t)memheap;
        MemHeapNodeSplit(node, next);
        if (!MemHeapNodeIsUsed(node->next->next)) {
            MemHeapNodeCombine(node->next);
        }
    }

#if (defined(MDS_MEMHEAP_STATS) && (MDS_MEMHEAP_STATS > 0))
    memheap->cur += ((uintptr_t)(node->next) - (uintptr_t)(node)) - nodeSize;
    if (memheap->cur > memheap->max) {
        memheap->max = memheap->cur;
    }
#endif

    if ((uintptr_t)node < memheap->lfree) {
        MemHeapRelistFree(memheap, node->next);
    }
    MDS_SemaphoreRelease(&(memheap->sem));

    return (node);
}

void MDS_MemHeapFree(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    MemHeapNode_t *node = (MemHeapNode_t *)((uint8_t *)(ptr) - sizeof(MemHeapNode_t));
    if (node == NULL) {
        return;
    }

    MDS_MemHeap_t *memheap = (MDS_MemHeap_t *)(node->baseptr & MDS_SYS_MEMHEAP_BASE);
    if (MDS_SemaphoreAcquire(&(memheap->sem), MDS_TICK_FOREVER) != MDS_EOK) {
        return;
    }

    if (MemHeapIsPtrInside(memheap, ptr)) {
        MemHeapNodeFree(memheap, node);
    } else {
        MDS_MEMORY_PRINT("memheap(%p) free ptr(%p) is not inside", memheap, ptr);
    }

    MDS_SemaphoreRelease(&(memheap->sem));
}

void *MDS_MemHeapAlloc(MDS_MemHeap_t *memheap, size_t size)
{
    MDS_ASSERT(memheap != NULL);
    MDS_ASSERT(size > 0);
    MDS_ASSERT(MDS_ObjectGetType(&(memheap->object)) == MDS_OBJECT_TYPE_MEMHEAP);

    size_t alignSize = VALUE_ALIGN(size + MDS_SYSMEM_ALIGN_SIZE - 1, MDS_SYSMEM_ALIGN_SIZE);
    size_t totalSize = MemHeapTotalSize(memheap);
    if ((alignSize == 0) || (alignSize > totalSize)) {
        MDS_MEMORY_PRINT("memheap(%p) alloc size:%u error of total:%u", memheap, alignSize, totalSize);
        return (NULL);
    }

    if (MDS_SemaphoreAcquire(&(memheap->sem), MDS_TICK_FOREVER) != MDS_EOK) {
        return (NULL);
    }

    MemHeapNode_t *node = MDS_MemHeapNodeAlloc(memheap, alignSize);

    MDS_SemaphoreRelease(&(memheap->sem));

    return ((node != NULL) ? ((uint8_t *)node + sizeof(MemHeapNode_t)) : (NULL));
}

void *MDS_MemHeapCalloc(MDS_MemHeap_t *memheap, size_t nmemb, size_t size)
{
    MDS_ASSERT(memheap != NULL);
    MDS_ASSERT((nmemb > 0) && (size > 0));
    MDS_ASSERT(MDS_ObjectGetType(&(memheap->object)) == MDS_OBJECT_TYPE_MEMHEAP);

    size_t alignSize = VALUE_ALIGN((nmemb * size) + MDS_SYSMEM_ALIGN_SIZE - 1, MDS_SYSMEM_ALIGN_SIZE);
    size_t totalSize = MemHeapTotalSize(memheap);
    if ((alignSize == 0) || (alignSize > totalSize)) {
        MDS_MEMORY_PRINT("memheap(%p) calloc size:%u error of total:%u", memheap, alignSize, totalSize);
        return (NULL);
    }

    if (MDS_SemaphoreAcquire(&(memheap->sem), MDS_TICK_FOREVER) != MDS_EOK) {
        return (NULL);
    }

    MemHeapNode_t *node = MDS_MemHeapNodeAlloc(memheap, alignSize);

    MDS_SemaphoreRelease(&(memheap->sem));

    if (node != NULL) {
        void *ptr = (uint8_t *)node + sizeof(MemHeapNode_t);
        MDS_MemBuffSet(ptr, 0, alignSize);
        return (ptr);
    }

    return (NULL);
}

void *MDS_MemHeapRealloc(MDS_MemHeap_t *memheap, void *ptr, size_t size)
{
    MDS_ASSERT(memheap != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memheap->object)) == MDS_OBJECT_TYPE_MEMHEAP);

    if (ptr == NULL) {
        return (MDS_MemHeapAlloc(memheap, size));
    } else if (size == 0) {
        MDS_MemHeapFree(ptr);
        return (NULL);
    }

    size_t alignSize = VALUE_ALIGN(size + MDS_SYSMEM_ALIGN_SIZE - 1, MDS_SYSMEM_ALIGN_SIZE);
    size_t totalSize = MemHeapTotalSize(memheap);
    if ((!MemHeapIsPtrInside(memheap, ptr)) || (alignSize > totalSize)) {
        MDS_MEMORY_PRINT("memheap(%p) realloc ptr(%p) size:%u error of total:%u", memheap, ptr, alignSize, totalSize);
        return (NULL);
    }

    MemHeapNode_t *node = (MemHeapNode_t *)((uint8_t *)(ptr) - sizeof(MemHeapNode_t));
    MemHeapNode_t *next = MDS_MemHeapNodeRealloc(memheap, node, alignSize);

    return ((next != NULL) ? ((uint8_t *)next + sizeof(MemHeapNode_t)) : (NULL));
}

void *MDS_MemHeapAlignedAlloc(MDS_MemHeap_t *memheap, size_t align, size_t size)
{
    MDS_ASSERT(memheap != NULL);
    MDS_ASSERT(size > 0);
    MDS_ASSERT(MDS_ObjectGetType(&(memheap->object)) == MDS_OBJECT_TYPE_MEMHEAP);

    if ((align == 0) || ((align % MDS_SYSMEM_ALIGN_SIZE) != 0)) {
        return (NULL);
    }

    MemHeapNode_t *next = NULL;
    size_t needSize = VALUE_ALIGN(size + MDS_SYSMEM_ALIGN_SIZE - 1, MDS_SYSMEM_ALIGN_SIZE);
    size_t nodeSize = VALUE_ALIGN(sizeof(MemHeapNode_t) + align - 1, MDS_SYSMEM_ALIGN_SIZE);
    size_t alignSize = MDS_MEMHEAP_NODE_MINSIZE + nodeSize + needSize;
    size_t totalSize = MemHeapTotalSize(memheap);
    if ((alignSize == 0) || (alignSize > totalSize)) {
        MDS_MEMORY_PRINT("memheap(%p) aligned alloc size:%u error of total:%u", memheap, alignSize, totalSize);
        return (NULL);
    }

    if (MDS_SemaphoreAcquire(&(memheap->sem), MDS_TICK_FOREVER) != MDS_EOK) {
        return (NULL);
    }

    MemHeapNode_t *node = MDS_MemHeapNodeAlloc(memheap, alignSize);
    if (node != NULL) {
        next = (MemHeapNode_t *)((uintptr_t)(node->next) - sizeof(MemHeapNode_t) - needSize);
        if (next != node) {
            next->baseptr = ((uintptr_t)memheap) | MDS_SYS_MEMHEAP_USED;
            MemHeapNodeSplit(node, next);
            MemHeapNodeFree(memheap, node);
        }
    }

    MDS_SemaphoreRelease(&(memheap->sem));

    return ((next != NULL) ? ((uint8_t *)next + sizeof(MemHeapNode_t)) : (NULL));
}

/* SysMem ------------------------------------------------------------------ */
static MDS_MemHeap_t g_sysMemHeap;

__attribute__((weak)) void MDS_SysMemBuff(void **heapBase, void **heapLimit)
{
#if defined(__IAR_SYSTEMS_ICC__)
#pragma section(".heap")
    *heapBase = __section_start(".heap");
    *heapLimit = __section_end(".heap");
#else
    extern uintptr_t __HeapBase;
    extern uintptr_t __HeapLimit;
    *heapBase = &__HeapBase;
    *heapLimit = &__HeapLimit;
#endif
}

void MDS_SysMemInit(void)
{
    if (MDS_ObjectGetType(&(g_sysMemHeap.object)) == MDS_OBJECT_TYPE_MEMHEAP) {
        return;
    }

    void *heapBase, *heapLimit;
    MDS_SysMemBuff(&heapBase, &heapLimit);

    MDS_Err_t err = MDS_MemHeapInit(&g_sysMemHeap, "sysmem", heapBase, heapLimit);

    MDS_MEMORY_PRINT("MDS_SysMemInit(%p, %p) err:%d", heapBase, heapLimit, err);

    UNUSED(err);
}

void MDS_SysMemFree(void *ptr)
{
    MDS_ASSERT(MDS_CoreInterruptCurrent() == 0);

    MDS_SysMemInit();
    MDS_MemHeapFree(ptr);
}

void *MDS_SysMemAlloc(size_t size)
{
    MDS_ASSERT(MDS_CoreInterruptCurrent() == 0);

    MDS_SysMemInit();
    return (MDS_MemHeapAlloc(&g_sysMemHeap, size));
}

void *MDS_SysMemCalloc(size_t nmemb, size_t size)
{
    MDS_ASSERT(MDS_CoreInterruptCurrent() == 0);

    MDS_SysMemInit();
    return (MDS_MemHeapCalloc(&g_sysMemHeap, nmemb, size));
}

void *MDS_SysMemRealloc(void *ptr, size_t size)
{
    MDS_ASSERT(MDS_CoreInterruptCurrent() == 0);

    MDS_SysMemInit();
    return (MDS_MemHeapRealloc(&g_sysMemHeap, ptr, size));
}

void *MDS_SysMemAlignedAlloc(size_t align, size_t size)
{
    MDS_ASSERT(MDS_CoreInterruptCurrent() == 0);

    MDS_SysMemInit();
    return (MDS_MemHeapAlignedAlloc(&g_sysMemHeap, align, size));
}
