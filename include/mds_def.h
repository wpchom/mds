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

#ifdef CONFIG_MDS_CONFIG_FILE
#include CONFIG_MDS_CONFIG_FILE
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
#if (defined(CONFIG_MDS_TICK_U64) && (CONFIG_MDS_TICK_U64))
typedef uint64_t MDS_Tick_t;
#else
typedef uint32_t MDS_Tick_t;
#endif

typedef struct MDS_TimeStamp {
    int64_t ts;
} MDS_TimeStamp_t;

typedef struct MDS_Lock {
    intptr_t key;
} MDS_Lock_t;

typedef struct MDS_Mask {
    uintptr_t mask;
} MDS_Mask_t;

typedef struct MDS_Timeout {
    MDS_Tick_t ticks;
} MDS_Timeout_t;

typedef union MDS_Arg {
    void *arg;
} MDS_Arg_t;

typedef enum MDS_Err {
    MDS_EOK = 0,
    MDS_EPERM = -1,      // Operation not permitted
    MDS_ENOENT = -2,     // No such file or directory
    MDS_EINTR = -4,      // Interrupted system call
    MDS_EIO = -5,        // I/O error
    MDS_EAGAIN = -11,    // Try again
    MDS_ENOMEM = -12,    // Out of memory
    MDS_EACCES = -13,    // Permission denied
    MDS_EFAULT = -14,    // Bad address
    MDS_EBUSY = -16,     // Device or resource busy
    MDS_EEXIST = -17,    // File exists
    MDS_ENODEV = -19,    // No such device
    MDS_ENOTDIR = -20,   // Not a directory
    MDS_EISDIR = -21,    // Is a directory
    MDS_EINVAL = -22,    // Invalid argument
    MDS_ERANGE = -34,    // Math result not representable
    MDS_ETIMEOUT = -62,  // Timer expired

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

/* Linked List ------------------------------------------------------------- */
typedef struct MDS_SListNode {
    struct MDS_SListNode *next;
} MDS_SListNode_t;

typedef struct MDS_DListNode {
    struct MDS_DListNode *prev, *next;
} MDS_DListNode_t;

#define MDS_DLIST_INIT(list) {.next = &(list), .prev = &(list)}

static inline void MDS_DListInitNode(MDS_DListNode_t *node)
{
    node->prev = node;
    node->next = node;
}

static inline void MDS_DListInsertNodeNext(MDS_DListNode_t *list, MDS_DListNode_t *node)
{
    node->prev = list;
    node->next = list->next;
    list->next->prev = node;
    list->next = node;
}

static inline void MDS_DListInsertNodePrev(MDS_DListNode_t *list, MDS_DListNode_t *node)
{
    node->prev = list->prev;
    node->next = list;
    list->prev->next = node;
    list->prev = node;
}

static inline void MDS_DListRemoveNode(MDS_DListNode_t *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = node;
    node->next = node;
}

static inline bool MDS_DListIsEmpty(const MDS_DListNode_t *node)
{
    return (node->prev == node);
}

static inline size_t MDS_DListGetCount(const MDS_DListNode_t *list)
{
    size_t cnt = 0;

    for (MDS_DListNode_t *node = list->next; node != list; node = node->next) {
        cnt += 1;
    }

    return (cnt);
}

static inline MDS_DListNode_t *MDS_DListForeachNext(const MDS_DListNode_t *list,
                                                    int (*cmp)(const MDS_DListNode_t *, void *),
                                                    void *arg)
{
    for (MDS_DListNode_t *node = list->next, *next = node->next; (node != list) && (node != NULL);
         node = next, next = node->next) {
        if (cmp(node, arg) == 0) {
            return (node);
        }
    }

    return (NULL);
}

static inline MDS_DListNode_t *MDS_DListForeachPrev(const MDS_DListNode_t *list,
                                                    int (*cmp)(const MDS_DListNode_t *, void *),
                                                    void *arg)
{
    for (MDS_DListNode_t *node = list->prev, *prev = node->prev; (node != list) && (node != NULL);
         node = prev, prev = node->prev) {
        if (cmp(node, arg) == 0) {
            return (node);
        }
    }

    return (NULL);
}

#define MDS_LIST_FOREACH_NEXT(iter, member, head)                                                 \
    for ((iter) = CONTAINER_OF((head)->next, __typeof__(*(iter)), member);                        \
         &((iter)->member) != (head);                                                             \
         (iter) = CONTAINER_OF((iter)->member.next, __typeof__(*(iter)), member))

#define MDS_LIST_FOREACH_PREV(iter, member, head)                                                 \
    for ((iter) = CONTAINER_OF((head)->prev, __typeof__(*(iter)), member);                        \
         &((iter)->member) != (head);                                                             \
         (iter) = CONTAINER_OF((iter)->member.prev, __typeof__(*(iter)), member))

/* Skip List --------------------------------------------------------------- */
void MDS_SkipListInitNode(MDS_DListNode_t node[], size_t size);
void MDS_SkipListRemoveNode(MDS_DListNode_t node[], size_t size);
bool MDS_SkipListIsEmpty(MDS_DListNode_t node[], size_t size);
MDS_DListNode_t *MDS_SkipListSearchNode(
    MDS_DListNode_t *last[], MDS_DListNode_t list[], size_t size, const void *value,
    int (*compare)(const MDS_DListNode_t *node, const void *value));
size_t MDS_SkipListInsertNode(MDS_DListNode_t *last[], MDS_DListNode_t node[], size_t size,
                              size_t rand, size_t shift);

/* Tree -------------------------------------------------------------------- */
typedef struct MDS_TreeNode {
    struct MDS_TreeNode *parent;
    MDS_DListNode_t child;
    MDS_DListNode_t sibling;
} MDS_TreeNode_t;

void MDS_TreeInitNode(MDS_TreeNode_t *node);
MDS_TreeNode_t *MDS_TreeInsertNode(MDS_TreeNode_t *parent, MDS_TreeNode_t *node);
MDS_TreeNode_t *MDS_TreeRemoveNode(MDS_TreeNode_t *node);
size_t MDS_TreeForeachNode(const MDS_TreeNode_t *tree,
                           void (*func)(const MDS_TreeNode_t *, MDS_Arg_t *), MDS_Arg_t *arg);

/* Message ----------------------------------------------------------------- */
typedef struct MDS_MsgList {
    const void *buff;
    size_t len;
    struct MDS_MsgList *next;
} MDS_MsgList_t;

size_t MDS_MsgListGetLength(const MDS_MsgList_t *msg);
size_t MDS_MsgListCopyBuff(void *buff, size_t size, const MDS_MsgList_t *msg);

/* Memory ------------------------------------------------------------------ */
bool MDS_MemAddrIsAligned(const void *address, uintptr_t align);
void *MDS_MemBuffSet(void *dst, int c, size_t len);
size_t MDS_MemBuffCopy(void *dst, size_t size, const void *src, size_t len);
void *MDS_MemBuffCpy(void *dst, const void *src, size_t size);
void *MDS_MemBuffCcpy(void *dst, const void *src, int c, size_t size);
int MDS_MemBuffCmp(const void *buf1, const void *buf2, size_t size);

/* String ------------------------------------------------------------------ */
size_t MDS_StrAscLetterLength(const char *str);
size_t MDS_StrAscNumberLength(const char *str, int base);
size_t MDS_StrAsc2Hex(uint8_t *hex, size_t size, const char *asc, bool rightAlign);
size_t MDS_StrHex2Asc(char *asc, size_t size, const uint8_t *hex, size_t len, bool lowCase);
size_t MDS_Strnlen(const char *str, size_t maxlen);
size_t MDS_Strlen(const char *str);
size_t MDS_Strlcpy(char *dst, const char *src, size_t dsize);
size_t MDS_Strlcat(char *dst, const char *src, size_t dsize);
unsigned long long MDS_Strtoull(const char *str, char **context, int base);
long long MDS_Strtoll(const char *str, char **context, int base);
unsigned long MDS_Strtoul(const char *str, char **context, int base);
long MDS_Strtol(const char *str, char **context, int base);

/* Format ------------------------------------------------------------------ */
int MDS_Vsnprintf(char *buff, size_t size, const char *fmt, va_list ap);
int MDS_Snprintf(char *buff, size_t size, const char *fmt, ...);
int MDS_Vsprintf(char *buff, const char *fmt, va_list ap);
int MDS_Sprintf(char *buff, const char *fmt, ...);
int MDS_Vsscanf(const char *buff, const char *fmt, va_list ap);
int MDS_Sscanf(const char *buff, const char *fmt, ...);

/* Time -------------------------------------------------------------------- */
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

MDS_TimeStamp_t MDS_TIME_ChangeTimeStamp(MDS_TimeDate_t *tm, int8_t tz);
void MDS_TIME_ChangeTimeDate(MDS_TimeDate_t *tm, MDS_TimeStamp_t timestamp, int8_t tz);
MDS_TimeStamp_t MDS_TIME_DiffTimeMs(MDS_TimeDate_t *tm1, MDS_TimeDate_t *tm2);

#ifdef __cplusplus
}
#endif

#endif /* __MDS_DEF_H__ */
