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
#ifndef __MDS_LOG_H__
#define __MDS_LOG_H__

/* Include ----------------------------------------------------------------- */
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define ------------------------------------------------------------------ */
#ifndef MDS_LOG_TAG
#define MDS_LOG_TAG ""
#endif

#define MDS_LOG_LEVEL_OFF     0x00U
#define MDS_LOG_LEVEL_THROUGH 0x01U
#define MDS_LOG_LEVEL_FATAL   0x02U
#define MDS_LOG_LEVEL_ERROR   0x03U
#define MDS_LOG_LEVEL_WARN    0x04U
#define MDS_LOG_LEVEL_INFO    0x05U
#define MDS_LOG_LEVEL_DEBUG   0x06U
#define MDS_LOG_LEVEL_ALL     0x07U

#ifndef MDS_LOG_BUILD_LEVEL
#define MDS_LOG_BUILD_LEVEL MDS_LOG_LEVEL_ALL
#endif

#define MDS_LOG_ARG_SEQS(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N
#define MDS_LOG_ARG_NUMS(...)                                                                                          \
    MDS_LOG_ARG_SEQS(0, ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#ifndef MDS_LOG_COMPRESS_ARGS_MAX
#define MDS_LOG_COMPRESS_ARGS_MAX 7
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct MDS_LOG_Compress {
    uint8_t magic;
    uint32_t address   : 24;  // 0xFFxxxxxx
    uint32_t level     : 4;
    uint32_t count     : 4;
    uint32_t psn       : 12;
    uint64_t timestamp : 44;  // ms
    uint32_t args[MDS_LOG_COMPRESS_ARGS_MAX];
} MDS_LOG_Compress_t;

/* Function ---------------------------------------------------------------- */
extern size_t MDS_LOG_CompressStruct(MDS_LOG_Compress_t *log, size_t level, const char *fmt, size_t cnt, va_list ap);
extern void MDS_LOG_VaPrintf(size_t level, const char *fmt, size_t cnt, va_list ap);
extern void MDS_LOG_Printf(size_t level, const char *fmt, size_t cnt, ...);

#if (MDS_LOG_BUILD_LEVEL >= MDS_LOG_LEVEL_THROUGH)
#define MDS_LOG_T(fmt, ...)                                                                                            \
    do {                                                                                                               \
        static const char __attribute__((section(".logstr.through"))) fmtstr[] = MDS_LOG_TAG fmt "\n";                 \
        MDS_LOG_Printf(MDS_LOG_LEVEL_THROUGH, fmtstr, MDS_LOG_ARG_NUMS(__VA_ARGS__), ##__VA_ARGS__);                   \
    } while (0)
#else
#define MDS_LOG_T(fmt, ...)
#endif

#if (MDS_LOG_BUILD_LEVEL >= MDS_LOG_LEVEL_FATAL)
#define MDS_LOG_F(fmt, ...)                                                                                            \
    do {                                                                                                               \
        static const char __attribute__((section(".logstr.fatal"))) fmtstr[] = MDS_LOG_TAG fmt "\n";                   \
        MDS_LOG_Printf(MDS_LOG_LEVEL_FATAL, fmtstr, MDS_LOG_ARG_NUMS(__VA_ARGS__), ##__VA_ARGS__);                     \
    } while (0)
#else
#define MDS_LOG_F(fmt, ...)
#endif

#if (MDS_LOG_BUILD_LEVEL >= MDS_LOG_LEVEL_ERROR)
#define MDS_LOG_E(fmt, ...)                                                                                            \
    do {                                                                                                               \
        static const char __attribute__((section(".logstr.error"))) fmtstr[] = MDS_LOG_TAG fmt "\n";                   \
        MDS_LOG_Printf(MDS_LOG_LEVEL_ERROR, fmtstr, MDS_LOG_ARG_NUMS(__VA_ARGS__), ##__VA_ARGS__);                     \
    } while (0)
#else
#define MDS_LOG_E(fmt, ...)
#endif

#if (MDS_LOG_BUILD_LEVEL >= MDS_LOG_LEVEL_WARN)
#define MDS_LOG_W(fmt, ...)                                                                                            \
    do {                                                                                                               \
        static const char __attribute__((section(".logstr.warn"))) fmtstr[] = MDS_LOG_TAG fmt "\n";                    \
        MDS_LOG_Printf(MDS_LOG_LEVEL_WARN, fmtstr, MDS_LOG_ARG_NUMS(__VA_ARGS__), ##__VA_ARGS__);                      \
    } while (0)
#else
#define MDS_LOG_W(fmt, ...)
#endif

#if (MDS_LOG_BUILD_LEVEL >= MDS_LOG_LEVEL_INFO)
#define MDS_LOG_I(fmt, ...)                                                                                            \
    do {                                                                                                               \
        static const char __attribute__((section(".logstr.info"))) fmtstr[] = MDS_LOG_TAG fmt "\n";                    \
        MDS_LOG_Printf(MDS_LOG_LEVEL_INFO, fmtstr, MDS_LOG_ARG_NUMS(__VA_ARGS__), ##__VA_ARGS__);                      \
    } while (0)
#else
#define MDS_LOG_I(fmt, ...)
#endif

#if (MDS_LOG_BUILD_LEVEL >= MDS_LOG_LEVEL_DEBUG)
#define MDS_LOG_D(fmt, ...)                                                                                            \
    do {                                                                                                               \
        static const char __attribute__((section(".logstr.debug"))) fmtstr[] = MDS_LOG_TAG fmt "\n";                   \
        MDS_LOG_Printf(MDS_LOG_LEVEL_DEBUG, fmtstr, MDS_LOG_ARG_NUMS(__VA_ARGS__), ##__VA_ARGS__);                     \
    } while (0)
#else
#define MDS_LOG_D(fmt, ...)
#endif

/* Panic ------------------------------------------------------------------- */
extern __attribute__((noreturn)) void MDS_PanicPrintf(const char *fmt, ...);

#define MDS_PANIC(fmt, ...)                                                                                            \
    do {                                                                                                               \
        static const char __attribute__((section(".logstr.panic"))) fmtstr[] = "[PANIC]" fmt "\n";                     \
        MDS_PanicPrintf(fmtstr, ##__VA_ARGS__);                                                                        \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* __MDS_LOG_H__ */
