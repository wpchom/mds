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
#include "kernel/mds_sys.h"

/* Define ----------------------------------------------------------------- */
MDS_LOG_MODULE_DECLARE(kernel, CONFIG_MDS_KERNEL_LOG_LEVEL);

#if ((defined(__CC_ARM) && defined(__TARGET_FPU_VFP)) ||                                          \
     (defined(__clang__) && defined(__VFP_FP__) && !defined(__SOFTFP__)) ||                       \
     (defined(__ICCARM__) && defined(__ARMVFP__)) ||                                              \
     (defined(__GNUC__) && defined(__VFP_FP__) && !defined(__SOFTFP__)))
#define CORE_WITH_FPU 1
#else
#define CORE_WITH_FPU 0
#endif

struct SCB_Typedef {
    volatile uint32_t CPUID;
    volatile uint32_t ICSR;
    volatile uint32_t VTOR;
    volatile uint32_t AIRCR;
    volatile uint32_t SCR;
    volatile uint32_t CCR;
    volatile uint32_t SHPR1;
    volatile uint32_t SHPR2;
    volatile uint32_t SHPR3;
};

#define SCB ((struct SCB_Typedef *)0xE000ED00)

/* Exception ---------------------------------------------------------------
 * MSP                                !< 0 Stack
 * Reset_Handler                      !< 1 Reset
 * NMI_Handler              = -14,    !< 2 Non Maskable Interrupt
 * HardFault_Handler        = -13,    !< 3 Cortex-M Hard Fault Interrupt
 * MemManage_Handler        = -12,    !< 4 Cortex-M Memory Management Interrupt
 * BusFault_Handler         = -11,    !< 5 Cortex-M Bus Fault Interrupt
 * UsageFault_Handler       = -10,    !< 6 Cortex-M Usage Fault Interrupt
 * SVC_Handler              = -5,     !< 11 Cortex-M SV Call Interrupt
 * DebugMon_Handler         = -4,     !< 12 Cortex-M Debug Monitor Interrupt
 * PendSV_Handler           = -2,     !< 14 Cortex-M Pend SV Interrupt
 * SysTick_Handler          = -1,     !< 15 Cortex-M System Tick Interrupt
 */

/* StackFrame -------------------------------------------------------------- */
struct ExceptionStackFrame {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;

#if CORE_WITH_FPU
    uint32_t s0;
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
    uint32_t s12;
    uint32_t s13;
    uint32_t s14;
    uint32_t s15;
    uint32_t fpscr;
    uint32_t rsv;
#endif
};

struct StackFrame {
#if CORE_WITH_FPU
    uint32_t exc_flag;
#endif

    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;

#if CORE_WITH_FPU
    uint32_t s16;
    uint32_t s17;
    uint32_t s18;
    uint32_t s19;
    uint32_t s20;
    uint32_t s21;
    uint32_t s22;
    uint32_t s23;
    uint32_t s24;
    uint32_t s25;
    uint32_t s26;
    uint32_t s27;
    uint32_t s28;
    uint32_t s29;
    uint32_t s30;
    uint32_t s31;
#endif

    struct ExceptionStackFrame exception;
};

struct ExceptionInfo {
    uint32_t exc_return;
    struct StackFrame stack;
};

/* CoreFunction ------------------------------------------------------------ */
size_t MDS_SchedulerFFS(size_t value)
{
    if (value != 0) {
        __asm volatile("rbit        %0, %0" : "=r"(value));
        __asm volatile("clz         %0, %0" : : "r"(value));
        __asm volatile("adds        %0, %0, #1" : "=r"(value) : "r"(value));
    }

    return (value);
}

__attribute__((always_inline)) static inline uintptr_t CORE_GetMSP(void)
{
    uintptr_t result;

    __asm volatile("mrs         %0, msp" : "=r"(result));

    return (result);
}

__attribute__((always_inline)) static inline uintptr_t CORE_GetPSP(void)
{
    uintptr_t result;

    __asm volatile("mrs         %0, psp" : "=r"(result));

    return (result);
}

inline void MDS_CoreIdleSleep(void)
{
    __asm volatile("wfi");
}

/* CoreInterrupt ----------------------------------------------------------- */
inline intptr_t MDS_CoreInterruptCurrent(void)
{
    register intptr_t result;

    __asm volatile("mrs         %0, ipsr" : "=r"(result) : : "memory");

    return (result & 0x1FF);
}

inline MDS_Lock_t MDS_CoreInterruptLock(void)
{
    register intptr_t result;

    __asm volatile("mrs         %0, primask" : "=r"(result));
    __asm volatile("cpsid       i" : : : "memory");
    __asm volatile("isb" : : : "memory");

    return ((MDS_Lock_t) {.key = result});
}

inline void MDS_CoreInterruptRestore(MDS_Lock_t lock)
{
    __asm volatile("msr         primask, %0" : : "r"(lock.key) : "memory");
    __asm volatile("isb" : : : "memory");
}

/* CoreThread -------------------------------------------------------------- */
void *MDS_CoreThreadStackInit(void *stackBase, size_t stackSize, void *entry, void *arg,
                              void *exit)
{
    uintptr_t sp = VALUE_ALIGN((uintptr_t)(stackBase) + stackSize, sizeof(uint64_t));
    struct StackFrame *stack = (struct StackFrame *)(sp - sizeof(struct StackFrame));

    MDS_MemBuffSet(stackBase, '@', stackSize);
    for (size_t idx = 0; idx < (sizeof(struct StackFrame) / sizeof(uint32_t)); idx++) {
        ((uint32_t *)stack)[idx] = 0xDEADBEEF;
    }

    stack->exception.r0 = (uint32_t)(arg);
    stack->exception.r1 = 0;
    stack->exception.r2 = 0;
    stack->exception.r3 = 0;
    stack->exception.r12 = 0;
    stack->exception.lr = (uint32_t)(exit);
    stack->exception.pc = (uint32_t)(entry);
    stack->exception.psr = 0x01000000;

#if CORE_WITH_FPU
    stack->exc_flag = 0xFFFFFFED;
#endif

    return (stack);
}

bool MDS_CoreThreadStackCheck(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread != NULL);

    if (((*(uint8_t *)(thread->stackBase)) != '@') ||
        ((uintptr_t)(thread->stackPoint) <= (uintptr_t)(thread->stackBase)) ||
        ((uintptr_t)(thread->stackPoint) >
         ((uintptr_t)(thread->stackBase) + (uintptr_t)(thread->stackSize)))) {
        return (false);
    }

#if (defined(CONFIG_MDS_KERNEL_STATS_ENABLE) && (CONFIG_MDS_KERNEL_STATS_ENABLE != 0))
#endif

    return (true);
}

/* CoreScheduler ----------------------------------------------------------- */
#if (defined(CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX) &&                                            \
     (CONFIG_MDS_KERNEL_THREAD_PRIORITY_MAX != 0))
static struct CoreScheduler {
    uintptr_t swflag;
    uintptr_t *fromSP;
    uintptr_t *toSP;
} g_coreScheduler;

void MDS_CoreSchedulerStartup(void *toSP)
{
    g_coreScheduler.swflag = true;
    g_coreScheduler.fromSP = NULL;
    g_coreScheduler.toSP = toSP;

#if CORE_WITH_FPU
    uintptr_t control;
    // CONTROL.FPCA = 0
    __asm volatile("mrs         %0, control" : "=r"(control));
    __asm volatile("bic         %0, #0x04" : "=r"(control));
    __asm volatile("msr         control, %0" : : "r"(control));
#endif

    // SCB_SHPR3 Priority: PendSV = 0xFF, SysTick = 0x00
    SCB->SHPR3 = 0x00FF0000;

    // SCB_ICSR PendSV trig
    SCB->ICSR = 0x10000000;

    __asm volatile("cpsie       f" : : : "memory");
    __asm volatile("cpsie       i" : : : "memory");
}

void MDS_CoreSchedulerSwitch(void *fromSP, void *toSP)
{
    if (g_coreScheduler.swflag == false) {
        g_coreScheduler.swflag = true;
        g_coreScheduler.fromSP = fromSP;
    }

    g_coreScheduler.toSP = toSP;

    // SCB_ICSR PendSV trig
    SCB->ICSR = 0x10000000;
}

__attribute__((naked)) void PendSV_SwtichThread(void);
__attribute__((naked)) void PendSV_SwtichExit(void);
__attribute__((naked)) void PendSV_Handler(void)
{
    // r0 = MDS_CoreInterruptLock();
    __asm volatile("mrs         r0, primask");
    __asm volatile("cpsid       i" : : : "memory");

    // r1 = g_coreScheduler
    __asm volatile("ldr         r1, =%0" : : "X"(&g_coreScheduler));

    // if (g_coreScheduler.swflag) {
    __asm volatile("ldr         r2, [r1, #0x00]");
    __asm volatile("cbz         r2, %0" : : "X"(PendSV_SwtichExit));

    // g_coreScheduler.swflag = false;
    __asm volatile("mov         r2, #0");
    __asm volatile("str         r2, [r1, #0x00]");

    // if (g_coreScheduler.fromSP != NULL)
    __asm volatile("ldr         r2, [r1, #0x04]");
    __asm volatile("cbz         r2, %0" : : "X"(PendSV_SwtichThread));

    __asm volatile("ldr         r3, [r2]");
    __asm volatile("mrs         r3, psp");

#if CORE_WITH_FPU
    // exc_return[4] == 0
    __asm volatile(
        "tst         lr, #0x10       \n" "it          eq              \n" "vstmdbeq    r3!, {d8 - d15} \n");
#endif

    __asm volatile("stmfd       r3!, {r4 - r11}");

#if CORE_WITH_FPU
    __asm volatile("stmfd       r3!, {lr}");
#endif

    __asm volatile("str         r3, [r2]");

    __asm volatile("PendSV_SwtichThread:");

    // if (g_coreScheduler.toSP != NULL)
    __asm volatile("ldr         r2, [r1, #0x08]");
    __asm volatile("cbz         r2, %0" : : "X"(PendSV_SwtichExit));

    __asm volatile("ldr         r3, [r2]");

#if CORE_WITH_FPU
    __asm volatile("ldmfd       r3!, {lr}");
#endif

    __asm volatile("ldmfd       r3!, {r4 - r11}");

#if CORE_WITH_FPU
    // exc_return[4] == 0
    __asm volatile(
        "tst         lr, #0x10       \n" "it          eq              \n" "vldmiaeq    r3!, {d8 - d15} \n");
#endif

    __asm volatile("msr         psp, r3");

    // PendSV_SwtichExit:
    __asm volatile("PendSV_SwtichExit:");

    // MDS_CoreInterruptRestore(r0);
    __asm volatile("msr         primask, r0" : : : "memory");

    __asm volatile("orr         lr, lr, #0x04");

    __asm volatile("bx          lr");
}
#endif

/* Backtrace --------------------------------------------------------------- */
#if (defined(CONFIG_MDS_CORE_BACKTRACE_DEPTH) && (CONFIG_MDS_CORE_BACKTRACE_DEPTH != 0))
__attribute__((weak)) bool MDS_CoreStackPointerInCode(uintptr_t pc)
{
#if defined(__IAR_SYSTEMS_ICC__)
#pragma section(".text")
    const uintptr_t __text_start[] = __section_start(".text");
    const uintptr_t __text_end[] = __section_end(".text");
#else
    extern void __text_start(void);
    extern void __text_end(void);
#endif
    return (((uintptr_t)__text_start <= pc) && (pc <= (uintptr_t)__text_end)) ? (true) : (false);
}

static bool CORE_DisassemblyInsIsBL(uintptr_t addr)
{
    uint16_t ins1 = *((uint16_t *)addr);
    uint16_t ins2 = *((uint16_t *)(addr + sizeof(uint16_t)));

#define BL_INS_MASK  0xF800
#define BL_INS_HIGH  0xF800
#define BL_INS_LOW   0xF000
#define BLX_INX_MASK 0xFF00
#define BLX_INX      0x4700

    if (((ins2 & BL_INS_MASK) == BL_INS_HIGH) && ((ins1 & BL_INS_MASK) == BL_INS_LOW)) {
        return (true);
    } else if ((ins2 & BLX_INX_MASK) == BLX_INX) {
        return (true);
    } else {
        return (false);
    }
}

static void CORE_StackBacktrace(uintptr_t stackPoint, uintptr_t stackLimit, size_t depth)
{
    for (size_t dp = 0; (dp < depth) && (stackPoint < stackLimit);
         stackPoint += sizeof(uintptr_t)) {
        uintptr_t pc = *((uintptr_t *)stackPoint) - sizeof(uintptr_t) - 1;
        if (MDS_CoreStackPointerInCode(pc) && CORE_DisassemblyInsIsBL(pc)) {
            MDS_LOG_F("[BACKTRACE] %zu: %p", dp, (void *)pc);
            dp += 1;
        }
    }
}
#endif

/* Exception --------------------------------------------------------------- */
static struct StackFrame *g_exceptionContext = NULL;

__attribute__((weak)) void MDS_CoreExceptionCallback(bool exit)
{
    UNUSED(exit);
}

static void CORE_ExceptionBacktrace(void)
{
#if (defined(CONFIG_MDS_CORE_BACKTRACE_DEPTH) && (CONFIG_MDS_CORE_BACKTRACE_DEPTH != 0))
    uintptr_t msp = CORE_GetMSP();
    const uintptr_t **SCB_VTOR_STACK = (const uintptr_t **)0xE000ED08;
    MDS_LOG_F("msp:%p stacklimit:%p backtrace", (void *)msp, (void *)**SCB_VTOR_STACK);
    CORE_StackBacktrace(msp, **SCB_VTOR_STACK, CONFIG_MDS_CORE_BACKTRACE_DEPTH);

    MDS_Thread_t *thread = MDS_KernelCurrentThread();
    if (thread != NULL) {
        uintptr_t psp = CORE_GetPSP();
        MDS_LOG_F("current thread(%p) entry:%p psp:%p stackbase:%p stacksize:%zu backtrace",
                  thread, thread->entry, (void *)psp, thread->stackBase, thread->stackSize);
        CORE_StackBacktrace(psp, (uintptr_t)(thread->stackBase) + thread->stackSize,
                            CONFIG_MDS_CORE_BACKTRACE_DEPTH);
    }
#endif
}

void MDS_CorePanicTrace(void)
{
    MDS_CoreExceptionCallback(false);

    CORE_ExceptionBacktrace();

    MDS_CoreExceptionCallback(true);
}

static __attribute__((noreturn)) void MDS_CoreHardFaultException(struct ExceptionInfo *excInfo)
{
    MDS_CoreExceptionCallback(false);

    if (g_exceptionContext == NULL) {
        g_exceptionContext = &(excInfo->stack);

        MDS_LOG_F("[HARDFAULT] on ipsr:%zx exc_return:%lx", MDS_CoreInterruptCurrent(),
                  excInfo->exc_return);
        MDS_LOG_F("r0 :%p r1 :%p r2 :%p r3 :%p ", (void *)(g_exceptionContext->exception.r0),
                  (void *)(g_exceptionContext->exception.r1),
                  (void *)(g_exceptionContext->exception.r2),
                  (void *)(g_exceptionContext->exception.r3));
        MDS_LOG_F("r4 :%p r5 :%p r6 :%p r7 :%p ", (void *)(g_exceptionContext->r4),
                  (void *)(g_exceptionContext->r5), (void *)(g_exceptionContext->r6),
                  (void *)(g_exceptionContext->r7));
        MDS_LOG_F("r8 :%p r9 :%p r10:%p r11:%p ", (void *)(g_exceptionContext->r8),
                  (void *)(g_exceptionContext->r9), (void *)(g_exceptionContext->r10),
                  (void *)(g_exceptionContext->r11));
        MDS_LOG_F("r12:%p lr :%p pc :%p psr:%p ", (void *)(g_exceptionContext->exception.r12),
                  (void *)(g_exceptionContext->exception.lr),
                  (void *)(g_exceptionContext->exception.pc),
                  (void *)(g_exceptionContext->exception.psr));

        CORE_ExceptionBacktrace();
    }

    MDS_CoreExceptionCallback(true);

    for (;;) {
    }
}

__attribute__((naked, noreturn)) void HardFault_Handler(void)
{
    // exc_return[2] == 0
    __asm volatile(
        "tst         lr, #0x04       \n" "ite         eq              \n" "mrseq       r0, msp         \n" "mrsne       r0, psp         \n");

#if CORE_WITH_FPU
    // exc_return[4] == 0
    __asm volatile(
        "tst         lr, #0x10       \n" "it          eq              \n" "vstmdbeq    r0!, {d8 - d15} \n");
#endif

    __asm volatile("stmfd       r0!, {r4 - r11}");
#if CORE_WITH_FPU
    __asm volatile("stmfd       r0!, {lr}");
#endif
    __asm volatile("stmfd       r0!, {lr}");

    __asm volatile("bl          %0" : : "X"(MDS_CoreHardFaultException));

    __asm volatile("b           .");
}

__attribute((noreturn, alias("HardFault_Handler"))) void MemManage_Handler(void);
__attribute((noreturn, alias("HardFault_Handler"))) void BusFault_Handler(void);
__attribute((noreturn, alias("HardFault_Handler"))) void UsageFault_Handler(void);
