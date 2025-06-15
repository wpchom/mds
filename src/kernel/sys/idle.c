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
#include "kernel.h"

/* Define ------------------------------------------------------------------ */
#ifndef CONFIG_MDS_IDLE_THREAD_STACKSIZE
#define CONFIG_MDS_IDLE_THREAD_STACKSIZE 384
#endif

#ifndef CONFIG_MDS_IDLE_THREAD_TICKS
#define CONFIG_MDS_IDLE_THREAD_TICKS 16
#endif

#ifndef CONFIG_MDS_IDLE_THREAD_HOOKS
#define CONFIG_MDS_IDLE_THREAD_HOOKS 0
#endif

/* Variable ---------------------------------------------------------------- */
// multi idle for each cpu
static MDS_Thread_t g_idleThread;
static uint8_t g_idleStack[CONFIG_MDS_IDLE_THREAD_STACKSIZE];

/* Function ---------------------------------------------------------------- */
__attribute__((weak)) void MDS_IdleLowPowerControl(void)
{
    MDS_CoreIdleSleep();
}

MDS_Thread_t *MDS_KernelIdleThread(void)
{
    return (&g_idleThread);
}

#if (defined(CONFIG_MDS_IDLE_THREAD_HOOKS) && (CONFIG_MDS_IDLE_THREAD_HOOKS != 0))
static void (*g_idleHook[CONFIG_MDS_IDLE_THREAD_HOOKS])(void);

MDS_Err_t MDS_KernelAddIdleHook(void (*hook)(void))
{
    size_t idx;
    MDS_Err_t err = MDS_ERROR;
    MDS_Item_t lock = MDS_CoreInterruptLock();

    for (idx = 0; idx < ARRAY_SIZE(g_idleHook); idx++) {
        if (g_idleHook[idx] == NULL) {
            g_idleHook[idx] = hook;
            err = MDS_EOK;
            break;
        }
    }

    MDS_CoreInterruptRestore(lock);

    return (err);
}

MDS_Err_t MDS_KernelDelIdleHook(void (*hook)(void))
{
    size_t idx;
    MDS_Err_t err = MDS_ERROR;
    MDS_Item_t lock = MDS_CoreInterruptLock();

    for (idx = 0; idx < ARRAY_SIZE(g_idleHook); idx++) {
        if (g_idleHook[idx] == hook) {
            g_idleHook[idx] = NULL;
            err = MDS_EOK;
            break;
        }
    }

    MDS_CoreInterruptRestore(lock);

    return (err);
}
#endif

static void IDLE_ThreadDefunct(void)
{
    MDS_LOOP {
        MDS_Thread_t *thread = MDS_KernelPopDefunct();
        if (thread == NULL) {
            break;
        }

        if (!MDS_ObjectIsCreated(&(thread->object))) {
            MDS_ObjectDeInit(&(thread->object));
        } else {
            MDS_SysMemFree(thread->stackBase);
            MDS_ObjectDestroy(&(thread->object));
        }
    }
}

static __attribute__((noreturn)) void IDLE_ThreadEntry(MDS_Arg_t *arg)
{
    UNUSED(arg);

    MDS_LOOP {
        IDLE_ThreadDefunct();

#if (defined(CONFIG_MDS_THREAD_IDLE_HOOK_SIZE) && (CONFIG_MDS_THREAD_IDLE_HOOK_SIZE != 0))
        size_t idx;
        for (idx = 0; idx < ARRAY_SIZE(g_idleHook); idx++) {
            if (g_idleHook[idx] != NULL) {
                g_idleHook[idx]();
            }
        }
#endif

        MDS_IdleLowPowerControl();
    }
}

void MDS_IdleThreadInit(void)
{
    MDS_Err_t err = MDS_ThreadInit(&g_idleThread, "idle", IDLE_ThreadEntry, NULL, &g_idleStack,
                                   sizeof(g_idleStack),
                                   MDS_THREAD_PRIORITY(CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX),
                                   MDS_TIMEOUT_TICKS(CONFIG_MDS_IDLE_THREAD_TICKS));

    if (err == MDS_EOK) {
        MDS_ThreadStartup(&g_idleThread);
    } else {
        MDS_PANIC("idle thread initialize err:%d", err);
    }
}
