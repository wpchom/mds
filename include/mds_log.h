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
#include "mds_def.h"
#include "mds_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Config ------------------------------------------------------------------ */
#ifndef CONFIG_MDS_LOG_ENABLE
#define CONFIG_MDS_LOG_ENABLE 1
#endif

#ifndef CONFIG_MDS_LOG_BUILD_LEVEL
#define CONFIG_MDS_LOG_BUILD_LEVEL MDS_LOG_LEVEL_INF
#endif

#ifndef CONFIG_MDS_LOG_FILTER_ENABLE
#define CONFIG_MDS_LOG_FILTER_ENABLE 0
#endif

#ifndef CONFIG_MDS_LOG_ASSERT_ENABLE
#define CONFIG_MDS_LOG_ASSERT_ENABLE 1
#endif

#ifndef CONFIG_MDS_LOG_FORMAT_SECTION
#define CONFIG_MDS_LOG_FORMAT_SECTION ".logfmt."
#endif

/* Log --------------------------------------------------------------------- */
#define MDS_LOG_LEVEL_OFF 0
#define MDS_LOG_LEVEL_FAT 1
#define MDS_LOG_LEVEL_ERR 2
#define MDS_LOG_LEVEL_WRN 3
#define MDS_LOG_LEVEL_INF 4
#define MDS_LOG_LEVEL_DBG 5

typedef struct MDS_LOG_Module MDS_LOG_Module_t;
typedef void (*MDS_LOG_VaPrint_t)(const MDS_LOG_Module_t *module, uint8_t level, size_t va_size,
                                  const char *fmt, va_list va_args);

typedef struct MDS_LOG_Filter {
    MDS_LOG_VaPrint_t backend;
    uint8_t level;  // align ?
} MDS_LOG_Filter_t;

struct MDS_LOG_Module {
    const char *name;
#if (defined(CONFIG_MDS_LOG_FILTER_ENABLE) && (CONFIG_MDS_LOG_FILTER_ENABLE != 0))
    MDS_LOG_Filter_t *filter;
#endif
};

#define __LOG_FORMAT_SECTION_STR(_lvl)                                                            \
    CONFIG_MDS_LOG_FORMAT_SECTION MDS_ARGUMENT_CAT(__LOG_FORMAT_SECTION_, _lvl) "."
#define __LOG_FORMAT_SECTION_0 "off"
#define __LOG_FORMAT_SECTION_1 "fatal"
#define __LOG_FORMAT_SECTION_2 "error"
#define __LOG_FORMAT_SECTION_3 "warn"
#define __LOG_FORMAT_SECTION_4 "info"
#define __LOG_FORMAT_SECTION_5 "debug"
#define __LOG_FORMAT_SECTION__ "panic"

#define __LOG_MODULE_LEVEL(...) MDS_ARGUMENT_GET_N(1, ##__VA_ARGS__, CONFIG_MDS_LOG_BUILD_LEVEL)

#define MDS_LOG_MODULE_DECLARE(_name, ...)                                                        \
    static __attribute__((used))                                                                  \
    const uint8_t __THIS_LOG_MODULE_LEVEL = __LOG_MODULE_LEVEL(__VA_ARGS__);                      \
    extern const MDS_LOG_Module_t G_MDS_LOG_MODULE_##_name;                                       \
    static __attribute__((used))                                                                  \
    const MDS_LOG_Module_t *const __THIS_LOG_MODULE_HANDLE = &(G_MDS_LOG_MODULE_##_name)

#if (defined(CONFIG_MDS_LOG_FILTER_ENABLE) && (CONFIG_MDS_LOG_FILTER_ENABLE != 0))
#define MDS_LOG_MODULE_DEFINE(_name, ...)                                                         \
    static __attribute__((used))                                                                  \
    const uint8_t __THIS_LOG_MODULE_LEVEL = __LOG_MODULE_LEVEL(__VA_ARGS__);                      \
    static MDS_LOG_Filter_t g_mds_log_filter_##_name = {                                          \
        .level = __LOG_MODULE_LEVEL(__VA_ARGS__),                                                 \
        .backend = NULL,                                                                          \
    };                                                                                            \
    const MDS_LOG_Module_t G_MDS_LOG_MODULE_##_name = {                                           \
        .name = #_name, .filter = &(g_mds_log_filter_##_name)};                                   \
    static __attribute__((used))                                                                  \
    const MDS_LOG_Module_t *const __THIS_LOG_MODULE_HANDLE = &(G_MDS_LOG_MODULE_##_name)

#define MDS_LOG_MODULE_LEVEL(_lvl)       __THIS_LOG_MODULE_HANDLE->filter->level = _lvl
#define MDS_LOG_MODULE_BACKEND(_backend) __THIS_LOG_MODULE_HANDLE->filter->backend = _backend
#else
#define MDS_LOG_MODULE_DEFINE(_name, ...)                                                         \
    static __attribute__((used))                                                                  \
    const uint8_t __THIS_LOG_MODULE_LEVEL = __LOG_MODULE_LEVEL(__VA_ARGS__);                      \
    const MDS_LOG_Module_t G_MDS_LOG_MODULE_##_name = {.name = #_name};                           \
    static __attribute__((used))                                                                  \
    const MDS_LOG_Module_t *const __THIS_LOG_MODULE_HANDLE = &(G_MDS_LOG_MODULE_##_name)

#define MDS_LOG_MODULE_LEVEL(_lvl)       (void)(__THIS_LOG_MODULE_LEVEL)
#define MDS_LOG_MODULE_BACKEND(_backend) (void)(__THIS_LOG_MODULE_LEVEL)
#endif

#define __LOG_ARGSIZE_INDEX(idx, x, ...)                                                          \
    _Generic((x),                                                                                 \
        bool: sizeof(int),                                                                        \
        char: sizeof(int),                                                                        \
        signed char: sizeof(int),                                                                 \
        unsigned char: sizeof(int),                                                               \
        short: sizeof(int),                                                                       \
        unsigned short: sizeof(int),                                                              \
        int: sizeof(int),                                                                         \
        unsigned int: sizeof(unsigned int),                                                       \
        long: sizeof(long),                                                                       \
        unsigned long: sizeof(unsigned long),                                                     \
        long long: sizeof(long long),                                                             \
        unsigned long long: sizeof(unsigned long long),                                           \
        float: sizeof(float),                                                                     \
        double: sizeof(double),                                                                   \
        long double: sizeof(long double),                                                         \
        default: sizeof(void *))

#define __LOG_ARGUMENT_SIZE(...)                                                                  \
    (0 + MDS_ARGUMENT_FOREACH_N(__LOG_ARGSIZE_INDEX, (+), 0, ##__VA_ARGS__))

#if (defined(CONFIG_MDS_LOG_ENABLE) && (CONFIG_MDS_LOG_ENABLE != 0))
#define MDS_LOG_PRINT(_lvl, _fmt, ...)                                                            \
    do {                                                                                          \
        if (_lvl <= __THIS_LOG_MODULE_LEVEL) {                                                    \
            static __attribute__((section(__LOG_FORMAT_SECTION_STR(_lvl))))                       \
            const char __logfmt[] = _fmt "\n";                                                    \
            MDS_LOG_ModulePrintf(__THIS_LOG_MODULE_HANDLE, _lvl,                                  \
                                 __LOG_ARGUMENT_SIZE(__VA_ARGS__), __logfmt, ##__VA_ARGS__);      \
        }                                                                                         \
    } while (0)
#else
#define MDS_LOG_PRINT(_lvl, _fmt, ...) (void)(__THIS_LOG_MODULE_LEVEL)
#endif

#define MDS_LOG_F(_fmt, ...) MDS_LOG_PRINT(MDS_LOG_LEVEL_FAT, _fmt, ##__VA_ARGS__)

#define MDS_LOG_E(_fmt, ...) MDS_LOG_PRINT(MDS_LOG_LEVEL_ERR, _fmt, ##__VA_ARGS__)

#define MDS_LOG_W(_fmt, ...) MDS_LOG_PRINT(MDS_LOG_LEVEL_WRN, _fmt, ##__VA_ARGS__)

#define MDS_LOG_I(_fmt, ...) MDS_LOG_PRINT(MDS_LOG_LEVEL_INF, _fmt, ##__VA_ARGS__)

#define MDS_LOG_D(_fmt, ...) MDS_LOG_PRINT(MDS_LOG_LEVEL_DBG, _fmt, ##__VA_ARGS__)

/* Function ---------------------------------------------------------------- */
void MDS_LOG_RegisterVaPrint(MDS_LOG_VaPrint_t logVaPrint);

__attribute__((format(printf, 4, 5))) void MDS_LOG_ModulePrintf(const MDS_LOG_Module_t *module,
                                                                uint8_t level, size_t va_size,
                                                                const char *fmt, ...);

/* Panic ------------------------------------------------------------------- */
__attribute__((noreturn, format(printf, 2, 3))) void MDS_PanicPrintf(size_t va_size,
                                                                     const char *fmt, ...);

#if (defined(CONFIG_MDS_LOG_ENABLE) && (CONFIG_MDS_LOG_ENABLE != 0))
#define MDS_PANIC(_fmt, ...)                                                                      \
    do {                                                                                          \
        void *caller = __builtin_return_address(0);                                               \
        static __attribute__((section(__LOG_FORMAT_SECTION_STR(_))))                              \
        const char __logfmt[] = "[PANIC] caller:%p " _fmt "\n";                                   \
        MDS_PanicPrintf(sizeof(void *) + __LOG_ARGUMENT_SIZE(__VA_ARGS__), __logfmt, caller,      \
                        ##__VA_ARGS__);                                                           \
    } while (0)
#else
#define MDS_PANIC(_fmt, ...)                                                                      \
    do {                                                                                          \
        MDS_PanicPrintf(0, NULL);                                                                 \
    } while (0)
#endif

/* Assert ------------------------------------------------------------------ */
#if (defined(CONFIG_MDS_LOG_ASSERT_ENABLE) && (CONFIG_MDS_LOG_ASSERT_ENABLE != 0))
#define MDS_ASSERT(condition)                                                                     \
    do {                                                                                          \
        if (!(condition)) {                                                                       \
            MDS_PANIC("[ASSERT] caller:%p", __builtin_return_address(0));                         \
        }                                                                                         \
    } while (0)
#else
#define MDS_ASSERT(condition) (void)(condition)
#endif

/* Hook -------------------------------------------------------------------- */
#define MDS_HOOK_DECLARE(_MODULE, _TYPE)                                                          \
    MDS_COND_CODE_1(CONFIG_MDS_HOOK_ENABLE_##_MODULE, (extern const _TYPE G_MDS_HOOK_##_MODULE),  \
                    ())

#define MDS_HOOK_DEFINE(_MODULE, _TYPE, ...)                                                      \
    MDS_COND_CODE_1(CONFIG_MDS_HOOK_ENABLE_##_MODULE,                                             \
                    (const _TYPE G_MDS_HOOK_##_MODULE = {__DEBRACKET __VA_ARGS__}), ())

#define MDS_HOOK_CALL(_MODULE, _func, ...)                                                        \
    MDS_COND_CODE_1(CONFIG_MDS_HOOK_ENABLE_##_MODULE, (do {                                       \
                        if ((G_MDS_HOOK_##_MODULE._func) != NULL) {                               \
                            G_MDS_HOOK_##_MODULE._func(__DEBRACKET __VA_ARGS__);                  \
                        }                                                                         \
                    } while (0)),                                                                 \
                    ())

/* Compress ---------------------------------------------------------------- */
typedef struct MDS_LOG_Compress {
    uint32_t magic     : 8;
    uint32_t address   : 24;  // 0xFFxxxxxx
    uint32_t level     : 4;
    uint32_t count     : 4;
    uint32_t psn       : 12;
    uint64_t timestamp : 44;  // ms
    uint32_t args[0];
} MDS_LOG_Compress_t;

size_t MDS_LOG_CompressStructVa(MDS_LOG_Compress_t *log, size_t level, size_t cnt, const char *fmt,
                                va_list ap);
size_t MDS_LOG_CompressSturctPrint(MDS_LOG_Compress_t *log, size_t level, size_t cnt,
                                   const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* __MDS_LOG_H__ */
