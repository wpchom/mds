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
#ifndef __MDS_DEF_H__
#define __MDS_DEF_H__

/* Include ----------------------------------------------------------------- */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef int64_t MDS_Time_t;
typedef intptr_t MDS_Item_t;
typedef uintptr_t MDS_Mask_t;
typedef union MDS_Arg {
    void *arg;
} MDS_Arg_t;

typedef enum MDS_Err {
    MDS_EOK = 0,
    MDS_EPERM = -1,     // Operation not permitted
    MDS_ENOENT = -2,    // No such file or directory
    MDS_EINTR = -4,     // Interrupted system call
    MDS_EIO = -5,       // I/O error
    MDS_EAGAIN = -11,   // Try again
    MDS_ENOMEM = -12,   // Out of memory
    MDS_EACCES = -13,   // Permission denied
    MDS_EFAULT = -14,   // Bad address
    MDS_EBUSY = -16,    // Device or resource busy
    MDS_EEXIST = -17,   // File exists
    MDS_ENODEV = -19,   // No such device
    MDS_ENOTDIR = -20,  // Not a directory
    MDS_EISDIR = -21,   // Is a directory
    MDS_EINVAL = -22,   // Invalid argument
    MDS_ERANGE = -34,   // Math result not representable
    MDS_ETIME = -62,    // Timer expired

    MDS_ENODEF = -__INT_MAX__,
} MDS_Err_t;

/* Define ------------------------------------------------------------------ */
#define MDS_LOOP for (;;)

#define MDS_BITS_OF_BYTE 8U
#define MDS_NUM_BIN_BASE 2U
#define MDS_NUM_OCT_BASE 8U
#define MDS_NUM_DEC_BASE 10U
#define MDS_NUM_HEX_BASE 16U

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) ((sizeof(array)) / sizeof((array)[0]))
#endif

#ifndef OFFSET_OF
#define OFFSET_OF(T, x) ((size_t)(&((T *)0)->x))
#endif

#ifndef CONTAINER_OF
#define CONTAINER_OF(p, T, x) ((T *)(((char *)(p)) - OFFSET_OF(T, x)))
#endif

#ifndef VALUE_RANGE
#define VALUE_RANGE(val, min, max) (((val) <= (min)) ? (min) : (((val) >= (max)) ? (max) : (val)))
#endif

#ifndef VALUE_ALIGN
#define VALUE_ALIGN(val, align) ((val) & (~((align) - (1ULL))))
#endif

/* Data -------------------------------------------------------------------- */
static inline uint16_t MDS_GetU16BE(const uint8_t *array)
{
    return ((uint16_t)((((uint16_t)(array[0x00])) << (MDS_BITS_OF_BYTE * 0x01)) |
                       (((uint16_t)(array[0x01])) << (MDS_BITS_OF_BYTE * 0x00))));
}

static inline void MDS_PutU16BE(uint8_t *array, uint16_t value)
{
    array[0x00] = (uint8_t)(value >> (MDS_BITS_OF_BYTE * 0x01));
    array[0x01] = (uint8_t)(value >> (MDS_BITS_OF_BYTE * 0x00));
}

static inline uint16_t MDS_GetU16LE(const uint8_t *array)
{
    return ((uint16_t)((((uint16_t)(array[0x00])) << (MDS_BITS_OF_BYTE * 0x00)) |
                       (((uint16_t)(array[0x01])) << (MDS_BITS_OF_BYTE * 0x01))));
}

static inline void MDS_PutU16LE(uint8_t *array, uint16_t value)
{
    array[0x00] = (uint8_t)(value >> (MDS_BITS_OF_BYTE * 0x00));
    array[0x01] = (uint8_t)(value >> (MDS_BITS_OF_BYTE * 0x01));
}

static inline uint32_t MDS_GetU32BE(const uint8_t *array)
{
    return ((uint32_t)((((uint32_t)(array[0x00])) << (MDS_BITS_OF_BYTE * 0x03)) |
                       (((uint32_t)(array[0x01])) << (MDS_BITS_OF_BYTE * 0x02)) |
                       (((uint32_t)(array[0x02])) << (MDS_BITS_OF_BYTE * 0x01)) |
                       (((uint32_t)(array[0x03])) << (MDS_BITS_OF_BYTE * 0x00))));
}

static inline void MDS_PutU32BE(uint8_t *array, uint32_t value)
{
    array[0x00] = (uint8_t)(value >> (MDS_BITS_OF_BYTE * 0x03));
    array[0x01] = (uint8_t)(value >> (MDS_BITS_OF_BYTE * 0x02));
    array[0x02] = (uint8_t)(value >> (MDS_BITS_OF_BYTE * 0x01));
    array[0x03] = (uint8_t)(value >> (MDS_BITS_OF_BYTE * 0x00));
}

static inline uint32_t MDS_GetU32LE(const uint8_t *array)
{
    return ((uint32_t)((((uint32_t)(array[0x00])) << (MDS_BITS_OF_BYTE * 0x00)) |
                       (((uint32_t)(array[0x01])) << (MDS_BITS_OF_BYTE * 0x01)) |
                       (((uint32_t)(array[0x02])) << (MDS_BITS_OF_BYTE * 0x02)) |
                       (((uint32_t)(array[0x03])) << (MDS_BITS_OF_BYTE * 0x03))));
}

static inline void MDS_PutU32LE(uint8_t *array, uint32_t value)
{
    array[0x00] = (uint8_t)(value >> (MDS_BITS_OF_BYTE * 0x00));
    array[0x01] = (uint8_t)(value >> (MDS_BITS_OF_BYTE * 0x01));
    array[0x02] = (uint8_t)(value >> (MDS_BITS_OF_BYTE * 0x02));
    array[0x03] = (uint8_t)(value >> (MDS_BITS_OF_BYTE * 0x03));
}

/* Linked List ------------------------------------------------------------- */
typedef struct MDS_ListNode {
    struct MDS_ListNode *prev, *next;
} MDS_ListNode_t;

static inline void MDS_ListInitNode(MDS_ListNode_t *node)
{
    node->prev = node;
    node->next = node;
}

static inline void MDS_ListInsertNodeNext(MDS_ListNode_t *list, MDS_ListNode_t *node)
{
    node->prev = list;
    node->next = list->next;
    list->next->prev = node;
    list->next = node;
}

static inline void MDS_ListInsertNodePrev(MDS_ListNode_t *list, MDS_ListNode_t *node)
{
    node->prev = list->prev;
    node->next = list;
    list->prev->next = node;
    list->prev = node;
}

static inline void MDS_ListRemoveNode(MDS_ListNode_t *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = node;
    node->next = node;
}

static inline bool MDS_ListIsEmpty(const MDS_ListNode_t *node)
{
    return (node->prev == node);
}

extern size_t MDS_ListGetLength(const MDS_ListNode_t *list);

#define MDS_LIST_FOREACH_NEXT(iter, member, head)                                                                      \
    for ((iter) = CONTAINER_OF((head)->next, __typeof__(*(iter)), member); &((iter)->member) != (head);                \
         (iter) = CONTAINER_OF((iter)->member.next, __typeof__(*(iter)), member))

#define MDS_LIST_FOREACH_PREV(iter, member, head)                                                                      \
    for ((iter) = CONTAINER_OF((head)->prev, __typeof__(*(iter)), member); &((iter)->member) != (head);                \
         (iter) = CONTAINER_OF((iter)->member.prev, __typeof__(*(iter)), member))

/* Skip List --------------------------------------------------------------- */
extern void MDS_SkipListInitNode(MDS_ListNode_t node[], size_t size);
extern void MDS_SkipListRemoveNode(MDS_ListNode_t node[], size_t size);
extern bool MDS_SkipListIsEmpty(MDS_ListNode_t node[], size_t size);
extern MDS_ListNode_t *MDS_SkipListSearchNode(MDS_ListNode_t list[], size_t size, const void *value,
                                              int (*compare)(const MDS_ListNode_t *node, const void *value));
extern size_t MDS_SkipListInsertNode(MDS_ListNode_t skip[], MDS_ListNode_t node[], size_t size, size_t rand,
                                     size_t shift);

/* Tree -------------------------------------------------------------------- */
typedef struct MDS_TreeNode {
    struct MDS_TreeNode *parent;
    MDS_ListNode_t child;
    MDS_ListNode_t sibling;
} MDS_TreeNode_t;

extern void MDS_TreeInitNode(MDS_TreeNode_t *node);
extern MDS_TreeNode_t *MDS_TreeInsertNode(MDS_TreeNode_t *parent, MDS_TreeNode_t *node);
extern MDS_TreeNode_t *MDS_TreeRemoveNode(MDS_TreeNode_t *node);
extern size_t MDS_TreeForeachNode(const MDS_TreeNode_t *tree, void (*func)(const MDS_TreeNode_t *, MDS_Arg_t *),
                                  MDS_Arg_t *arg);

/* Message ----------------------------------------------------------------- */
typedef struct MDS_MsgList {
    const void *buff;
    size_t len;
    struct MDS_MsgList *next;
} MDS_MsgList_t;

extern size_t MDS_MsgListGetLength(const MDS_MsgList_t *msg);
extern size_t MDS_MsgListCopyBuff(void *buff, size_t size, const MDS_MsgList_t *msg);

/* Memory ------------------------------------------------------------------ */
extern bool MDS_MemAddrIsAligned(const void *address, uintptr_t align);
extern void *MDS_MemBuffSet(void *dst, int c, size_t len);
extern size_t MDS_MemBuffCopy(void *dst, size_t size, const void *src, size_t len);
extern void *MDS_MemBuffCcpy(void *dst, const void *src, int c, size_t size);

/* String ------------------------------------------------------------------ */
extern size_t MDS_StrAscLetterLength(const char *str);
extern size_t MDS_StrAscNumberLength(const char *str, int base);
extern size_t MDS_StrAsc2Hex(uint8_t *hex, size_t size, const char *asc, bool rightAlign);
extern size_t MDS_StrHex2Asc(char *asc, size_t size, const uint8_t *hex, size_t len, bool lowCase);

/* Time ----------------------------------------------------------------- */
#define MDS_TIME_NSEC_OF_SEC  1000000000
#define MDS_TIME_USEC_OF_SEC  1000000
#define MDS_TIME_MSEC_OF_SEC  1000
#define MDS_TIME_USEC_OF_MSEC 1000
#define MDS_TIME_SEC_OF_MIN   60
#define MDS_TIME_MSEC_OF_MIN  (MDS_TIME_MSEC_OF_SEC * MDS_TIME_SEC_OF_MIN)
#define MDS_TIME_MIN_OF_HOUR  60
#define MDS_TIME_SEC_OF_HOUR  (MDS_TIME_SEC_OF_MIN * MDS_TIME_MIN_OF_HOUR)
#define MDS_TIME_MSEC_OF_HOUR (MDS_TIME_MSEC_OF_SEC * MDS_TIME_SEC_OF_HOUR)
#define MDS_TIME_HOUR_OF_DAY  24
#define MDS_TIME_MIN_OF_DAY   (MDS_TIME_MIN_OF_HOUR * MDS_TIME_HOUR_OF_DAY)
#define MDS_TIME_SEC_OF_DAY   (MDS_TIME_SEC_OF_MIN * MDS_TIME_MIN_OF_DAY)
#define MDS_TIME_MSEC_OF_DAY  (MDS_TIME_MSEC_OF_SEC * MDS_TIME_SEC_OF_DAY)
#define MDS_TIME_DAY_OF_WEEK  7
#define MDS_TIME_DAY_OF_YEAR  365

typedef struct MDS_TimeDate {
    int16_t msec;   // microseconds [0~999]
    int8_t second;  // seconds [0~60]
    int8_t minute;  // minute [0~ 59]
    int8_t hour;    // hour [0~23]
    int8_t wday;    // day of week [0~6]
    int8_t mday;    // day of month [1~31]
    int8_t month;   // month [1~12]
    int16_t year;   // year [20xx]
    int16_t yday;   // days in year [0~365]
} MDS_TimeDate_t;

typedef enum MDS_TIME_Weekday {
    MDS_TIME_WEEKDAY_SUN,
    MDS_TIME_WEEKDAY_MON,
    MDS_TIME_WEEKDAY_TUE,
    MDS_TIME_WEEKDAY_WED,
    MDS_TIME_WEEKDAY_THU,
    MDS_TIME_WEEKDAY_FRI,
    MDS_TIME_WEEKDAY_SAT,
} MDS_TIME_Weekday_t;

typedef enum MDS_TIME_Month {
    MDS_TIME_MONTH_JAN = 1,
    MDS_TIME_MONTH_FEB,
    MDS_TIME_MONTH_MAR,
    MDS_TIME_MONTH_APR,
    MDS_TIME_MONTH_MAY,
    MDS_TIME_MONTH_JUN,
    MDS_TIME_MONTH_JUL,
    MDS_TIME_MONTH_AUG,
    MDS_TIME_MONTH_SEP,
    MDS_TIME_MONTH_OCT,
    MDS_TIME_MONTH_NOV,
    MDS_TIME_MONTH_DEC,
} MDS_TIME_Month_t;

extern MDS_Time_t MDS_TIME_ChangeTimeStamp(MDS_TimeDate_t *tm, int8_t tz);
extern void MDS_TIME_ChangeTimeDate(MDS_TimeDate_t *tm, MDS_Time_t timestamp, int8_t tz);
extern MDS_Time_t MDS_TIME_DiffTimeMs(MDS_TimeDate_t *tm1, MDS_TimeDate_t *tm2);

/* Init -------------------------------------------------------------------- */
#ifndef MDS_INIT_SECTION
#define MDS_INIT_SECTION ".mds.init."
#endif

#define MDS_INIT_PRIORITY_0 "0."
#define MDS_INIT_PRIORITY_1 "1."
#define MDS_INIT_PRIORITY_2 "2."
#define MDS_INIT_PRIORITY_3 "3."
#define MDS_INIT_PRIORITY_4 "4."
#define MDS_INIT_PRIORITY_5 "5."
#define MDS_INIT_PRIORITY_6 "6."
#define MDS_INIT_PRIORITY_7 "7."
#define MDS_INIT_PRIORITY_8 "8."
#define MDS_INIT_PRIORITY_9 "9."

typedef void (*MDS_InitFunc_t)(void);

#define MDS_INIT_IMPORT(priority, func)                                                                                \
    static const __attribute__((section(MDS_INIT_SECTION priority #func))) MDS_InitFunc_t __MDS_INIT_##func = (func)

static inline void MDS_InitPorting(void)
{
    static const __attribute__((section(MDS_INIT_SECTION "\000"))) void *initBegin = NULL;
    static const __attribute__((section(MDS_INIT_SECTION "\177"))) void *initLimit = NULL;
    const MDS_InitFunc_t *initStart = (const MDS_InitFunc_t *)((uintptr_t)(&initBegin) + sizeof(void *));
    const MDS_InitFunc_t *initEnd = (const MDS_InitFunc_t *)((uintptr_t)(&initLimit));

    for (const MDS_InitFunc_t *init = initStart; init < initEnd; init++) {
        if (*init != NULL) {
            (*init)();
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif /* __MDS_DEF_H__ */
