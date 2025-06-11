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

/* Define ------------------------------------------------------------------ */
MDS_LOG_MODULE_DECLARE(kernel, CONFIG_MDS_KERNEL_LOG_LEVEL);

/* Typedef ----------------------------------------------------------------- */
typedef struct MemHeapLLFF_Node {
    uintptr_t baseptr;
    struct MemHeapLLFF_Node *prev, *next;
} MemHeapLLFF_Node_t;

/* Variable ---------------------------------------------------------------- */
static const uintptr_t MDS_MEMHEAP_LLFF_BASE = (UINTPTR_MAX ^ 1U);
static const uintptr_t MDS_MEMHEAP_LLFF_USED = (1U);
static const size_t MDS_MEMHEAP_LLFF_MINSIZE = VALUE_ALIGN(
    sizeof(MemHeapLLFF_Node_t) + MDS_SYSMEM_ALIGN_SIZE - 1, MDS_SYSMEM_ALIGN_SIZE);

/* Function ---------------------------------------------------------------- */
static bool MemHeapLLFF_NodeIsUsed(MemHeapLLFF_Node_t *node)
{
    return ((node->baseptr & MDS_MEMHEAP_LLFF_USED) == MDS_MEMHEAP_LLFF_USED);
}

static void MemHeapLLFF_NodeCombine(MemHeapLLFF_Node_t *node)
{
    node->next->next->prev = node;
    node->next = node->next->next;
}

static void MemHeapLLFF_NodeSplit(MemHeapLLFF_Node_t *node, MemHeapLLFF_Node_t *next)
{
    next->prev = node;
    next->next = node->next;

    node->next->prev = next;
    node->next = next;
}

static size_t MemHeapLLFF_Size(MDS_MemHeap_t *memheap)
{
    MemHeapLLFF_Node_t *limit = (MemHeapLLFF_Node_t *)(memheap->limit);

    return ((uintptr_t)(memheap->limit) - (uintptr_t)(limit->next));
}

static bool MemHeapLLFF_IsPtrInside(MDS_MemHeap_t *memheap, void *ptr)
{
    MemHeapLLFF_Node_t *limit = (MemHeapLLFF_Node_t *)(memheap->limit);

    return (((uintptr_t)(limit->next) < (uintptr_t)(ptr)) &&
            ((uintptr_t)(ptr) < (uintptr_t)(memheap->limit)));
}

static void MemHeapLLFF_RelistFree(MDS_MemHeap_t *memheap, MemHeapLLFF_Node_t *node)
{
    MemHeapLLFF_Node_t *lfree = node;

    while (MemHeapLLFF_NodeIsUsed(lfree) &&
           ((uintptr_t)(lfree->next) != (uintptr_t)(memheap->limit))) {
        lfree = lfree->next;
    }

    memheap->begin = (void *)lfree;
}

static MDS_Err_t MDS_MemHeapLLFF_Setup(MDS_MemHeap_t *memheap, void *heapBase, size_t heapSize)
{
    uintptr_t alignBegin = VALUE_ALIGN((uintptr_t)heapBase + MDS_SYSMEM_ALIGN_SIZE - 1,
                                       MDS_SYSMEM_ALIGN_SIZE);
    uintptr_t alignLimit = VALUE_ALIGN((uintptr_t)heapBase + heapSize, MDS_SYSMEM_ALIGN_SIZE);
    if ((alignLimit - alignBegin) < (sizeof(MemHeapLLFF_Node_t) + sizeof(MemHeapLLFF_Node_t))) {
        return (MDS_ENOMEM);
    }

    memheap->begin = (void *)alignBegin;
    memheap->limit = (void *)(alignLimit - sizeof(MemHeapLLFF_Node_t));

    MemHeapLLFF_Node_t *lfree = (MemHeapLLFF_Node_t *)(memheap->begin);
    MemHeapLLFF_Node_t *limit = (MemHeapLLFF_Node_t *)(memheap->limit);

    lfree->baseptr = ((uintptr_t)memheap) & MDS_MEMHEAP_LLFF_BASE;
    lfree->prev = limit;
    lfree->next = limit;

    limit->baseptr = ((uintptr_t)memheap) | MDS_MEMHEAP_LLFF_USED;
    limit->prev = lfree;
    limit->next = lfree;

    MDS_HOOK_CALL(KERNEL, memheap,
                  (memheap, MDS_KERNEL_TRACE_MEMHEAP_INIT, (void *)alignBegin, (void *)alignLimit,
                   sizeof(MemHeapLLFF_Node_t)));

    MDS_LOG_D("[memory] memheap(%p) init begin:%p limit:%p size:%zu", memheap, (void *)alignBegin,
              (void *)alignLimit, alignLimit - alignBegin);

    return (MDS_EOK);
}

static void MemHeapLLFF_NodeFree(MDS_MemHeap_t *memheap, MemHeapLLFF_Node_t *node)
{
#if (defined(CONFIG_MDS_KERNEL_STATS_ENABLE) && (CONFIG_MDS_KERNEL_STATS_ENABLE != 0))
    memheap->size.cur -= (uintptr_t)(node->next) - (uintptr_t)(node);
#endif

    node->baseptr &= ~MDS_MEMHEAP_LLFF_USED;
    if (!MemHeapLLFF_NodeIsUsed(node->next)) {
        MemHeapLLFF_NodeCombine(node);
    }
    if (!MemHeapLLFF_NodeIsUsed(node->prev)) {
        MemHeapLLFF_NodeCombine(node->prev);
        node = node->prev;
    }

    if ((uintptr_t)node < (uintptr_t)(memheap->begin)) {
        memheap->begin = (void *)node;
    }

    MDS_HOOK_CALL(KERNEL, memheap,
                  (memheap, MDS_KERNEL_TRACE_MEMHEAP_FREE, NULL, node,
                   (uintptr_t)(node->next) - (uintptr_t)(node) - sizeof(MemHeapLLFF_Node_t)));

    MDS_LOG_D("[memory] memheap(%p) free node:%p size:%zu", memheap, node,
              (uintptr_t)(node->next) - (uintptr_t)(node) - sizeof(MemHeapLLFF_Node_t));
}

static void MDS_MemHeapLLFF_Free(MDS_MemHeap_t *memheap, void *ptr)
{
    MemHeapLLFF_Node_t *node = (MemHeapLLFF_Node_t *)((uint8_t *)(ptr) -
                                                      sizeof(MemHeapLLFF_Node_t));
    if (node == NULL) {
        return;
    }

    if (MemHeapLLFF_IsPtrInside(memheap, ptr)) {
        MemHeapLLFF_NodeFree(memheap, node);
    } else {
        MDS_LOG_W("[memory] memheap(%p) free ptr(%p) is not inside", memheap, ptr);
    }
}

static MemHeapLLFF_Node_t *MemHeapLLFF_NodeAlloc(MDS_MemHeap_t *memheap, size_t alignSize)
{
    size_t hopeSize = sizeof(MemHeapLLFF_Node_t) + alignSize;
    MemHeapLLFF_Node_t *lfree = (MemHeapLLFF_Node_t *)(memheap->begin);
    MemHeapLLFF_Node_t *limit = (MemHeapLLFF_Node_t *)(memheap->limit);

    for (MemHeapLLFF_Node_t *node = lfree; node != limit; node = node->next) {
        if (MemHeapLLFF_NodeIsUsed(node) ||
            (((uintptr_t)(node->next) - (uintptr_t)(node)) < hopeSize)) {
            continue;
        }

        node->baseptr = ((uintptr_t)memheap) | MDS_MEMHEAP_LLFF_USED;

        MemHeapLLFF_Node_t *next = (MemHeapLLFF_Node_t *)((uintptr_t)(node) + hopeSize);
        if (((uintptr_t)(node->next) - (uintptr_t)(next)) >= MDS_MEMHEAP_LLFF_MINSIZE) {
            next->baseptr = (uintptr_t)memheap;
            MemHeapLLFF_NodeSplit(node, next);
        }

#if (defined(CONFIG_MDS_KERNEL_STATS_ENABLE) && (CONFIG_MDS_KERNEL_STATS_ENABLE != 0))
        memheap->size.cur += (uintptr_t)(node->next) - (uintptr_t)(node);
        if (memheap->size.cur > memheap->size.max) {
            memheap->size.max = memheap->size.cur;
        }
#endif

        MemHeapLLFF_RelistFree(memheap, lfree);

        MDS_HOOK_CALL(KERNEL, memheap,
                      (memheap, MDS_KERNEL_TRACE_MEMHEAP_ALLOC,
                       (uint8_t *)node + sizeof(MemHeapLLFF_Node_t), NULL, alignSize));

        MDS_LOG_D("[memory] memheap(%p) first:%p alloc node:%p size:%zu", memheap, limit->next,
                  node, alignSize);

        return (node);
    }

    MDS_LOG_D("[memory] memheap(%p) first:%p alloc size:%zu no memory", memheap, limit->next,
              alignSize);

    return (NULL);
}

static void *MDS_MemHeapLLFF_Alloc(MDS_MemHeap_t *memheap, size_t size)
{
    size_t alignSize = VALUE_ALIGN(size + MDS_SYSMEM_ALIGN_SIZE - 1, MDS_SYSMEM_ALIGN_SIZE);
    size_t totalSize = MemHeapLLFF_Size(memheap);
    if ((alignSize == 0) || (alignSize > totalSize)) {
        MDS_LOG_E("[memory] memheap(%p) alloc size:%zu error of total:%zu", memheap, alignSize,
                  totalSize);
        return (NULL);
    }

    MemHeapLLFF_Node_t *node = MemHeapLLFF_NodeAlloc(memheap, alignSize);

    return ((node != NULL) ? ((uint8_t *)node + sizeof(MemHeapLLFF_Node_t)) : (NULL));
}

static MemHeapLLFF_Node_t *MemHeapLLFF_NodeRealloc(MDS_MemHeap_t *memheap,
                                                   MemHeapLLFF_Node_t *node, size_t alignSize)
{
    MemHeapLLFF_Node_t *next = NULL;

    size_t hopeSize = sizeof(MemHeapLLFF_Node_t) + alignSize;
    size_t nodeSize = (uintptr_t)(node->next) - (uintptr_t)(node);
    if (hopeSize > nodeSize) {
        if (MemHeapLLFF_NodeIsUsed(node->next) ||
            (hopeSize > ((uintptr_t)(node->next->next) - (uintptr_t)(node)))) {
            next = MemHeapLLFF_NodeAlloc(memheap, alignSize);
            if (next != NULL) {
                MDS_MemBuffCopy((uint8_t *)next + sizeof(MemHeapLLFF_Node_t), alignSize,
                                (uint8_t *)node + sizeof(MemHeapLLFF_Node_t),
                                nodeSize - sizeof(MemHeapLLFF_Node_t));
                MemHeapLLFF_NodeFree(memheap, node);
            }

            MDS_HOOK_CALL(KERNEL, memheap,
                          (memheap, MDS_KERNEL_TRACE_MEMHEAP_REALLOC,
                           (uint8_t *)node + sizeof(MemHeapLLFF_Node_t),
                           (uint8_t *)next + sizeof(MemHeapLLFF_Node_t), alignSize));

            return (next);
        } else {
            MemHeapLLFF_NodeCombine(node);

            MDS_HOOK_CALL(KERNEL, memheap,
                          (memheap, MDS_KERNEL_TRACE_MEMHEAP_REALLOC,
                           (uint8_t *)node + sizeof(MemHeapLLFF_Node_t),
                           (uint8_t *)node + sizeof(MemHeapLLFF_Node_t), alignSize));

            MDS_LOG_D("[memory] memheap(%p) realloc node:%p extend size:%zu->%zu", memheap, node,
                      nodeSize, hopeSize);
        }
    } else {
        MDS_HOOK_CALL(KERNEL, memheap,
                      (memheap, MDS_KERNEL_TRACE_MEMHEAP_REALLOC,
                       (uint8_t *)node + sizeof(MemHeapLLFF_Node_t),
                       (uint8_t *)node + sizeof(MemHeapLLFF_Node_t), alignSize));

        MDS_LOG_D("[memory] memheap(%p) realloc node:%p reduce size:%zu->%zu", memheap, node,
                  nodeSize, hopeSize);
    }

    next = (MemHeapLLFF_Node_t *)((uintptr_t)(node) + hopeSize);
    if (((uintptr_t)(node->next) - (uintptr_t)(next)) >= MDS_MEMHEAP_LLFF_MINSIZE) {
        next->baseptr = (uintptr_t)memheap;
        MemHeapLLFF_NodeSplit(node, next);
        if (!MemHeapLLFF_NodeIsUsed(node->next->next)) {
            MemHeapLLFF_NodeCombine(node->next);
        }
    }

#if (defined(CONFIG_MDS_KERNEL_STATS_ENABLE) && (CONFIG_MDS_KERNEL_STATS_ENABLE != 0))
    memheap->size.cur += ((uintptr_t)(node->next) - (uintptr_t)(node)) - nodeSize;
    if (memheap->size.cur > memheap->size.max) {
        memheap->size.max = memheap->size.cur;
    }
#endif

    if ((uintptr_t)node < (uintptr_t)(memheap->begin)) {
        MemHeapLLFF_RelistFree(memheap, node->next);
    }

    return (node);
}

static void *MDS_MemHeapLLFF_Realloc(MDS_MemHeap_t *memheap, void *ptr, size_t size)
{
    size_t alignSize = VALUE_ALIGN(size + MDS_SYSMEM_ALIGN_SIZE - 1, MDS_SYSMEM_ALIGN_SIZE);
    size_t totalSize = MemHeapLLFF_Size(memheap);
    if ((!MemHeapLLFF_IsPtrInside(memheap, ptr)) || (alignSize > totalSize)) {
        MDS_LOG_E("[memory] memheap(%p) realloc ptr(%p) size:%zu error of total:%zu", memheap, ptr,
                  alignSize, totalSize);
        return (NULL);
    }

    MemHeapLLFF_Node_t *node = (MemHeapLLFF_Node_t *)((uint8_t *)(ptr) -
                                                      sizeof(MemHeapLLFF_Node_t));
    MemHeapLLFF_Node_t *next = MemHeapLLFF_NodeRealloc(memheap, node, alignSize);

    return ((next != NULL) ? ((uint8_t *)next + sizeof(MemHeapLLFF_Node_t)) : (NULL));
}

static void MDS_MemHeapLLFF_Size(MDS_MemHeap_t *memheap, MDS_MemHeapSize_t *size)
{
#if (defined(CONFIG_MDS_KERNEL_STATS_ENABLE) && (CONFIG_MDS_KERNEL_STATS_ENABLE != 0))
    if (size != NULL) {
        *size = memheap->size;
    }
#else
    UNUSED(memheap);
    UNUSED(size);
#endif
}

const MDS_MemHeapOps_t G_MDS_MEMHEAP_OPS_LLFF = {
    .setup = MDS_MemHeapLLFF_Setup,
    .alloc = MDS_MemHeapLLFF_Alloc,
    .realloc = MDS_MemHeapLLFF_Realloc,
    .free = MDS_MemHeapLLFF_Free,
    .size = MDS_MemHeapLLFF_Size,
};
