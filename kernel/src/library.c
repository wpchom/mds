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

/* Skip List --------------------------------------------------------------- */
void MDS_SkipListInitNode(MDS_ListNode_t node[], size_t size)
{
    MDS_ASSERT(node != NULL);

    for (size_t level = 0; level < size; level++) {
        MDS_ListInitNode(&(node[level]));
    }
}

void MDS_SkipListRemoveNode(MDS_ListNode_t node[], size_t size)
{
    MDS_ASSERT(node != NULL);
    UNUSED(size);

    for (size_t level = 0; level < size; level++) {
        MDS_ListRemoveNode(&(node[level]));
    }
}

bool MDS_SkipListIsEmpty(MDS_ListNode_t node[], size_t size)
{
    MDS_ASSERT(node != NULL);
    UNUSED(size);

    return (MDS_ListIsEmpty(&(node[0])));
}

MDS_ListNode_t *MDS_SkipListSearchNode(MDS_ListNode_t list[], size_t size, const void *value,
                                       int (*compare)(const MDS_ListNode_t *node, const void *value))
{
    MDS_ASSERT(list != NULL);
    MDS_ASSERT(size > 0);
    MDS_ASSERT(compare != NULL);

    MDS_ListNode_t *skip = list;

    for (size_t level = 0; level < size; level++) {
        for (; skip != list[level].prev; skip = skip->next) {
            const MDS_ListNode_t *node = skip->next - level;
            if (compare(node, value) > 0) {
                break;
            }
        }
        if (level != (size - 1)) {
            skip = skip + 1;
        }
    }

    return (skip - size + 1);
}

size_t MDS_SkipListInsertNode(MDS_ListNode_t skip[], MDS_ListNode_t node[], size_t size, size_t rand, size_t shift)
{
    MDS_ASSERT(skip != NULL);
    MDS_ASSERT(node != NULL);
    MDS_ASSERT(size > 0);

     size_t level = 1;

    do {
        MDS_ListInsertNodeNext(&(skip[size - level]), &(node[size - level]));
        rand >>= shift;
    } while ((++level <= size) && !(rand & ((1UL << shift) - 1U)));

    return (level);
}

/* Tree -------------------------------------------------------------------- */
void MDS_TreeInitNode(MDS_TreeNode_t *node)
{
    MDS_ASSERT(node != NULL);

    node->parent = NULL;
    MDS_ListInitNode(&(node->child));
    MDS_ListInitNode(&(node->sibling));
}

MDS_TreeNode_t *MDS_TreeInsertNode(MDS_TreeNode_t *parent, MDS_TreeNode_t *node)
{
    MDS_TreeNode_t *tree = parent;

    if (tree == NULL) {
        tree = node;
    } else if (node != NULL) {
        MDS_ListInsertNodeNext(&(tree->child), &(node->sibling));
        node->parent = tree;
    }

    return (tree);
}

MDS_TreeNode_t *MDS_TreeRemoveNode(MDS_TreeNode_t *node)
{
    if ((node == NULL) || (!MDS_ListIsEmpty(&(node->child)))) {
        return (NULL);
    }

    MDS_ListRemoveNode(&(node->sibling));
    node->parent = NULL;

    return (node);
}

size_t MDS_TreeForeachNode(const MDS_TreeNode_t *tree, void (*func)(const MDS_TreeNode_t *, MDS_Arg_t *),
                           MDS_Arg_t *arg)
{
    if (tree == NULL) {
        return (0);
    }

    if (func != NULL) {
        func(tree, arg);
    }

    size_t cnt = 1;
    MDS_TreeNode_t *iter = NULL;
    MDS_LIST_FOREACH_NEXT (iter, sibling, &(tree->child)) {
        cnt += MDS_TreeForeachNode(iter, func, arg);
    }

    return (cnt);
}

/* Message ----------------------------------------------------------------- */
size_t MDS_MsgListGetLength(const MDS_MsgList_t *msg)
{
     size_t len = 0;
    const MDS_MsgList_t *cur = msg;

    while (cur != NULL) {
        len += cur->len;
        cur = cur->next;
    }

    return (len);
}

size_t MDS_MsgListCopyBuff(void *buff, size_t size, const MDS_MsgList_t *msg)
{
    MDS_ASSERT(buff != NULL);

     size_t len = 0;

    for (const MDS_MsgList_t *cur = msg; cur != NULL; cur = cur->next) {
        size_t cnt = MDS_MemBuffCopy((uint8_t *)buff + len, size, cur->buff, cur->len);
        if (cnt == 0) {
            break;
        }
        len += cnt;
        size -= cnt;
    }

    return (len);
}

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

#if (defined(MDS_LIB_MINIABLE) && (MDS_LIB_MINIABLE > 0))
    while (ptr < end) {
        *ptr++ = c;
    }
#else
    while (ptr < end) {
        if ((((uintptr_t)end - (uintptr_t)ptr) >= sizeof(size_t)) &&
            (((uintptr_t)ptr - (uintptr_t)dst) >= sizeof(size_t)) && MDS_MemAddrIsAligned(ptr, sizeof(size_t))) {
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

#if (defined(MDS_LIB_MINIABLE) && (MDS_LIB_MINIABLE > 0))
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

/* String ------------------------------------------------------------------ */
size_t MDS_StrAscLetterLength(const char *str)
{
    size_t cnt = 0;

    while (*str != '\0') {
        if ((('0' <= *str) && (*str <= '9')) || (('A' <= *str) && (*str <= 'Z')) || (('a' <= *str) && (*str <= 'z'))) {
            str += 1;
        } else {
            break;
        }
    }

    return (cnt);
}

size_t MDS_StrAscNumberLength(const char *str, int base)
{
    size_t cnt = 0;

    while (*str != '\0') {
        int val = 0;
        if (*str >= 'a') {
            val = *str - 'a' + 0x0A;
        } else if (*str >= 'A') {
            val = *str - 'A' + 0x0A;
        } else {
            val = *str - '0';
        }

        if ((val < 0) || (val > base)) {
            break;
        }
        cnt += 1;
        str += 1;
    }

    return (cnt);
}

size_t MDS_StrAsc2Hex(uint8_t *hex, size_t size, const char *asc, bool leftAlign)
{
    size_t cnt = 0;
    size_t len = MDS_StrAscNumberLength(asc, MDS_NUM_HEX_BASE);
    int odd = (!leftAlign && ((len & 0x01) != 0)) ? (1) : (0);

    for (size_t ofs = 0; (ofs < len) && (cnt < size); ofs++) {
        int val = (asc[ofs] >= 'a') ? (asc[ofs] - 'a' + 0x0A)
                                    : ((asc[ofs] >= 'A') ? (asc[ofs] - 'A' + 0x0A) : (asc[ofs] - '0'));
        if (odd == 0) {
            hex[cnt] = (val << (MDS_BITS_OF_BYTE >> 1));
        } else {
            hex[cnt] = (ofs > 0) ? (hex[cnt] | val) : (val);
            cnt++;
        }
        odd ^= 1;
    }

    return ((odd) ? (cnt + 1) : (cnt));
}

size_t MDS_StrHex2Asc(char *asc, size_t size, const uint8_t *hex, size_t len, bool lowerCase)
{
    const char caseUL = (lowerCase) ? ('a') : ('A');
    size_t idx = 0;
    size_t cnt = 0;

    if (size > 0) {
        while ((idx < len) && (cnt < (size - 1))) {
            uint8_t val = ((cnt & 0x01) == 0U) ? (hex[idx] >> (MDS_BITS_OF_BYTE >> 1)) : (hex[idx++] & 0x0FU);
            asc[cnt++] = (val >= 0x0A) ? (val - 0x0A + caseUL) : (val + '0');
        }
        asc[cnt] = '\0';
    }

    return (cnt);
}

/* Time ----------------------------------------------------------------- */
#define TIME_UNIX_YEAR_BEGIN    1970
#define TIME_UNIX_WEEKDAY_BEGIN 4

#define TIME_IS_LEAPYEAR(year)                                                                                         \
    (((((year) % 400) == 0) || ((((year) % 4) == 0) && (((year) % 100) != 0))) ? (true) : (false))

#define TIME_GET_YEARDAYS(year)                                                                                        \
    ((((year) - (TIME_UNIX_YEAR_BEGIN) + (1)) / 4) + (((year) - (TIME_UNIX_YEAR_BEGIN) - (1)) / 100) +                 \
     (((year) - (TIME_UNIX_YEAR_BEGIN) + (299)) / 400))

static const int16_t G_YDAYS_OF_MONTH[] = {-1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364};

MDS_Time_t MDS_TIME_ChangeTimeStamp(MDS_TimeDate_t *tm, int8_t tz)
{
    MDS_Time_t tmp = ((tm->year - TIME_UNIX_YEAR_BEGIN) * MDS_TIME_DAY_OF_YEAR) + TIME_GET_YEARDAYS(tm->year);

    tm->yday = tm->mday + G_YDAYS_OF_MONTH[tm->month - 1];
    if ((tm->month > MDS_TIME_MONTH_FEB) && (TIME_IS_LEAPYEAR(tm->year))) {
        tm->yday += 1;
    }

    tmp = (tmp + tm->yday) * MDS_TIME_SEC_OF_DAY + MDS_TIME_SEC_OF_HOUR * (tz + tm->hour) +
          MDS_TIME_SEC_OF_MIN * tm->minute + tm->second;

    return (tmp * MDS_TIME_MSEC_OF_SEC + tm->msec);
}

void MDS_TIME_ChangeTimeDate(MDS_TimeDate_t *tm, MDS_Time_t timestamp, int8_t tz)
{
    MDS_Time_t tmp = timestamp + (MDS_Time_t)tz * MDS_TIME_MSEC_OF_SEC * MDS_TIME_SEC_OF_HOUR;

    tm->msec = tmp % MDS_TIME_MSEC_OF_SEC;
    tmp /= MDS_TIME_MSEC_OF_SEC;
    tm->second = tmp % MDS_TIME_SEC_OF_MIN;
    tmp /= MDS_TIME_SEC_OF_MIN;
    tm->minute = tmp % MDS_TIME_MIN_OF_HOUR;
    tmp /= MDS_TIME_MIN_OF_HOUR;
    tm->hour = tmp % MDS_TIME_HOUR_OF_DAY;
    tmp /= MDS_TIME_HOUR_OF_DAY;
    tm->wday = (TIME_UNIX_WEEKDAY_BEGIN + tmp) % MDS_TIME_DAY_OF_WEEK;
    for (tm->year = TIME_UNIX_YEAR_BEGIN; tmp > 0; (tm->year)++) {
        int16_t yday = (TIME_IS_LEAPYEAR(tm->year)) ? (MDS_TIME_DAY_OF_YEAR + 1) : (MDS_TIME_DAY_OF_YEAR);
        if (tmp >= yday) {
            tmp -= yday;
        } else {
            break;
        }
    }
    tm->yday = tmp + 1;
    for (tm->month = MDS_TIME_MONTH_DEC; tm->month >= MDS_TIME_MONTH_JAN; tm->month -= 1) {
        int16_t yday = G_YDAYS_OF_MONTH[tm->month - 1];
        if ((tm->month > MDS_TIME_MONTH_FEB) && TIME_IS_LEAPYEAR(tm->year)) {
            yday += 1;
        }
        if (yday < tmp) {
            tmp -= yday;
            break;
        }
    }
    tm->mday = tmp;
}

MDS_Time_t MDS_TIME_DiffTimeMs(MDS_TimeDate_t *tm1, MDS_TimeDate_t *tm2)
{
    MDS_Time_t ts1 = MDS_TIME_ChangeTimeStamp(tm1, 0);
    MDS_Time_t ts2 = MDS_TIME_ChangeTimeStamp(tm2, 0);

    return (ts1 - ts2);
}
