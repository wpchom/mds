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

/* Function ---------------------------------------------------------------- */
MDS_Err_t MDS_MemHeapInit(MDS_MemHeap_t *memheap, const char *name, void *begin, void *limit,
                          const MDS_MemHeapOps_t *ops)
{
    MDS_ASSERT(memheap != NULL);
    MDS_ASSERT(ops != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(memheap->object), MDS_OBJECT_TYPE_MEMHEAP, name);
    if (err != MDS_EOK) {
        return (err);
    }

    do {
        err = MDS_SemaphoreInit(&(memheap->lock), name, 0, 1);
        if (err != MDS_EOK) {
            break;
        }

        if ((memheap->ops != NULL) && (memheap->ops->setup != NULL)) {
            err = memheap->ops->setup(memheap, begin, limit);
        } else {
            err = MDS_ENOENT;
        }

        if (err == MDS_EOK) {
            memheap->ops = ops;
#if (defined(MDS_MEMHEAP_STATS) && (MDS_MEMHEAP_STATS > 0))
            memheap->size.max = 0;
            memheap->size.cur = 0;
            memheap->size.total = (uintptr_t)limit - (uintptr_t)begin;
#endif
        } else {
            MDS_SemaphoreDeInit(&(memheap->lock));
        }
    } while (0);

    if (err != MDS_EOK) {
        MDS_ObjectDeInit(&(memheap->object));
    }

    return (err);
}

MDS_Err_t MDS_MemHeapDeInit(MDS_MemHeap_t *memheap)
{
    MDS_ASSERT(memheap != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memheap->object)) == MDS_OBJECT_TYPE_MEMHEAP);

    MDS_Err_t err = MDS_SemaphoreDeInit(&(memheap->lock));
    if (err == MDS_EOK) {
        MDS_ObjectDeInit(&(memheap->object));
    }

    return (err);
}

void MDS_MemHeapFree(MDS_MemHeap_t *memheap, void *ptr)
{
    MDS_ASSERT(memheap != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memheap->object)) == MDS_OBJECT_TYPE_MEMHEAP);
    MDS_ASSERT(ptr != NULL);

    if (ptr == NULL) {
        return;
    }

    MDS_Err_t err = MDS_SemaphoreAcquire(&(memheap->lock), MDS_TICK_FOREVER);
    if (err == MDS_EOK) {
        if ((memheap->ops != NULL) && (memheap->ops->free != NULL)) {
            memheap->ops->free(memheap, ptr);
        }
    }

    MDS_SemaphoreRelease(&(memheap->lock));
}

void *MDS_MemHeapAlloc(MDS_MemHeap_t *memheap, size_t size)
{
    MDS_ASSERT(memheap != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memheap->object)) == MDS_OBJECT_TYPE_MEMHEAP);

    void *pbuf = NULL;

    MDS_Err_t err = MDS_SemaphoreAcquire(&(memheap->lock), MDS_TICK_FOREVER);
    if (err == MDS_EOK) {
        if ((memheap->ops != NULL) && (memheap->ops->alloc != NULL)) {
            pbuf = memheap->ops->alloc(memheap, size);
        }
    }

    MDS_SemaphoreRelease(&(memheap->lock));

    return (pbuf);
}

void *MDS_MemHeapRealloc(MDS_MemHeap_t *memheap, void *ptr, size_t size)
{
    MDS_ASSERT(memheap != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memheap->object)) == MDS_OBJECT_TYPE_MEMHEAP);

    if (ptr == NULL) {
        return (MDS_MemHeapAlloc(memheap, size));
    } else if (size == 0) {
        MDS_MemHeapFree(memheap, ptr);
        return (NULL);
    }

    void *pbuf = NULL;

    MDS_Err_t err = MDS_SemaphoreAcquire(&(memheap->lock), MDS_TICK_FOREVER);
    if (err == MDS_EOK) {
        if ((memheap->ops != NULL) && (memheap->ops->realloc != NULL)) {
            pbuf = memheap->ops->realloc(memheap, ptr, size);
        }
    }

    MDS_SemaphoreRelease(&(memheap->lock));

    return (pbuf);
}

void *MDS_MemHeapCalloc(MDS_MemHeap_t *memheap, size_t nmemb, size_t size)
{
    MDS_ASSERT((nmemb > 0) && (size > 0));

    void *pbuf = MDS_MemHeapAlloc(memheap, nmemb * size);
    if (pbuf != NULL) {
        MDS_MemBuffSet(pbuf, 0, nmemb * size);
    }

    return (pbuf);
}

extern void MDS_MemHeapSize(MDS_MemHeap_t *memheap, MDS_MemHeapSize_t *size)
{
    MDS_ASSERT(memheap != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memheap->object)) == MDS_OBJECT_TYPE_MEMHEAP);

    if ((memheap->ops != NULL) && (memheap->ops->size != NULL)) {
        memheap->ops->size(memheap, size);
    }
}

/* SysMem ------------------------------------------------------------------ */
static MDS_MemHeap_t g_sysMemHeap;

__attribute__((weak)) void MDS_SysMemAddress(void **heapBegin, void **heapLimit)
{
#if defined(__IAR_SYSTEMS_ICC__)
#pragma section(".heap")
    *heapBegin = __section_start(".heap");
    *heapLimit = __section_end(".heap");
#else
    extern uintptr_t __HeapBase;
    extern uintptr_t __HeapLimit;
    *heapBegin = &__HeapBase;
    *heapLimit = &__HeapLimit;
#endif
}

void MDS_SysMemInit(void)
{
    if (MDS_ObjectGetType(&(g_sysMemHeap.object)) == MDS_OBJECT_TYPE_MEMHEAP) {
        return;
    }

    void *heapBase, *heapLimit;
    MDS_SysMemAddress(&heapBase, &heapLimit);

    MDS_MemHeapInit(&g_sysMemHeap, "sysmem", heapBase, heapLimit, &G_MDS_MEMHEAP_OPS_LLFF);
}

void MDS_SysMemFree(void *ptr)
{
    MDS_SysMemInit();

    MDS_MemHeapFree(&g_sysMemHeap, ptr);
}

void *MDS_SysMemAlloc(size_t size)
{
    MDS_SysMemInit();

    return (MDS_MemHeapAlloc(&g_sysMemHeap, size));
}

void *MDS_SysMemCalloc(size_t nmemb, size_t size)
{
    MDS_SysMemInit();

    return (MDS_MemHeapCalloc(&g_sysMemHeap, nmemb, size));
}

void *MDS_SysMemRealloc(void *ptr, size_t size)
{
    MDS_SysMemInit();

    return (MDS_MemHeapRealloc(&g_sysMemHeap, ptr, size));
}

__attribute__((alias("MDS_SysMemFree"))) void free(void *ptr);
__attribute__((alias("MDS_SysMemAlloc"))) void *malloc(size_t size);
__attribute__((alias("MDS_SysMemCalloc"))) void *calloc(size_t nmemb, size_t size);
__attribute__((alias("MDS_SysMemRealloc"))) void *realloc(void *ptr, size_t size);
