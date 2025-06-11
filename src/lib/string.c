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

/* String ------------------------------------------------------------------ */
size_t MDS_StrAscLetterLength(const char *str)
{
    size_t cnt = 0;

    while (*str != '\0') {
        if ((('0' <= *str) && (*str <= '9')) || (('A' <= *str) && (*str <= 'Z')) ||
            (('a' <= *str) && (*str <= 'z'))) {
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
        int val = (asc[ofs] >= 'a')
                      ? (asc[ofs] - 'a' + 0x0A)
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
            uint8_t val = ((cnt & 0x01) == 0U) ? (hex[idx] >> (MDS_BITS_OF_BYTE >> 1))
                                               : (hex[idx++] & 0x0FU);
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

size_t MDS_Strnlen(const char *str, size_t maxlen)
{
    const char *ch = str;

    while ((maxlen != 0) && (*ch != '\0')) {
        ch++;
        maxlen--;
    }

    return ((size_t)(ch - str));
}

size_t MDS_Strlen(const char *str)
{
    const char *ch = str;

    while (*ch != '\0') {
        ch++;
    }

    return (ch - str);
}

size_t MDS_Strlcpy(char *dst, const char *src, size_t dsize)
{
    const char *osrc = src;

    while ((dsize > 1) && (*src != '\0')) {
        *dst++ = *src++;
        dsize--;
    }

    while (*src != '\0') {
        src++;
    }

    if (dsize > 0) {
        *dst = '\0';
    }

    return (src - osrc);
}

size_t MDS_Strlcat(char *dst, const char *src, size_t dsize)
{
    size_t dlen;
    const char *odst = dst;
    const char *osrc = src;

    while ((dsize > 0) && (*dst != '\0')) {
        dst++;
        dsize--;
    }
    dlen = dst - odst;

    for (odst = dst; *src != '\0'; src++) {
        if (dsize > 1) {
            *dst++ = *src;
            dsize--;
        }
    }

    if (dst != odst) {
        *dst = '\0';
    }

    return (dlen + (src - osrc));
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
