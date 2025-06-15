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

    int width = (*fmt == '*') ? (fmt++, va_arg(*ap, int))
                              : (MDS_Strtol(fmt, (char **)(&fmt), MDS_NUM_DEC_BASE));
    args->width = (width < 0) ? (args->flags |= FMT_FLAG_LEFT, -width) : (width);

    int prec = 0;
    if (*fmt == '.') {
        prec = ((*++fmt == '*') ? (fmt++, va_arg(*ap, int))
                                : (MDS_Strtol(fmt, (char **)(&fmt), MDS_NUM_DEC_BASE)));
    }
    args->precision = (prec > 0) ? (args->flags |= FMT_FLAG_PRECISION, (unsigned int)(prec)) : (0);

    if (*fmt == 'l') {
        args->flags |= (*++fmt == 'l') ? (fmt++, FMT_FLAG_LONG_LONG | FMT_FLAG_LONG)
                                       : (FMT_FLAG_LONG);
    } else if (*fmt == 'h') {
        args->flags |= (*++fmt == 'h') ? (fmt++, FMT_FLAG_SHORT | FMT_FLAG_CHAR)
                                       : (FMT_FLAG_SHORT);
    } else if (*fmt == 'X') {
        args->flags |= FMT_FLAG_UPCASE;
    }

    return (fmt);
}

static size_t FMT_PrintBuff(char *buff, size_t size, size_t pos, const char *str, size_t len,
                            FMT_Args_t *args)
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
        while (((len < args->precision) ||
                (((args->flags & FMT_FLAG_ZERO) != 0U) && (len < args->width))) &&
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

static void FMT_PrintBuffInterger(char *buff, size_t size, size_t *pos, char *valbuf, size_t len,
                                  FMT_Args_t *args)
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

static void FMT_lltoa(char *buff, size_t size, size_t *pos, unsigned long long value,
                      FMT_Args_t *args)
{
    char valbuf[FMT_NTOA_BUFFER_SIZE];
    size_t len = 0;

    do {
        unsigned char digit = (char)(value % args->base);
        if (digit < MDS_NUM_DEC_BASE) {
            valbuf[len++] = (char)(digit + '0');
        } else {
            valbuf[len++] = (char)(digit - MDS_NUM_DEC_BASE +
                                   ((args->flags & FMT_FLAG_UPCASE) ? ('A') : ('a')));
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

static void FMT_PrintInteger(char *buff, size_t size, size_t *pos, const char ch, va_list *ap,
                             FMT_Args_t *args)
{
    unsigned long long value;

    if (ch == 'p') {
        value = (uintptr_t)va_arg(*ap, void *);
        args->flags |= FMT_FLAG_ZERO | FMT_FLAG_HASH;
    } else {
        long long tmp = (args->flags & FMT_FLAG_LONG_LONG)
                            ? (va_arg(*ap, long long))
                            : ((args->flags & FMT_FLAG_LONG) ? (va_arg(*ap, long))
                                                             : (va_arg(*ap, int)));
        if ((ch == 'd') || (ch == 'i')) {
            if ((args->flags & FMT_FLAG_CHAR) != 0U) {
                tmp = (char)tmp;
            } else if ((args->flags & FMT_FLAG_SHORT) != 0U) {
                tmp = (short)tmp;
            } else if ((args->flags & (FMT_FLAG_LONG_LONG | FMT_FLAG_LONG)) == 0U) {
                tmp = (int)tmp;
            }
            value = (unsigned long long)((tmp < 0) ? (args->flags |= FMT_FLAG_NEGATIVE, -tmp)
                                                   : (tmp));
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
        len = MDS_Strnlen(str, args->precision);
    } else {
        len = MDS_Strlen(str);
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
#if (defined(CONFIG_MDS_FORMAT_WITH_FLOAT) && (CONFIG_MDS_FORMAT_WITH_FLOAT != 0))
    } else if ((ch == 'f') || (ch == 'F') || (ch == 'e') || (ch == 'E') || (ch == 'g') ||
               (ch == 'G')) {
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

int MDS_Vsnprintf(char *buff, size_t size, const char *fmt, va_list ap)
{
    size_t pos;
    va_list acpy;

    if ((buff == NULL) || (size == 0) || (fmt == NULL)) {
        return (0);
    }

    for (pos = 0; (*fmt != '\0') && (pos < (size - 1)); fmt++) {
        if (*fmt == '%') {
            fmt++;
            va_copy(acpy, ap);
            int ret = FMT_VaParsePrint(buff, size, &pos, &fmt, &acpy);
            va_end(ap);
            va_copy(ap, acpy);
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

int MDS_Snprintf(char *buff, size_t size, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = MDS_Vsnprintf(buff, size, fmt, ap);
    va_end(ap);

    return (ret);
}

int MDS_Vsprintf(char *buff, const char *fmt, va_list ap)
{
    return (MDS_Vsnprintf(buff, __SIZE_MAX__, fmt, ap));
}

int MDS_Sprintf(char *buff, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = MDS_Vsnprintf(buff, __SIZE_MAX__, fmt, ap);
    va_end(ap);

    return (ret);
}

static const char *FMT_ParseScanfFormatFlags(const char *fmt, FMT_Args_t *args)
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
        args->flags |= (*++fmt == 'l') ? (fmt++, FMT_FLAG_LONG_LONG | FMT_FLAG_LONG)
                                       : (FMT_FLAG_LONG);
    } else if (*fmt == 'h') {
        args->flags |= (*++fmt == 'h') ? (fmt++, FMT_FLAG_SHORT | FMT_FLAG_CHAR)
                                       : (FMT_FLAG_SHORT);
    }

    return (fmt);
}

static bool FMT_IsSpace(const char ch)
{
    return ((ch == ' ') || (ch == '\t') || (ch == '\v') || (ch == '\f') || (ch == '\r') ||
            (ch == '\n'));
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
    size_t idx = 0;
    size_t size = (args->width <= 0) ? (sizeof(char)) : (args->width);

    if ((args->flags & FMT_FLAG_IGNORE) == 0U) {
        pch = va_arg(*ap, char *);
    }

    if (pch != NULL) {
        while ((*buff != '\0') && (idx < size)) {
            pch[idx++] = *buff++;
            if (*buff == ch) {
                break;
            }
        }
    } else {
        while ((*buff != '\0') && (idx < size)) {
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
    size_t idx = 0;
    size_t size = (args->width > 0) ? (args->width) : (__SIZE_MAX__);

    if ((args->flags & FMT_FLAG_IGNORE) == 0U) {
        pch = va_arg(*ap, char *);
    }

    if (pch != NULL) {
        while ((*buff != '\0') && (!FMT_IsSpace(*buff)) && ((idx + 1) < size)) {
            pch[idx++] = *buff++;
            if (*buff == ch) {
                break;
            }
        }
        pch[idx] = '\0';
    } else {
        while ((*buff != '\0') && (!FMT_IsSpace(*buff)) && (idx < size)) {
            idx++;
            buff++;
        }
    }

    return (buff);
}

static const char *FMT_ParseScan(const char *buff, const char **fmt, va_list *ap)
{
    char ch;
    FMT_Args_t args = {0};

    *fmt = FMT_ParseScanfFormatFlags(*fmt, &args);
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

static int FMT_VaStringScanf(const char *buff, const char *fmt, va_list ap)
{
    if ((buff == NULL) || (fmt == NULL)) {
        return (0);
    }

    int ret = 0;
    va_list acpy;

    buff = FMT_SkipSpace(buff);
    while ((*fmt != '\0') && (*buff != '\0')) {
        if (*fmt == '%') {
            fmt++;
            va_copy(acpy, ap);
            const char *tmp = FMT_ParseScan(buff, &fmt, &acpy);
            va_end(ap);
            va_copy(ap, acpy);
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

int MDS_Vsscanf(const char *buff, const char *fmt, va_list ap)
{
    return (FMT_VaStringScanf(buff, fmt, ap));
}

int MDS_Sscanf(const char *buff, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = FMT_VaStringScanf(buff, fmt, ap);
    va_end(ap);

    return (ret);
}
