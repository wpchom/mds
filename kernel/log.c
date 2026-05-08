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
MDS_LOG_MODULE_DEFINE(default, CONFIG_MDS_LOG_BUILD_LEVEL);

#ifndef CONFIG_MDS_LOG_MSGQUEUE_NUMS
#define CONFIG_MDS_LOG_MSGQUEUE_NUMS 0
#endif

#ifndef CONFIG_MDS_LOG_THREAD_STACKSIZE
#define CONFIG_MDS_LOG_THREAD_STACKSIZE 512
#endif

#ifndef CONFIG_MDS_LOG_THREAD_PRIORITY
#define CONFIG_MDS_LOG_THREAD_PRIORITY (CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX - 1)
#endif

#ifndef CONFIG_MDS_LOG_THREAD_TICKS
#define CONFIG_MDS_LOG_THREAD_TICKS 32
#endif

/* Variable ---------------------------------------------------------------- */
#if (defined(CONFIG_MDS_LOG_FILTER_ENABLE) && (CONFIG_MDS_LOG_FILTER_ENABLE != 0))
#define MDS_LOG_DEFAULT_PRINT(module, ...)                                                         \
    do {                                                                                           \
        if ((G_MDS_LOG_MODULE_default.filter != NULL) &&                                           \
            (G_MDS_LOG_MODULE_default.filter->print != NULL)) {                                    \
            G_MDS_LOG_MODULE_default.filter->print(module, ##__VA_ARGS__);                         \
        }                                                                                          \
    } while (0)
#else
static MDS_LOG_VaPrint_t g_logVaPrintFunc = NULL;
#endif

#if ((CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX > 0) &&                                                \
     ((defined(CONFIG_MDS_LOG_MSGQUEUE_NUMS) && (CONFIG_MDS_LOG_MSGQUEUE_NUMS > 0))))
enum MDS_LOG_Status {
    MDS_LOG_STATUS_NOINIT = 0,
    MDS_LOG_STATUS_INITING,
    MDS_LOG_STATUS_IDLE,
    MDS_LOG_STATUS_BUSY,
};

static struct MDS_LOG_Handle {
    MDS_SpinLock_t spinlock;
    MDS_Thread_t *thread;
    MDS_MsgQueue_t *mq;
    uint16_t loss; // atomic
    uint16_t psn;  // atomic
} g_logHandle;

typedef struct MDS_LOG_Message {
    const MDS_LOG_Module_t *module;
    uint8_t level : 4;
    uint8_t count : 4;
    uint32_t psn : 12;
    uint64_t timestamp : 44; // ms
    const char *fmt;
    int32_t args[CONFIG_MDS_LOG_MSGARGS_NUMS];
} MDS_LOG_Message_t;
#define __LOG_MESSAGE_ARG(idx, x, ...) ((x)->args[idx])
#endif

/* Function ---------------------------------------------------------------- */
void MDS_LOG_RegisterVaPrint(MDS_LOG_VaPrint_t logVaPrint)
{
#if (defined(CONFIG_MDS_LOG_FILTER_ENABLE) && (CONFIG_MDS_LOG_FILTER_ENABLE != 0))
    MDS_LOG_MODULE_PRINT(default, logVaPrint);
#else
    g_logVaPrintFunc = logVaPrint;
#endif
}

#if ((CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX > 0) &&                                                \
     ((defined(CONFIG_MDS_LOG_MSGQUEUE_NUMS) && (CONFIG_MDS_LOG_MSGQUEUE_NUMS > 0))))
static void MDS_LOG_ModuleWrite(const MDS_LOG_Module_t *module, uint8_t level, size_t va_cnt,
                                const char *fmt, ...)
{
    va_list va_args;
    va_start(va_args, fmt);

    do {
#if (defined(CONFIG_MDS_LOG_FILTER_ENABLE) && (CONFIG_MDS_LOG_FILTER_ENABLE != 0))
        if ((module != NULL) && (module->filter != NULL) && (module->filter->print != NULL)) {
            module->filter->print(module, level, va_cnt, fmt, va_args);
        } else {
            MDS_LOG_DEFAULT_PRINT(module, level, va_cnt, fmt, va_args);
        }
#else
        if (g_logVaPrintFunc != NULL) {
            g_logVaPrintFunc(module, level, va_cnt, fmt, va_args);
        }
#endif
    } while (0);

    va_end(va_args);
}

static void MDS_LOG_ThreadEntry(MDS_Arg_t arg)
{
    UNUSED(arg);

    if (g_logHandle.mq == NULL) {
        return;
    }

    for (;;) {
        MDS_LOG_Message_t *recv = NULL;
        MDS_Err_t err = MDS_MsgQueueRecvAcquire(g_logHandle.mq, &recv, MDS_TIMEOUT_FOREVER);

        while (MDS_ErrIsSame(err, MDS_EOK)) {
            MDS_LOG_ModuleWrite(
                recv->module, recv->level, recv->count, recv->fmt,
                MDS_ARGUMENT_FORLIST_N(CONFIG_MDS_LOG_MSGARGS_NUMS, __LOG_MESSAGE_ARG, (, ), recv));

            MDS_MsgQueueRecvRelease(g_logHandle.mq, recv);
            recv = NULL;

            err = MDS_MsgQueueRecvAcquire(g_logHandle.mq, &recv, MDS_TIMEOUT_NOWAIT);
        }

        // TODO: atomic
        MDS_Lock_t lock = MDS_CriticalLock(&(g_logHandle.spinlock));
        uint16_t loss = g_logHandle.loss;
        g_logHandle.loss = 0;
        MDS_CriticalRestore(&(g_logHandle.spinlock), lock);

        if (loss > 0) {
            MDS_LOG_P("log msg loss:" PRIu16, loss);
        }
    }
}

static MDS_Err_t MDS_LOG_ThreadInit(void)
{
    if (g_logHandle.mq == NULL) {
        g_logHandle.mq =
            MDS_MsgQueueCreate("mqlog", sizeof(MDS_LOG_Message_t), CONFIG_MDS_LOG_MSGQUEUE_NUMS);
    }
    if (g_logHandle.mq == NULL) {
        return (MDS_ENOMEM);
    }

    if (g_logHandle.thread == NULL) {
        g_logHandle.thread =
            MDS_ThreadCreate("thlog", MDS_LOG_ThreadEntry, NULL, CONFIG_MDS_LOG_THREAD_STACKSIZE,
                             MDS_THREAD_PRIORITY(CONFIG_MDS_LOG_THREAD_PRIORITY),
                             MDS_TIMEOUT_TICKS(CONFIG_MDS_LOG_THREAD_TICKS));
    }
    if (g_logHandle.thread == NULL) {
        return (MDS_ENOMEM);
    }

    MDS_SpinLockInit(&(g_logHandle.spinlock));

    return (MDS_ThreadStartup(g_logHandle.thread));
}

void MDS_LOG_ModulePrintf(const MDS_LOG_Module_t *module, uint8_t level, size_t va_cnt,
                          const char *fmt, ...)
{
    if (module == __THIS_LOG_MODULE_HANDLE) {
        return;
    }

    if ((g_logHandle.mq == NULL) && (!MDS_ErrIsSame(MDS_LOG_ThreadInit(), MDS_EOK))) {
        return;
    }

    MDS_LOG_Message_t log;

    log.module = module;
    log.level = level;
    log.count = (va_cnt < CONFIG_MDS_LOG_MSGARGS_NUMS) ? (va_cnt) : (CONFIG_MDS_LOG_MSGARGS_NUMS);
    log.timestamp = MDS_ClockGetTimestamp(NULL).ts;
    log.fmt = fmt;

    va_list va_args;
    va_start(va_args, fmt);
    for (size_t idx = 0; idx < log.count; idx++) {
        log.args[idx] = va_arg(va_args, int32_t);
    }
    va_end(va_args);

    // TODO: atomic
    MDS_Lock_t lock = MDS_CriticalLock(&(g_logHandle.spinlock));
    log.psn = g_logHandle.psn;
    g_logHandle.psn += 1;
    MDS_CriticalRestore(&(g_logHandle.spinlock), lock);

    MDS_Err_t err = MDS_MsgQueueSend(g_logHandle.mq, &log, sizeof(log), MDS_TIMEOUT_NOWAIT);
    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        // TODO: atomic
        lock = MDS_CriticalLock(&(g_logHandle.spinlock));
        g_logHandle.loss += 1;
        MDS_CriticalRestore(&(g_logHandle.spinlock), lock);
    }
}
#else
void MDS_LOG_ModulePrintf(const MDS_LOG_Module_t *module, uint8_t level, size_t va_cnt,
                          const char *fmt, ...)
{
    va_list va_args;

    va_start(va_args, fmt);

    MDS_Tick_t tick = MDS_ClockGetTickCount();

    do {
#if (defined(CONFIG_MDS_LOG_FILTER_ENABLE) && (CONFIG_MDS_LOG_FILTER_ENABLE != 0))
        if ((module != NULL) && (module->filter != NULL) && (module->filter->print != NULL)) {
            module->filter->print(module, level, tick, va_cnt, fmt, va_args);
        } else {
            MDS_LOG_DEFAULT_PRINT(module, level, tick, va_cnt, fmt, va_args);
        }
#else
        if (g_logVaPrintFunc != NULL) {
            g_logVaPrintFunc(module, level, tick, va_cnt, fmt, va_args);
        }
#endif
    } while (0);

    va_end(va_args);
}
#endif

/* Panic ------------------------------------------------------------------- */
__attribute__((weak, noreturn)) void MDS_CorePanicTrace(void)
{
    for (;;) {
    }
}

void MDS_PanicPrintf(size_t va_cnt, const char *fmt, ...)
{
    MDS_Tick_t tick = MDS_ClockGetTickCount();

#if (defined(CONFIG_MDS_LOG_ENABLE) && (CONFIG_MDS_LOG_ENABLE != 0))
    const MDS_LOG_Module_t *module = &(__LOG_MODULE_HANDLE(default));
    va_list va_args;

    va_start(va_args, fmt);
    do {
#if (defined(CONFIG_MDS_LOG_FILTER_ENABLE) && (CONFIG_MDS_LOG_FILTER_ENABLE != 0))
        if ((module != NULL) && (module->filter != NULL) && (module->filter->print != NULL)) {
            module->filter->print(module, MDS_LOG_LEVEL_ERR, tick, va_cnt, fmt, va_args);
        } else {
            MDS_LOG_DEFAULT_PRINT(module, MDS_LOG_LEVEL_ERR, tick, va_cnt, fmt, va_args);
        }
#else
        if (g_logVaPrintFunc != NULL) {
            g_logVaPrintFunc(module, MDS_LOG_LEVEL_ERR, tick, va_cnt, fmt, va_args);
        }
#endif
    } while (0);
    va_end(va_args);
#else
    UNUSED(va_cnt);
    UNUSED(fmt);
#endif
}

/* Compress ---------------------------------------------------------------- */
#ifndef MDS_LOG_COMPRESS_MAGIC
#define MDS_LOG_COMPRESS_MAGIC 0xD6 /* log 109 => 0x6D */
#endif

#ifndef MDS_LOG_COMPRESS_ARG_FIX
/* 0xFFFFFFFF => -1111111111 (2's complement: 0xBDC5CA39) */
#define MDS_LOG_COMPRESS_ARG_FIX(x) (((x) == 0xFFFFFFFF) ? (0xBDC5CA39) : (x))
#endif

size_t MDS_LOG_CompressStructVa(MDS_LOG_Compress_t *log, size_t level, size_t va_cnt,
                                const char *fmt, va_list va_args)
{
    // TODO: atomic
    static size_t logCompressPsn = 0;

    if (log == NULL) {
        return (0);
    }

    if (va_cnt > CONFIG_MDS_LOG_MSGARGS_NUMS) {
        va_cnt = CONFIG_MDS_LOG_MSGARGS_NUMS;
    }

    log->magic = MDS_LOG_COMPRESS_MAGIC;
    log->address = (uintptr_t)fmt & 0x00FFFFFF;
    log->level = level;
    log->count = va_cnt;
    // TODO: atomic
    log->psn = logCompressPsn++;
    log->timestamp = MDS_ClockGetTimestamp(NULL).ts;

    for (size_t idx = 0; idx < va_cnt; idx++) {
        uint32_t val = va_arg(va_args, uint32_t);
        log->args[idx] = MDS_LOG_COMPRESS_ARG_FIX(val);
    }

    return (sizeof(MDS_LOG_Compress_t) -
            (sizeof(uint32_t) * (CONFIG_MDS_LOG_MSGARGS_NUMS + va_cnt)));
}

size_t MDS_LOG_CompressSturctPrint(MDS_LOG_Compress_t *log, size_t level, size_t va_cnt,
                                   const char *fmt, ...)
{
    va_list va_args;

    va_start(va_args, fmt);
    size_t len = MDS_LOG_CompressStructVa(log, level, va_cnt, fmt, va_args);
    va_end(va_args);

    return (len);
}
