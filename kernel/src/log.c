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
#include "mds_log.h"
#include "mds_sys.h"

/* Define ------------------------------------------------------------------ */
#ifndef MDS_LOG_COMPRESS_MAGIC
#define MDS_LOG_COMPRESS_MAGIC 0xD6 /* log 109 => 0x6D */
#endif

#ifndef MDS_LOG_COMPRESS_ARG_FIX
/* 0xFFFFFFFF => -1111111111 (2's complement: 0xBDC5CA39) */
#define MDS_LOG_COMPRESS_ARG_FIX(x) ((x == 0xFFFFFFFF) ? (0xBDC5CA39) : (x))
#endif

/* Function ---------------------------------------------------------------- */
size_t MDS_LOG_CompressStruct(MDS_LOG_Compress_t *log, size_t level, const char *fmt, size_t cnt, va_list ap)
{
    MDS_ASSERT(log != NULL);

    static size_t psn = 0;

    register MDS_Item_t lock = MDS_CoreInterruptLock();
    psn++;
    MDS_CoreInterruptRestore(lock);

    if (cnt > MDS_LOG_COMPRESS_ARGS_MAX) {
        cnt = MDS_LOG_COMPRESS_ARGS_MAX;
    }

    log->magic = MDS_LOG_COMPRESS_MAGIC;
    log->address = (uintptr_t)fmt & 0x00FFFFFF;
    log->level = level;
    log->count = cnt;
    log->psn = psn;
    log->timestamp = MDS_TIME_GetTimeStamp(NULL);

    for (size_t idx = 0; idx < cnt; idx++) {
        log->args[idx] = MDS_LOG_COMPRESS_ARG_FIX(va_arg(ap, uint32_t));
    }

    return (sizeof(MDS_LOG_Compress_t) - (sizeof(uint32_t) * (MDS_LOG_COMPRESS_ARGS_MAX + cnt)));
}

__attribute__((weak)) void MDS_LOG_VaPrintf(size_t level, const char *fmt, size_t cnt, va_list ap)
{
    UNUSED(level);
    UNUSED(fmt);
    UNUSED(cnt);
    UNUSED(ap);
}

void MDS_LOG_Printf(size_t level, const char *fmt, size_t cnt, ...)
{
    va_list ap;

    va_start(ap, cnt);
    MDS_LOG_VaPrintf(level, fmt, cnt, ap);
    va_end(ap);
}

__attribute__((noreturn)) void MDS_PanicPrintf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    MDS_LOG_VaPrintf(MDS_LOG_LEVEL_FATAL, fmt, 0, ap);
    va_end(ap);

    MDS_CoreInterruptLock();

    // TODO: add core backtrace

    MDS_LOOP {
    }
}

__attribute__((noreturn)) void MDS_AssertPrintf(const char *assertion, const char *function, void *caller)
{
    MDS_PanicPrintf("[ASSERT] '%s' in function:'%s' caller:%p,%p", assertion, function, caller,
                    __builtin_extract_return_addr(caller));
}
