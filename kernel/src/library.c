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
    register uint8_t *ptr = dst;
    register uint8_t *end = ptr + size;

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
    register uint8_t *d = dst;
    register const uint8_t *s = src;
    register const uint8_t *e = s + ((len < size) ? (len) : (size));

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
    register uint8_t *d = dst;
    register const uint8_t *s = src;

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
    register int cmp = 0;
    register const uint8_t *p1 = buf1;
    register const uint8_t *p2 = buf2;

    while ((size > 0) && (cmp == 0)) {
        cmp = *p1++ - *p2++;
        size -= sizeof(uint8_t);
    }

    return (cmp);
}

/* Message List ------------------------------------------------------------ */
size_t MDS_MsgListGetLength(const MDS_MsgList_t *msg)
{
    register size_t len = 0;
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

    register size_t len = 0;

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

/* Linked List ------------------------------------------------------------- */
size_t MDS_ListGetLength(const MDS_ListNode_t *list)
{
    register size_t len = 0;
    const MDS_ListNode_t *node = list;

    while (node->next != list) {
        len += 1;
        node = node->next;
    }

    return (len);
}

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

    register size_t level = 1;

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

static unsigned char MDS_StrChar2Hex(const char ch)
{
    if (('a' <= ch) && (ch <= 'f')) {
        return ((unsigned char)(ch - 'a' + 0x0A));
    } else if (('A' <= ch) && (ch <= 'F')) {
        return ((unsigned char)(ch - 'A' + 0x0A));
    } else if (('0' <= ch) && (ch <= '9')) {
        return ((unsigned char)(ch - '0' + 0));
    }
    return ((unsigned char)(-1));
}

unsigned long long MDS_Strtoull(const char *str, char **context, int base)
{
    unsigned long long value = 0;

    while (*str != '\0') {
        unsigned char digit = MDS_StrChar2Hex(*str);
        if (digit >= base) {
            break;
        }
        value = value * base + digit;
        str++;
    }

    if (context != NULL) {
        *context = (char *)str;
    }

    return (value);
}

long long MDS_Strtoll(const char *str, char **context, int base)
{
    int sign = 0;

    while ((*str == '-') || (*str == '+')) {
        if (*str++ == '-') {
            sign ^= 1;
        }
    }

    long long value = MDS_Strtoull(str, context, base);

    return ((sign != 0) ? (-value) : (value));
}

unsigned long MDS_Strtoul(const char *str, char **context, int base)
{
    return ((unsigned long)MDS_Strtoull(str, context, base));
}

long MDS_Strtol(const char *str, char **context, int base)
{
    return ((long)MDS_Strtoll(str, context, base));
}

/* Format ------------------------------------------------------------------ */
enum FMT_FormatFlag {
    FMT_FLAG_LEFT = 0x0001U,
    FMT_FLAG_PLUS = 0x0002U,
    FMT_FLAG_ZERO = 0x0004U,
    FMT_FLAG_SPACE = 0x0008U,
    FMT_FLAG_HASH = 0x0010U,
    FMT_FLAG_PRECISION = 0x0020U,
    FMT_FLAG_CHAR = 0x0040U,
    FMT_FLAG_SHORT = 0x0080U,
    FMT_FLAG_LONG = 0x0100U,
    FMT_FLAG_LONG_LONG = 0x0200U,
    FMT_FLAG_UPCASE = 0x0400U,
    FMT_FLAG_NEGATIVE = 0x0800U,
    FMT_FLAG_IGNORE = 0x1000U,
    FMT_FLAG_SECURE = 0x8000U,
};

typedef struct FMT_Args {
    unsigned int base;
    unsigned int flags;
    size_t width;
    size_t precision;
} FMT_Args_t;

static const char *FMT_ParsePrintFormatFlags(const char *fmt, va_list *ap, FMT_Args_t *args)
{
    for (; *fmt != '\0'; fmt++) {
        if (*fmt == '-') {
            args->flags |= FMT_FLAG_LEFT;
        } else if (*fmt == '+') {
            args->flags |= FMT_FLAG_PLUS;
        } else if (*fmt == ' ') {
            args->flags |= FMT_FLAG_SPACE;
        } else if (*fmt == '#') {
            args->flags |= FMT_FLAG_HASH;
        } else if (*fmt == '0') {
            args->flags |= FMT_FLAG_ZERO;
        } else {
            break;
        }
    }

    int width = (*fmt == '*') ? (fmt++, va_arg(*ap, int)) : (MDS_Strtol(fmt, (char **)(&fmt), MDS_NUM_DEC_BASE));
    args->width = (width < 0) ? (args->flags |= FMT_FLAG_LEFT, -width) : (width);

    int prec = 0;
    if (*fmt == '.') {
        prec = ((*++fmt == '*') ? (fmt++, va_arg(*ap, int)) : (MDS_Strtol(fmt, (char **)(&fmt), MDS_NUM_DEC_BASE)));
    }
    args->precision = (prec > 0) ? (args->flags |= FMT_FLAG_PRECISION, (unsigned int)(prec)) : (0);

    if (*fmt == 'l') {
        args->flags |= (*++fmt == 'l') ? (fmt++, FMT_FLAG_LONG_LONG | FMT_FLAG_LONG) : (FMT_FLAG_LONG);
    } else if (*fmt == 'h') {
        args->flags |= (*++fmt == 'h') ? (fmt++, FMT_FLAG_SHORT | FMT_FLAG_CHAR) : (FMT_FLAG_SHORT);
    } else if (*fmt == 'X') {
        args->flags |= FMT_FLAG_UPCASE;
    }

    return (fmt);
}

static const char *FMT_ParseScanfFormatFlags(const char *fmt, FMT_Args_t *args, bool secure)
{
    args->flags = 0U;
    for (; *fmt != '\0'; fmt++) {
        if (*fmt == '*') {
            args->flags |= FMT_FLAG_IGNORE;
        } else {
            break;
        }
    }

    int width = MDS_Strtol(fmt, (char **)(&fmt), MDS_NUM_DEC_BASE);
    args->width = (width > 0) ? (width) : (0);

    if (*fmt == 'l') {
        args->flags |= (*++fmt == 'l') ? (fmt++, FMT_FLAG_LONG_LONG | FMT_FLAG_LONG) : (FMT_FLAG_LONG);
    } else if (*fmt == 'h') {
        args->flags |= (*++fmt == 'h') ? (fmt++, FMT_FLAG_SHORT | FMT_FLAG_CHAR) : (FMT_FLAG_SHORT);
    }

    if (secure) {
        args->flags |= FMT_FLAG_SECURE;
    }

    return (fmt);
}

static size_t FMT_PrintBuff(char *buff, size_t size, size_t pos, const char *str, size_t len, FMT_Args_t *args)
{
    size_t start = pos;

    if (((args->flags & (FMT_FLAG_LEFT | FMT_FLAG_ZERO)) == 0U)) {
        for (size_t idx = len; (idx < args->width) && (pos < size); idx++) {
            buff[pos++] = ' ';
        }
    }

    while ((pos < size) && (len-- > 0)) {
        buff[pos++] = (args->base != 0) ? (str[len]) : (*str++);
    }

    if (((args->flags & FMT_FLAG_LEFT) != 0U)) {
        for (size_t idx = pos - start; (idx < args->width) && (pos < size); idx++) {
            buff[pos++] = ' ';
        }
    }

    return (pos);
}

#define FMT_NTOA_BUFFER_SIZE (sizeof(long long) * MDS_BITS_OF_BYTE + 1)
static size_t FMT_PrintBuffIntergerPad(char *valbuf, size_t len, FMT_Args_t *args)
{
    if (((args->flags & FMT_FLAG_LEFT) == 0U) || (args->precision > 0)) {
        if ((args->width > 0) && ((args->flags & FMT_FLAG_ZERO) != 0U) &&
            ((args->flags & (FMT_FLAG_PLUS | FMT_FLAG_NEGATIVE | FMT_FLAG_SPACE)) != 0U)) {
            args->width--;
        }
        while (((len < args->precision) || (((args->flags & FMT_FLAG_ZERO) != 0U) && (len < args->width))) &&
               (len < FMT_NTOA_BUFFER_SIZE)) {
            valbuf[len++] = '0';
        }
    }

    return (len);
}

static size_t FMT_PrintBuffIntergerHash(char *valbuf, size_t len, FMT_Args_t *args)
{
    if ((args->flags & FMT_FLAG_HASH) && (len < FMT_NTOA_BUFFER_SIZE)) {
        if (((args->flags & FMT_FLAG_PRECISION) == 0U) && (len > 0) &&
            ((len == args->precision) || (len == args->width))) {
            if ((--len > 0) && (args->base == MDS_NUM_HEX_BASE)) {
                len--;
            }
        }
        if (args->base == MDS_NUM_HEX_BASE) {
            valbuf[len++] = ((args->flags & FMT_FLAG_UPCASE) != 0U) ? ('X') : ('x');
        } else if (args->base == MDS_NUM_BIN_BASE) {
            valbuf[len++] = 'b';
        }
        valbuf[len++] = '0';
    }

    return (len);
}

static void FMT_PrintBuffInterger(char *buff, size_t size, size_t *pos, char *valbuf, size_t len, FMT_Args_t *args)
{
    len = FMT_PrintBuffIntergerPad(valbuf, len, args);
    len = FMT_PrintBuffIntergerHash(valbuf, len, args);
    if (len < FMT_NTOA_BUFFER_SIZE) {
        if ((args->flags & FMT_FLAG_NEGATIVE) != 0U) {
            valbuf[len++] = '-';
        } else if ((args->flags & FMT_FLAG_PLUS) != 0U) {
            valbuf[len++] = '+';
        } else if ((args->flags & FMT_FLAG_SPACE) != 0U) {
            valbuf[len++] = ' ';
        }
    }

    *pos = FMT_PrintBuff(buff, size, *pos, valbuf, len, args);
}

static void FMT_lltoa(char *buff, size_t size, size_t *pos, unsigned long long value, FMT_Args_t *args)
{
    char valbuf[FMT_NTOA_BUFFER_SIZE];
    size_t len = 0;

    do {
        unsigned char digit = (char)(value % args->base);
        if (digit < MDS_NUM_DEC_BASE) {
            valbuf[len++] = (char)(digit + '0');
        } else {
            valbuf[len++] = (char)(digit - MDS_NUM_DEC_BASE + ((args->flags & FMT_FLAG_UPCASE) ? ('A') : ('a')));
        }
    } while (((value /= args->base) > 0) && (len < sizeof(valbuf)));

    FMT_PrintBuffInterger(buff, size, pos, valbuf, len, args);
}

static unsigned int FMT_PrintIntegerBase(const char ch)
{
    if ((ch == 'd') || (ch == 'i') || (ch == 'u')) {
        return (MDS_NUM_DEC_BASE);
    } else if (ch == 'b') {
        return (MDS_NUM_BIN_BASE);
    } else if (ch == 'o') {
        return (MDS_NUM_OCT_BASE);
    } else if ((ch == 'x') || (ch == 'X') || (ch == 'p')) {
        return (MDS_NUM_HEX_BASE);
    }
    return (0);
}

static void FMT_PrintInteger(char *buff, size_t size, size_t *pos, const char ch, va_list *ap, FMT_Args_t *args)
{
    unsigned long long value;

    if (ch == 'p') {
        value = (uintptr_t)va_arg(*ap, void *);
        args->flags |= FMT_FLAG_ZERO | FMT_FLAG_HASH;
    } else {
        long long tmp = (args->flags & FMT_FLAG_LONG_LONG)
                            ? (va_arg(*ap, long long))
                            : ((args->flags & FMT_FLAG_LONG) ? (va_arg(*ap, long)) : (va_arg(*ap, int)));
        if ((ch == 'd') || (ch == 'i')) {
            if ((args->flags & FMT_FLAG_CHAR) != 0U) {
                tmp = (char)tmp;
            } else if ((args->flags & FMT_FLAG_SHORT) != 0U) {
                tmp = (short)tmp;
            } else if ((args->flags & (FMT_FLAG_LONG_LONG | FMT_FLAG_LONG)) == 0U) {
                tmp = (int)tmp;
            }
            value = (unsigned long long)((tmp < 0) ? (args->flags |= FMT_FLAG_NEGATIVE, -tmp) : (tmp));
        } else {
            if ((args->flags & FMT_FLAG_CHAR) != 0U) {
                tmp = (unsigned char)tmp;
            } else if ((args->flags & FMT_FLAG_SHORT) != 0U) {
                tmp = (unsigned short)tmp;
            } else if ((args->flags & (FMT_FLAG_LONG_LONG | FMT_FLAG_LONG)) == 0U) {
                tmp = (unsigned int)tmp;
            }
            value = (unsigned long long)(tmp);
            args->flags &= ~(FMT_FLAG_PLUS | FMT_FLAG_SPACE);
        }
        if ((args->base == MDS_NUM_DEC_BASE) || (value == 0)) {
            args->flags &= ~FMT_FLAG_HASH;
        }
    }

    FMT_lltoa(buff, size, pos, value, args);
}

static void FMT_PrintChar(char *buff, size_t size, size_t *pos, va_list *ap, FMT_Args_t *args)
{
    char ch = (char)va_arg(*ap, int);

    *pos = FMT_PrintBuff(buff, size, *pos, &ch, sizeof(ch), args);
}

static void FMT_PrintString(char *buff, size_t size, size_t *pos, va_list *ap, FMT_Args_t *args)
{
    size_t len;
    const char *str = va_arg(*ap, char *);
    if (str == NULL) {
        str = "NULL";
    }

    if (args->precision > 0) {
        len = strnlen(str, args->precision);
    } else {
        len = strlen(str);
    }

    *pos = FMT_PrintBuff(buff, size, *pos, str, len, args);
}

static int FMT_VaParsePrint(char *buff, size_t size, size_t *pos, const char **fmt, va_list *ap)
{
    char ch;
    FMT_Args_t args = {0};

    *fmt = FMT_ParsePrintFormatFlags(*fmt, ap, &args);
    ch = (*fmt)[0];
    args.base = FMT_PrintIntegerBase(ch);
    if (args.base > 0) {
        FMT_PrintInteger(buff, size, pos, ch, ap, &args);
#if (defined(MDS_FORMAT_WITH_FLOAT) && (MDS_FORMAT_WITH_FLOAT > 0))
    } else if ((ch == 'f') || (ch == 'F') || (ch == 'e') || (ch == 'E') || (ch == 'g') || (ch == 'G')) {
        FMT_PrintFloat(buff, size, pos, ch, ap, &args);
#endif
    } else if (ch == 'c') {
        args.flags &= ~FMT_FLAG_ZERO;
        FMT_PrintChar(buff, size, pos, ap, &args);
    } else if (ch == 's') {
        args.flags &= ~FMT_FLAG_ZERO;
        FMT_PrintString(buff, size, pos, ap, &args);
    } else if (ch == '%') {
        buff[(*pos)++] = '%';
    } else {
        return (-1);
    }

    return (0);
}

int MDS_VaStringNPrintf(char *buff, size_t size, const char *fmt, va_list ap)
{
    size_t pos;

    if ((buff == NULL) || (size == 0) || (fmt == NULL)) {
        return (0);
    }

    for (pos = 0; (*fmt != '\0') && (pos < (size - 1)); fmt++) {
        if (*fmt == '%') {
            fmt++;
            int ret = FMT_VaParsePrint(buff, size, &pos, &fmt, (va_list *)(&ap));
            if (ret != 0) {
                return (ret);
            }
        } else {
            buff[pos++] = *fmt;
        }
    }
    buff[pos] = '\0';

    return (pos);
}

int MDS_StringNPrintf(char *buff, size_t size, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = MDS_VaStringNPrintf(buff, size, fmt, ap);
    va_end(ap);

    return (ret);
}

int MDS_VaStringPrintf(char *buff, const char *fmt, va_list ap)
{
    return (MDS_VaStringNPrintf(buff, __SIZE_MAX__, fmt, ap));
}

int MDS_StringPrintf(char *buff, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = MDS_VaStringNPrintf(buff, __SIZE_MAX__, fmt, ap);
    va_end(ap);

    return (ret);
}

static bool FMT_IsSpace(const char ch)
{
    return ((ch == ' ') || (ch == '\t') || (ch == '\v') || (ch == '\f') || (ch == '\r') || (ch == '\n'));
}

static char *FMT_SkipSpace(const char *str)
{
    while (FMT_IsSpace(*str)) {
        str++;
    }

    return ((char *)str);
}

static const char *FMT_ScanInteger(const char *buff, va_list *ap, FMT_Args_t *args)
{
    char *endptr = NULL;
    long long value = MDS_Strtoll(buff, &endptr, args->base);

    if ((args->flags & FMT_FLAG_IGNORE) == 0U) {
        if ((args->flags & FMT_FLAG_CHAR) != 0U) {
            char *pval = va_arg(*ap, char *);
            *pval = (char)value;
        } else if ((args->flags & FMT_FLAG_SHORT) != 0U) {
            short *pval = va_arg(*ap, short *);
            *pval = (short)value;
        } else if ((args->flags & FMT_FLAG_LONG_LONG) != 0U) {
            long long *pval = va_arg(*ap, long long *);
            *pval = (long long)value;
        } else if ((args->flags & FMT_FLAG_LONG) != 0U) {
            long *pval = va_arg(*ap, long *);
            *pval = (long)value;
        } else {
            int *pval = va_arg(*ap, int *);
            *pval = (int)value;
        }
    }

    return ((const char *)endptr);
}

static const char *FMT_ScanChar(const char *buff, const char ch, va_list *ap, FMT_Args_t *args)
{
    char *pch = NULL;
    size_t size = 0;
    size_t len = (args->width <= 0) ? (sizeof(char)) : (args->width);
    size_t idx = 0;

    if ((args->flags & FMT_FLAG_IGNORE) == 0U) {
        pch = va_arg(*ap, char *);
        size = ((args->flags & FMT_FLAG_SECURE) != 0U) ? ((size_t)va_arg(*ap, int)) : (len);
    }

    if (size > 0) {
        while ((*buff != '\0') && (idx < len) && (idx < size)) {
            pch[idx++] = *buff++;
            if (*buff == ch) {
                break;
            }
        }
    } else {
        while ((*buff != '\0') && (idx < len) && (idx < size)) {
            idx++;
            buff++;
            if (*buff == ch) {
                break;
            }
        }
    }

    return (buff);
}

static const char *FMT_ScanString(const char *buff, const char ch, va_list *ap, FMT_Args_t *args)
{
    char *pch = NULL;
    size_t size = 0;
    size_t idx = 0;

    if ((args->flags & FMT_FLAG_IGNORE) == 0U) {
        pch = va_arg(*ap, char *);
        size = ((args->flags & FMT_FLAG_SECURE) != 0U) ? ((size_t)va_arg(*ap, int)) : (__SIZE_MAX__);
    }

    if (size > 0) {
        size = ((0 < args->width) && (args->width < (size - 1))) ? (args->width) : (size);
        while ((*buff != '\0') && (!FMT_IsSpace(*buff)) && ((idx + 1) < size)) {
            pch[idx++] = *buff++;
            if (*buff == ch) {
                break;
            }
        }
        pch[idx] = '\0';
    } else {
        buff++;
    }

    while ((*buff != '\0') && (*buff != ch) && (!FMT_IsSpace(*buff))) {
        buff++;
    }

    return (buff);
}

static const char *FMT_ParseScan(const char *buff, const char **fmt, va_list *ap, bool secure)
{
    char ch;
    FMT_Args_t args = {0};

    *fmt = FMT_ParseScanfFormatFlags(*fmt, &args, secure);
    ch = (*fmt)[0];
    args.base = FMT_PrintIntegerBase(ch);
    if (args.base > 0) {
        buff = FMT_ScanInteger(buff, ap, &args);
    } else if (ch == 'c') {
        buff = FMT_ScanChar(buff, (*fmt)[1], ap, &args);
    } else if (ch == 's') {
        buff = FMT_ScanString(buff, (*fmt)[1], ap, &args);
    } else if ((ch == '%') && (*buff == '%')) {
        buff++;
    }

    return (buff);
}

static int FMT_VaStringScanf(const char *buff, const char *fmt, va_list ap, bool secure)
{
    if ((buff == NULL) || (fmt == NULL)) {
        return (0);
    }

    int ret = 0;
    buff = FMT_SkipSpace(buff);
    while ((*fmt != '\0') && (*buff != '\0')) {
        if (*fmt == '%') {
            fmt++;
            const char *tmp = FMT_ParseScan(buff, (const char **)(&fmt), (va_list *)(&ap), secure);
            if (tmp == buff) {
                break;
            }
            if (*fmt != '%') {
                ret++;
            }
            while (('0' <= *fmt) && (*fmt <= '9')) {
                fmt++;
            }
            fmt++;
            buff = tmp;
        } else if (FMT_IsSpace(*fmt)) {
            fmt++;
            buff = FMT_SkipSpace(buff);
        } else if (*fmt == *buff) {
            fmt++;
            buff++;
        } else {
            break;
        }
    }

    return (ret);
}

int MDS_VaStringScanf(const char *buff, const char *fmt, va_list ap)
{
    return (FMT_VaStringScanf(buff, fmt, ap, false));
}

int MDS_StringScanf(const char *buff, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = FMT_VaStringScanf(buff, fmt, ap, false);
    va_end(ap);

    return (ret);
}

int MDS_VaStringScanfS(const char *buff, const char *fmt, va_list ap)
{
    return (FMT_VaStringScanf(buff, fmt, ap, true));
}

int MDS_StringScanfS(const char *buff, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = FMT_VaStringScanf(buff, fmt, ap, true);
    va_end(ap);

    return (ret);
}
