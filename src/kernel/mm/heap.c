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
#include "mds/sys.h"

/* Define ------------------------------------------------------------------ */
MDS_LOG_MODULE_DECLARE(kernel, CONFIG_MDS_KERNEL_LOG_LEVEL);

/* Function ---------------------------------------------------------------- */
MDS_Err_t MDS_MemHeapInit(MDS_MemHeap_t *memheap, const char *name, void *base, size_t size,
                          const MDS_MemHeapOps_t *ops)
{
    MDS_ASSERT(memheap != NULL);
    MDS_ASSERT(ops != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(memheap->object), MDS_OBJECT_TYPE_MEMHEAP, name);
    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        return (err);
    }

    do {
        err = MDS_SemaphoreInit(&(memheap->lock), name, 1, 1);
        if (!MDS_ErrIsSame(err, MDS_EOK)) {
            break;
        }

        if ((ops != NULL) && (ops->setup != NULL)) {
            err = ops->setup(memheap, base, size);
        } else {
            err = MDS_ENOENT;
        }

        if (MDS_ErrIsSame(err, MDS_EOK)) {
            memheap->ops = ops;
#if (defined(CONFIG_MDS_KERNEL_STATS_ENABLE) && (CONFIG_MDS_KERNEL_STATS_ENABLE != 0))
            memheap->size.max = 0;
            memheap->size.cur = 0;
            memheap->size.total = size;
#endif
        } else {
            MDS_SemaphoreDeInit(&(memheap->lock));
        }
    } while (0);

    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_ObjectDeInit(&(memheap->object));
    }

    return (err);
}

MDS_Err_t MDS_MemHeapDeInit(MDS_MemHeap_t *memheap)
{
    MDS_ASSERT(memheap != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memheap->object)) == MDS_OBJECT_TYPE_MEMHEAP);

    MDS_Err_t err = MDS_SemaphoreDeInit(&(memheap->lock));
    if (MDS_ErrIsSame(err, MDS_EOK)) {
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

    MDS_Err_t err = MDS_SemaphoreAcquire(&(memheap->lock), MDS_TIMEOUT_FOREVER);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
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

    MDS_Err_t err = MDS_SemaphoreAcquire(&(memheap->lock), MDS_TIMEOUT_FOREVER);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
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

    MDS_Err_t err = MDS_SemaphoreAcquire(&(memheap->lock), MDS_TIMEOUT_FOREVER);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
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
        memset(pbuf, 0, nmemb * size);
    }

    return (pbuf);
}

void MDS_MemHeapStatus(MDS_MemHeap_t *memheap, MDS_MemHeapSize_t *size)
{
    MDS_ASSERT(memheap != NULL);
    MDS_ASSERT(MDS_ObjectGetType(&(memheap->object)) == MDS_OBJECT_TYPE_MEMHEAP);

    if ((memheap->ops != NULL) && (memheap->ops->size != NULL)) {
        memheap->ops->size(memheap, size);
    }
}

/* SysMem ------------------------------------------------------------------ */
#if ((defined(CONFIG_MDS_SYSMEM_HEAP_SIZE) && (CONFIG_MDS_SYSMEM_HEAP_SIZE > 0)) &&                \
     (defined(CONFIG_MDS_SYSMEM_HEAP_OPS) && (CONFIG_MDS_SYSMEM_HEAP_OPS > 0)))
static __attribute__((
    section(CONFIG_MDS_SYSMEM_HEAP_SECTION))) uint8_t g_sysHeapMem[CONFIG_MDS_SYSMEM_HEAP_SIZE];
static struct {
    MDS_MemHeap_t memheap;
    MDS_SpinLock_t spinlock;
} g_sysHeap;

static MDS_Err_t MDS_SysMemInit(void)
{
    MDS_Err_t err = MDS_EOK;

    MDS_Lock_t lock = MDS_CriticalLock(&(g_sysHeap.spinlock));
    do {
        MDS_ObjectType_t type = MDS_ObjectGetType(&(g_sysHeap.memheap.object));
        if (type == MDS_OBJECT_TYPE_MEMHEAP) {
            break;
        }

        err = MDS_MemHeapInit(&(g_sysHeap.memheap), "sysheap", g_sysHeapMem,
                              CONFIG_MDS_SYSMEM_HEAP_SIZE, &MDS_SYSMEM_HEAP_OPS);

        if (!MDS_ErrIsSame(err, MDS_EOK)) {
            MDS_LOG_E("[memory] SysMemInit failed");
        }
    } while (0);
    MDS_CriticalRestore(&(g_sysHeap.spinlock), lock);

    return (err);
}

void MDS_SysMemFree(void *ptr)
{
    if (!MDS_ErrIsSame(MDS_SysMemInit(), MDS_EOK)) {
        return;
    }

    MDS_MemHeapFree(&(g_sysHeap.memheap), ptr);
}

void *MDS_SysMemAlloc(size_t size)
{
    if (!MDS_ErrIsSame(MDS_SysMemInit(), MDS_EOK)) {
        return (NULL);
    }

    return (MDS_MemHeapAlloc(&(g_sysHeap.memheap), size));
}

void *MDS_SysMemCalloc(size_t nmemb, size_t size)
{
    if (!MDS_ErrIsSame(MDS_SysMemInit(), MDS_EOK)) {
        return (NULL);
    }

    return (MDS_MemHeapCalloc(&(g_sysHeap.memheap), nmemb, size));
}

void *MDS_SysMemRealloc(void *ptr, size_t size)
{
    if (!MDS_ErrIsSame(MDS_SysMemInit(), MDS_EOK)) {
        return (NULL);
    }

    return (MDS_MemHeapRealloc(&(g_sysHeap.memheap), ptr, size));
}
#else
void MDS_SysMemFree(void *ptr)
{
    UNUSED(ptr);

    MDS_ASSERT("CONFIG_MDS_SYSMEM_HEAP_OPS is MDS_MEMHEAP_OPS_NONE");
}

void *MDS_SysMemAlloc(size_t size)
{
    UNUSED(size);

    MDS_ASSERT("CONFIG_MDS_SYSMEM_HEAP_OPS is MDS_MEMHEAP_OPS_NONE");

    return (NULL);
}

void *MDS_SysMemCalloc(size_t nmemb, size_t size)
{
    UNUSED(nmemb);
    UNUSED(size);

    MDS_ASSERT("CONFIG_MDS_SYSMEM_HEAP_OPS is MDS_MEMHEAP_OPS_NONE");

    return (NULL);
}

void *MDS_SysMemRealloc(void *ptr, size_t size)
{
    UNUSED(ptr);
    UNUSED(size);

    MDS_ASSERT("CONFIG_MDS_SYSMEM_HEAP_OPS is MDS_MEMHEAP_OPS_NONE");

    return (NULL);
}
#endif
