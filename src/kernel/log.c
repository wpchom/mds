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

/* Define ------------------------------------------------------------------ */
MDS_LOG_MODULE_DEFINE(kernel, CONFIG_MDS_KERNEL_LOG_LEVEL);

/* Variable ---------------------------------------------------------------- */
static MDS_LOG_VaPrint_t g_logVaPrintFunc = NULL;

/* Function ---------------------------------------------------------------- */
void MDS_LOG_RegisterVaPrint(MDS_LOG_VaPrint_t logVaPrint)
{
    g_logVaPrintFunc = logVaPrint;
}

static void MDS_LOG_ModuleVaPrintf(const MDS_LOG_Module_t *module, uint8_t level, size_t va_size,
                                   const char *fmt, va_list va_args)
{
#if (defined(CONFIG_MDS_LOG_FILTER_ENABLE) && (CONFIG_MDS_LOG_FILTER_ENABLE != 0))
    if ((module != NULL) && (module->filter != NULL)) {
        if (level > module->filter->level) {
            return;
        }
        if (module->filter->backend != NULL) {
            module->filter->backend(module, level, va_size, fmt, va_args);
            return;
        }
    }
#endif
    if (g_logVaPrintFunc != NULL) {
        g_logVaPrintFunc(module, level, va_size, fmt, va_args);
    }
}

void MDS_LOG_ModulePrintf(const MDS_LOG_Module_t *module, uint8_t level, size_t va_size,
                          const char *fmt, ...)
{
    va_list va_args;

    va_start(va_args, fmt);
    MDS_LOG_ModuleVaPrintf(module, level, va_size, fmt, va_args);
    va_end(va_args);
}

__attribute__((weak, noreturn)) void MDS_CorePanicTrace(void)
{
    for (;;) {
    }
}

__attribute__((noreturn)) void MDS_PanicPrintf(size_t va_size, const char *fmt, ...)
{
#if (defined(CONFIG_MDS_LOG_ENABLE) && (CONFIG_MDS_LOG_ENABLE != 0))
    va_list va_args;

    va_start(va_args, fmt);
    MDS_LOG_ModuleVaPrintf(__THIS_LOG_MODULE_HANDLE, MDS_LOG_LEVEL_FAT, va_size, fmt, va_args);
    va_end(va_args);
#else
    UNUSED(va_size);
    UNUSED(fmt);
#endif

    MDS_CorePanicTrace();
}

#if 0
/* Compress ---------------------------------------------------------------- */
#ifndef MDS_LOG_COMPRESS_ARGS_NUMS
#define MDS_LOG_COMPRESS_ARGS_NUMS 7
#endif

#ifndef MDS_LOG_COMPRESS_MAGIC
#define MDS_LOG_COMPRESS_MAGIC 0xD6 /* log 109 => 0x6D */
#endif

#ifndef MDS_LOG_COMPRESS_ARG_FIX
/* 0xFFFFFFFF => -1111111111 (2's complement: 0xBDC5CA39) */
#define MDS_LOG_COMPRESS_ARG_FIX(x) ((x == 0xFFFFFFFF) ? (0xBDC5CA39) : (x))
#endif

size_t MDS_LOG_CompressStructVa(MDS_LOG_Compress_t *log, size_t level, size_t cnt,
                                const char *fmt, va_list ap)
{
    static size_t logCompressPsn = 0;

    if (log == NULL) {
        return (0);
    }

    // atomic
    MDS_Lock_t lock = MDS_KernelEnterCritical();
    logCompressPsn++;
    MDS_KernelExitCritical(lock);

    if (cnt > MDS_LOG_COMPRESS_ARGS_NUMS) {
        cnt = MDS_LOG_COMPRESS_ARGS_NUMS;
    }

    log->magic = MDS_LOG_COMPRESS_MAGIC;
    log->address = (uintptr_t)fmt & 0x00FFFFFF;
    log->level = level;
    log->count = cnt;
    log->psn = logCompressPsn;
    log->timestamp = MDS_ClockGetTimestamp(NULL);

    for (size_t idx = 0; idx < cnt; idx++) {
        uint32_t val = va_arg(ap, uint32_t);
        log->args[idx] = MDS_LOG_COMPRESS_ARG_FIX(val);
    }

    return (sizeof(MDS_LOG_Compress_t) -
            (sizeof(uint32_t) * (MDS_LOG_COMPRESS_ARGS_NUMS + cnt)));
}

size_t MDS_LOG_CompressSturctPrint(MDS_LOG_Compress_t *log, size_t level, size_t cnt,
                                   const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    size_t len = MDS_LOG_CompressStructVa(log, level, cnt, fmt, ap);
    va_end(ap);

    return (len);
}
#endif
