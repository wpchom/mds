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

/* Hook -------------------------------------------------------------------- */
MDS_HOOK_INIT(INTERRUPT_ENTER, MDS_Item_t irq);
MDS_HOOK_INIT(INTERRUPT_EXIT, MDS_Item_t irq);

/* Define ----------------------------------------------------------------- */
#if ((defined(__CC_ARM) && defined(__TARGET_FPU_VFP)) ||                                                               \
     (defined(__clang__) && defined(__VFP_FP__) && !defined(__SOFTFP__)) ||                                            \
     (defined(__ICCARM__) && defined(__ARMVFP__)) ||                                                                   \
     (defined(__GNUC__) && defined(__VFP_FP__) && !defined(__SOFTFP__)))
#define CORE_WITH_FPU 1
#else
#define CORE_WITH_FPU 0
#endif

#ifndef MDS_INTERRUPT_IRQ_NUMS
#define MDS_INTERRUPT_IRQ_NUMS 0
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
#if (defined(MDS_INTERRUPT_IRQ_NUMS) && (MDS_INTERRUPT_IRQ_NUMS > 0))
static __attribute__((section(".mds.isr"))) struct {
    MDS_IsrHandler_t handler;
    MDS_Arg_t *arg;
} g_mds_isr[MDS_INTERRUPT_IRQ_NUMS];

void Interrupt_Handler(uintptr_t ipsr)
{
    MDS_HOOK_CALL(INTERRUPT_ENTER, ipsr);

    if ((ipsr < ARRAY_SIZE(g_mds_isr)) && (g_mds_isr[ipsr].handler != NULL)) {
        g_mds_isr[ipsr].handler(g_mds_isr[ipsr].arg);
    }

    MDS_HOOK_CALL(INTERRUPT_EXIT, ipsr);
}

MDS_Err_t MDS_CoreInterruptRequestRegister(MDS_Item_t irq, MDS_IsrHandler_t handler, MDS_Arg_t *arg)
{
    uintptr_t ipsr = irq + 0x10;
    if (ipsr < ARRAY_SIZE(g_mds_isr)) {
        register MDS_Item_t lock = MDS_CoreInterruptLock();
        g_mds_isr[ipsr].handler = handler;
        g_mds_isr[ipsr].arg = arg;
        MDS_CoreInterruptRestore(lock);

        return (MDS_EOK);
    }

    return (MDS_ERANGE);
}
#endif

inline MDS_Item_t MDS_CoreInterruptCurrent(void)
{
    register intptr_t result;

    __asm volatile("mrs         %0, ipsr" : "=r"(result) : : "memory");

    return (result & 0x1FF);
}

inline MDS_Item_t MDS_CoreInterruptLock(void)
{
    register intptr_t result;

    __asm volatile("mrs         %0, primask" : "=r"(result));
    __asm volatile("cpsid       i" : : : "memory");

    return (result);
}

inline void MDS_CoreInterruptRestore(MDS_Item_t lock)
{
    __asm volatile("msr         primask, %0" : : "r"(lock) : "memory");
}

/* CoreThread -------------------------------------------------------------- */
void *MDS_CoreThreadStackInit(void *stackBase, size_t stackSize, void *entry, void *arg, void *exit)
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
        ((uintptr_t)(thread->stackPoint) > ((uintptr_t)(thread->stackBase) + (uintptr_t)(thread->stackSize)))) {
        MDS_LOG_F("[CORE] thread(%p) entry:%p stackpoint:%p stackbase:%p stacksize:%u overflow", thread, thread->entry,
                  thread->stackPoint, thread->stackBase, thread->stackSize);
        return (false);
    }

    return (true);
}

/* CoreScheduler ----------------------------------------------------------- */
#if (defined(MDS_THREAD_PRIORITY_MAX) && (MDS_THREAD_PRIORITY_MAX > 0))
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

void PendSV_Handler(void)
{
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    if (g_coreScheduler.swflag) {
        g_coreScheduler.swflag = false;

        if (g_coreScheduler.fromSP != NULL) {
            register uintptr_t fromSP = *(g_coreScheduler.fromSP);

            __asm volatile("mrs         %0, psp" : : "r"(fromSP));

#if CORE_WITH_FPU
            // exc_return[4] == 0
            __asm volatile("tst         lr, #0x10       \n"
                           "it          eq              \n"
                           "vstmdbeq    %0!, {d8 - d15} \n" : "=r"(fromSP));
#endif

            __asm volatile("stmfd       %0!, {r4 - r11}" : "=r"(fromSP));

#if CORE_WITH_FPU
            __asm volatile("stmfd       %0!, {lr}" : "=r"(fromSP));
#endif

            *(g_coreScheduler.fromSP) = fromSP;
        }

        if (g_coreScheduler.toSP != NULL) {
            register uintptr_t toSP = *(g_coreScheduler.toSP);

#if CORE_WITH_FPU
            __asm volatile("ldmfd       %0!, {lr}" : : "r"(toSP));
#endif

            __asm volatile("ldmfd       %0!, {r4 - r11}" : : "r"(toSP));

#if CORE_WITH_FPU
            // exc_return[4] == 0
            __asm volatile("tst         lr, #0x10       \n"
                           "it          eq              \n"
                           "vldmiaeq    %0!, {d8 - d15} \n" : : "r"(toSP));
#endif

            __asm volatile("msr         psp, %0" : : "r"(toSP));
        }
    }

    MDS_CoreInterruptRestore(lock);

    __asm volatile("orr         lr, lr, #0x04");
}
#endif

/* Backtrace --------------------------------------------------------------- */
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
    uint16_t ins2 = *((uint16_t *)(addr + 0x02U));

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

__attribute__((weak)) void MDS_CoreBackTrace(uintptr_t stackPoint, uintptr_t stackLimit)
{
#ifndef MDS_CORE_BACKTRACE_DEPTH
#define MDS_CORE_BACKTRACE_DEPTH 16
#endif

    for (size_t depth = 0; (depth < MDS_CORE_BACKTRACE_DEPTH) && (stackPoint < stackLimit);
         stackPoint += sizeof(uintptr_t)) {
        uintptr_t pc = *((uintptr_t *)stackPoint) - sizeof(uintptr_t) - 1;
        if (MDS_CoreStackPointerInCode(pc) && CORE_DisassemblyInsIsBL(pc)) {
            MDS_LOG_F("[BACKTRACE] %d: %p", depth, pc);
            depth += 1;
        }
    }
}

/* Interrupt --------------------------------------------------------------- */
#if (defined(MDS_CORE_BACKTRACE) && (MDS_CORE_BACKTRACE > 0))
static struct StackFrame *g_exceptionContext = NULL;

__attribute__((weak)) void MDS_CoreExceptionCallback(void)
{
}

static __attribute__((noreturn)) void MDS_CoreHardFaultException(struct ExceptionInfo *excInfo)
{
    if (g_exceptionContext == NULL) {
        g_exceptionContext = &(excInfo->stack);
        MDS_Thread_t *thread = MDS_KernelCurrentThread();

        MDS_LOG_F("[HARDFAULT] on ipsr:%u", MDS_CoreInterruptCurrent());
        MDS_LOG_F("r0 :%p r1 :%p r2 :%p r3 :%p ", g_exceptionContext->exception.r0, g_exceptionContext->exception.r1,
                  g_exceptionContext->exception.r2, g_exceptionContext->exception.r3);
        MDS_LOG_F("r4 :%p r5 :%p r6 :%p r7 :%p ", g_exceptionContext->r4, g_exceptionContext->r5,
                  g_exceptionContext->r6, g_exceptionContext->r7);
        MDS_LOG_F("r8 :%p r9 :%p r10:%p r11:%p ", g_exceptionContext->r8, g_exceptionContext->r9,
                  g_exceptionContext->r10, g_exceptionContext->r11);
        MDS_LOG_F("r12:%p lr :%p pc :%p psr:%p ", g_exceptionContext->exception.r12, g_exceptionContext->exception.lr,
                  g_exceptionContext->exception.pc, g_exceptionContext->exception.psr);

        if ((excInfo->exc_return & 0x04) == 0U) {
            uintptr_t msp = CORE_GetMSP();
            const uintptr_t **SCB_VTOR_STACK = (const uintptr_t **)0xE000ED08;

            MDS_LOG_F("hardfault on msp(%p)", msp);

            MDS_CoreBackTrace(msp, **SCB_VTOR_STACK);
        }

        if (thread != NULL) {
            uintptr_t psp = CORE_GetPSP();

            MDS_LOG_F("current thread(%p) entry:%p psp:%p stackbase:%p stacksize:%u", thread, thread->entry, psp,
                      thread->stackBase, thread->stackSize);

            MDS_CoreBackTrace(psp, (uintptr_t)(thread->stackBase) + thread->stackSize);
        }
    }

    MDS_CoreExceptionCallback();

    for (;;) {
    }
}

__attribute__((naked, noreturn)) void HardFault_Handler(void)
{
    // exc_return[2] == 0
    __asm volatile("tst         lr, #0x04       \n"
                   "ite         eq              \n"
                   "mrseq       r0, msp         \n"
                   "mrsne       r0, psp         \n");

#if CORE_WITH_FPU
    // exc_return[4] == 0
    __asm volatile("tst         lr, #0x10       \n"
                   "it          eq              \n"
                   "vstmdbeq    r0!, {d8 - d15} \n");
#endif

    __asm volatile("stmfd       r0!, {r4 - r11}");
#if CORE_WITH_FPU
    __asm volatile("stmfd       r0!, {lr}");
#endif
    __asm volatile("stmfd       r0!, {lr}");

    __asm volatile("bl          %0" : : "i"(MDS_CoreHardFaultException));

    __asm volatile("b           .");
}

__attribute((noreturn, alias("HardFault_Handler"))) void MemManage_Handler(void);
__attribute((noreturn, alias("HardFault_Handler"))) void BusFault_Handler(void);
__attribute((noreturn, alias("HardFault_Handler"))) void UsageFault_Handler(void);
#endif
