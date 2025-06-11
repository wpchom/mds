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
#include "mds_def.h"
#include "mds_log.h"

/* Memory ------------------------------------------------------------------ */
bool MDS_MemAddrIsAligned(const void *addr, uintptr_t align)
{
    if ((align > sizeof(uint8_t)) && ((((uintptr_t)(addr)) & (align - 1)) == 0)) {
        return (true);
    }

    return (false);
}

void *MDS_MemBuffSet(void *dst, int c, size_t size)
{
    uint8_t *ptr = dst;
    uint8_t *end = ptr + size;

#if (defined(CONFIG_MDS_LIBRARY_MINIABLE) && (CONFIG_MDS_LIBRARY_MINIABLE != 0))
    while (ptr < end) {
        *ptr++ = c;
    }
#else
    while (ptr < end) {
        if ((((uintptr_t)end - (uintptr_t)ptr) >= sizeof(size_t)) &&
            (((uintptr_t)ptr - (uintptr_t)dst) >= sizeof(size_t)) &&
            MDS_MemAddrIsAligned(ptr, sizeof(size_t))) {
            uint8_t *align = ptr - sizeof(size_t);
            while (((uintptr_t)end - (uintptr_t)ptr) >= sizeof(size_t)) {
                *((size_t *)ptr) = *((size_t *)align);
                ptr += sizeof(size_t);
            }
        } else {
            *ptr++ = c;
        }
    }
#endif

    return (dst);
}

size_t MDS_MemBuffCopy(void *dst, size_t size, const void *src, size_t len)
{
    uint8_t *d = dst;
    const uint8_t *s = src;
    const uint8_t *e = s + ((len < size) ? (len) : (size));

#if (defined(CONFIG_MDS_LIBRARY_MINIABLE) && (CONFIG_MDS_LIBRARY_MINIABLE != 0))
    while (s < e) {
        *d++ = *s++;
    }
#else
    while (s < e) {
        if (MDS_MemAddrIsAligned(d, sizeof(size_t)) && MDS_MemAddrIsAligned(s, sizeof(size_t)) &&
            (((uintptr_t)(e - s)) >= sizeof(size_t))) {
            while (((uintptr_t)(e - s)) >= sizeof(size_t)) {
                *((size_t *)d) = *((size_t *)s);
                d += sizeof(size_t);
                s += sizeof(size_t);
            }
        } else {
            *d++ = *s++;
        }
    }
#endif

    return ((uintptr_t)d - (uintptr_t)dst);
}

void *MDS_MemBuffCpy(void *dst, const void *src, size_t size)
{
    MDS_MemBuffCopy(dst, size, src, size);

    return (dst);
}

void *MDS_MemBuffCcpy(void *dst, const void *src, int c, size_t size)
{
    uint8_t *d = dst;
    const uint8_t *s = src;

    while (size > 0) {
        if ((*d++ = *s++) == c) {
            return (dst);
        }
        size -= sizeof(uint8_t);
    }

    return (NULL);
}

int MDS_MemBuffCmp(const void *buf1, const void *buf2, size_t size)
{
    int cmp = 0;
    const uint8_t *p1 = buf1;
    const uint8_t *p2 = buf2;

    while ((size > 0) && (cmp == 0)) {
        cmp = *p1++ - *p2++;
        size -= sizeof(uint8_t);
    }

    return (cmp);
}
