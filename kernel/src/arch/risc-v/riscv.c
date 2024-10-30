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

/* Define ------------------------------------------------------------------ */
#define MSTATUS_UIE  0x00000001
#define MSTATUS_SIE  0x00000002
#define MSTATUS_HIE  0x00000004
#define MSTATUS_MIE  0x00000008
#define MSTATUS_UPIE 0x00000010
#define MSTATUS_SPIE 0x00000020
#define MSTATUS_HPIE 0x00000040
#define MSTATUS_MPIE 0x00000080
#define MSTATUS_SPP  0x00000100
#define MSTATUS_MPP  0x00001800
#define MSTATUS_FS   0x00006000
#define MSTATUS_XS   0x00018000

#if (defined(__riscv_xlen) && (__riscv_xlen == 64))
#define STORE    "sd "
#define LOAD     "ld "
#define REGBYTES 8

#define MSTATUS_SD   0x8000000000000000
#define MCAUSE_INT   0x8000000000000000
#define MCAUSE_CAUSE 0x7FFFFFFFFFFFFFFF

#else
#define STORE    "sw "
#define LOAD     "lw "
#define REGBYTES 4

#define MSTATUS_SD   0x80000000
#define MCAUSE_INT   0x80000000
#define MCAUSE_CAUSE 0x7FFFFFFF

#endif

#if (defined(__riscv_flen) && (__riscv_flen == 64))
#define FPSTORE    "fsd "
#define FPLOAD     "fld "
#define FPREGBYTES 8
typedef uint64_t rv_fpreg_t;
#else
#define FPSTORE    "fsw "
#define FPLOAD     "flw "
#define FPREGBYTES 4
typedef uint32_t rv_fpreg_t;
#endif

/* StackFrame -------------------------------------------------------------- */
struct StackFrame {
    uintptr_t mepc;     // mepc         | mchaine exception register
    uintptr_t ra;       // x1  - ra     | return address
    uintptr_t mstatus;  // mstatus      | machine status register
    uintptr_t fcsr;     // fcsr         | float point constrl and status register
    uintptr_t tp;       // x4  - tp     | thread pointer
    uintptr_t t0;       // x5  - t0     | temporary register 0
    uintptr_t t1;       // x6  - t1     | temporary register 1
    uintptr_t t2;       // x7  - t2     | temporary register 2
    uintptr_t s0_fp;    // x8  - s0/fp  | saved register 0 or frame pointer
    uintptr_t s1;       // x9  - s1     | saved register 1
    uintptr_t a0;       // x10 - a0     | return value or function argument 0
    uintptr_t a1;       // x11 - a1     | return value or function argument 1
    uintptr_t a2;       // x12 - a2     | function argument 2
    uintptr_t a3;       // x13 - a3     | function argument 3
    uintptr_t a4;       // x14 - a4     | function argument 4
    uintptr_t a5;       // x15 - a5     | function argument 5
#ifndef __riscv_32e
    uintptr_t a6;   // x16 - a6         | function argument 6
    uintptr_t a7;   // x17 - a7         | function argument 7
    uintptr_t s2;   // x18 - s2         | saved register 2
    uintptr_t s3;   // x19 - s3         | saved register 3
    uintptr_t s4;   // x20 - s4         | saved register 4
    uintptr_t s5;   // x21 - s5         | saved register 5
    uintptr_t s6;   // x22 - s6         | saved register 6
    uintptr_t s7;   // x23 - s7         | saved register 7
    uintptr_t s8;   // x24 - s8         | saved register 8
    uintptr_t s9;   // x25 - s9         | saved register 9
    uintptr_t s10;  // x26 - s10        | saved register 10
    uintptr_t s11;  // x27 - s11        | saved register 11
    uintptr_t t3;   // x28 - t3         | temporary register 3
    uintptr_t t4;   // x29 - t4         | temporary register 4
    uintptr_t t5;   // x30 - t5         | temporary register 5
    uintptr_t t6;   // x31 - t6         | temporary register 6
#endif

#ifdef __riscv_flen
    struct rv_fp {
        rv_fpreg_t f0;
        rv_fpreg_t f1;
        rv_fpreg_t f2;
        rv_fpreg_t f3;
        rv_fpreg_t f4;
        rv_fpreg_t f5;
        rv_fpreg_t f6;
        rv_fpreg_t f7;
        rv_fpreg_t f8;
        rv_fpreg_t f9;
        rv_fpreg_t f10;
        rv_fpreg_t f11;
        rv_fpreg_t f12;
        rv_fpreg_t f13;
        rv_fpreg_t f14;
        rv_fpreg_t f15;
        rv_fpreg_t f16;
        rv_fpreg_t f17;
        rv_fpreg_t f18;
        rv_fpreg_t f19;
        rv_fpreg_t f20;
        rv_fpreg_t f21;
        rv_fpreg_t f22;
        rv_fpreg_t f23;
        rv_fpreg_t f24;
        rv_fpreg_t f25;
        rv_fpreg_t f26;
        rv_fpreg_t f27;
        rv_fpreg_t f28;
        rv_fpreg_t f29;
        rv_fpreg_t f30;
        rv_fpreg_t f31;
    } fp;
#endif
};

__attribute__((always_inline)) inline void ContextStackSave(void)
{
#ifdef __riscv_flen
    __asm volatile("addi        sp, sp, -(%0)" : : "i"(32 * FPREGBYTES));

    __asm volatile(FPSTORE "    f0,  (%0)(sp)" : : "i"(0 * FPREGBYTES));
    __asm volatile(FPSTORE "    f1,  (%0)(sp)" : : "i"(1 * FPREGBYTES));
    __asm volatile(FPSTORE "    f2,  (%0)(sp)" : : "i"(2 * FPREGBYTES));
    __asm volatile(FPSTORE "    f3,  (%0)(sp)" : : "i"(3 * FPREGBYTES));
    __asm volatile(FPSTORE "    f4,  (%0)(sp)" : : "i"(4 * FPREGBYTES));
    __asm volatile(FPSTORE "    f5,  (%0)(sp)" : : "i"(5 * FPREGBYTES));
    __asm volatile(FPSTORE "    f6,  (%0)(sp)" : : "i"(6 * FPREGBYTES));
    __asm volatile(FPSTORE "    f7,  (%0)(sp)" : : "i"(7 * FPREGBYTES));
    __asm volatile(FPSTORE "    f8,  (%0)(sp)" : : "i"(8 * FPREGBYTES));
    __asm volatile(FPSTORE "    f9,  (%0)(sp)" : : "i"(9 * FPREGBYTES));
    __asm volatile(FPSTORE "    f10, (%0)(sp)" : : "i"(10 * FPREGBYTES));
    __asm volatile(FPSTORE "    f11, (%0)(sp)" : : "i"(11 * FPREGBYTES));
    __asm volatile(FPSTORE "    f12, (%0)(sp)" : : "i"(12 * FPREGBYTES));
    __asm volatile(FPSTORE "    f13, (%0)(sp)" : : "i"(13 * FPREGBYTES));
    __asm volatile(FPSTORE "    f14, (%0)(sp)" : : "i"(14 * FPREGBYTES));
    __asm volatile(FPSTORE "    f15, (%0)(sp)" : : "i"(15 * FPREGBYTES));
    __asm volatile(FPSTORE "    f16, (%0)(sp)" : : "i"(16 * FPREGBYTES));
    __asm volatile(FPSTORE "    f17, (%0)(sp)" : : "i"(17 * FPREGBYTES));
    __asm volatile(FPSTORE "    f18, (%0)(sp)" : : "i"(18 * FPREGBYTES));
    __asm volatile(FPSTORE "    f19, (%0)(sp)" : : "i"(19 * FPREGBYTES));
    __asm volatile(FPSTORE "    f20, (%0)(sp)" : : "i"(20 * FPREGBYTES));
    __asm volatile(FPSTORE "    f21, (%0)(sp)" : : "i"(21 * FPREGBYTES));
    __asm volatile(FPSTORE "    f22, (%0)(sp)" : : "i"(22 * FPREGBYTES));
    __asm volatile(FPSTORE "    f23, (%0)(sp)" : : "i"(23 * FPREGBYTES));
    __asm volatile(FPSTORE "    f24, (%0)(sp)" : : "i"(24 * FPREGBYTES));
    __asm volatile(FPSTORE "    f25, (%0)(sp)" : : "i"(25 * FPREGBYTES));
    __asm volatile(FPSTORE "    f26, (%0)(sp)" : : "i"(26 * FPREGBYTES));
    __asm volatile(FPSTORE "    f27, (%0)(sp)" : : "i"(27 * FPREGBYTES));
    __asm volatile(FPSTORE "    f28, (%0)(sp)" : : "i"(28 * FPREGBYTES));
    __asm volatile(FPSTORE "    f29, (%0)(sp)" : : "i"(29 * FPREGBYTES));
    __asm volatile(FPSTORE "    f30, (%0)(sp)" : : "i"(30 * FPREGBYTES));
    __asm volatile(FPSTORE "    f31, (%0)(sp)" : : "i"(31 * FPREGBYTES));
#endif

#ifndef __riscv_32e
    __asm volatile("addi        sp, sp, -(%0)" : : "i"(32 * REGBYTES));
#else
    __asm volatile("addi        sp, sp, -(%0)" : : "i"(16 * REGBYTES));
#endif

    __asm volatile(STORE "      x1,  (%0)(sp)" : : "i"(1 * REGBYTES));
    __asm volatile(STORE "      x4,  (%0)(sp)" : : "i"(4 * REGBYTES));
    __asm volatile(STORE "      x5,  (%0)(sp)" : : "i"(5 * REGBYTES));
    __asm volatile(STORE "      x6,  (%0)(sp)" : : "i"(6 * REGBYTES));
    __asm volatile(STORE "      x7,  (%0)(sp)" : : "i"(7 * REGBYTES));
    __asm volatile(STORE "      x8,  (%0)(sp)" : : "i"(8 * REGBYTES));
    __asm volatile(STORE "      x9,  (%0)(sp)" : : "i"(9 * REGBYTES));
    __asm volatile(STORE "      x10, (%0)(sp)" : : "i"(10 * REGBYTES));
    __asm volatile(STORE "      x11, (%0)(sp)" : : "i"(11 * REGBYTES));
    __asm volatile(STORE "      x12, (%0)(sp)" : : "i"(12 * REGBYTES));
    __asm volatile(STORE "      x13, (%0)(sp)" : : "i"(13 * REGBYTES));
    __asm volatile(STORE "      x14, (%0)(sp)" : : "i"(14 * REGBYTES));
    __asm volatile(STORE "      x15, (%0)(sp)" : : "i"(15 * REGBYTES));
#ifndef __riscv_32e
    __asm volatile(STORE "      x16, (%0)(sp)" : : "i"(16 * REGBYTES));
    __asm volatile(STORE "      x17, (%0)(sp)" : : "i"(17 * REGBYTES));
    __asm volatile(STORE "      x18, (%0)(sp)" : : "i"(18 * REGBYTES));
    __asm volatile(STORE "      x19, (%0)(sp)" : : "i"(19 * REGBYTES));
    __asm volatile(STORE "      x20, (%0)(sp)" : : "i"(20 * REGBYTES));
    __asm volatile(STORE "      x21, (%0)(sp)" : : "i"(21 * REGBYTES));
    __asm volatile(STORE "      x22, (%0)(sp)" : : "i"(22 * REGBYTES));
    __asm volatile(STORE "      x23, (%0)(sp)" : : "i"(23 * REGBYTES));
    __asm volatile(STORE "      x24, (%0)(sp)" : : "i"(24 * REGBYTES));
    __asm volatile(STORE "      x25, (%0)(sp)" : : "i"(25 * REGBYTES));
    __asm volatile(STORE "      x26, (%0)(sp)" : : "i"(26 * REGBYTES));
    __asm volatile(STORE "      x27, (%0)(sp)" : : "i"(27 * REGBYTES));
    __asm volatile(STORE "      x28, (%0)(sp)" : : "i"(28 * REGBYTES));
    __asm volatile(STORE "      x29, (%0)(sp)" : : "i"(29 * REGBYTES));
    __asm volatile(STORE "      x30, (%0)(sp)" : : "i"(30 * REGBYTES));
    __asm volatile(STORE "      x31, (%0)(sp)" : : "i"(31 * REGBYTES));
#endif

#ifdef __riscv_flen
    __asm volatile("csrrs       t0, fcsr, x0");
    __asm volatile(STORE "      t0, (%0)(sp)" : : "i"(3 * REGBYTES));
#endif

    __asm volatile("csrr        t0,  mstatus");
    __asm volatile(STORE "      t0,  (%0)(sp)" : : "i"(2 * REGBYTES));

    __asm volatile("csrr        t0,  mepc");
    __asm volatile(STORE "      t0,  (%0)(sp)" : : "i"(0 * REGBYTES));
}

__attribute__((naked)) void ContextStackExit(void)
{
    __asm volatile(LOAD "       t0, (%0)(sp)" : : "i"(0 * REGBYTES));
    __asm volatile("csrw        mepc, t0");

    __asm volatile(LOAD "       t0,  (%0)(sp)" : : "i"(2 * REGBYTES));
    __asm volatile("li          t1, %0" : : "i"(MSTATUS_MPP));
    __asm volatile("or          t0, t0, t1");
    __asm volatile("csrw        mstatus, t0");

#ifdef __riscv_flen
    __asm volatile(LOAD "       t0, (%0)(sp)" : : "i"(3 * REGBYTES));
    __asm volatile("csrrw       x0, fcsr, t0");
#endif

    __asm volatile(LOAD "       x1,  (%0)(sp)" : : "i"(1 * REGBYTES));
    __asm volatile(LOAD "       x4,  (%0)(sp)" : : "i"(4 * REGBYTES));
    __asm volatile(LOAD "       x5,  (%0)(sp)" : : "i"(5 * REGBYTES));
    __asm volatile(LOAD "       x6,  (%0)(sp)" : : "i"(6 * REGBYTES));
    __asm volatile(LOAD "       x7,  (%0)(sp)" : : "i"(7 * REGBYTES));
    __asm volatile(LOAD "       x8,  (%0)(sp)" : : "i"(8 * REGBYTES));
    __asm volatile(LOAD "       x9,  (%0)(sp)" : : "i"(9 * REGBYTES));
    __asm volatile(LOAD "       x10, (%0)(sp)" : : "i"(10 * REGBYTES));
    __asm volatile(LOAD "       x11, (%0)(sp)" : : "i"(11 * REGBYTES));
    __asm volatile(LOAD "       x12, (%0)(sp)" : : "i"(12 * REGBYTES));
    __asm volatile(LOAD "       x13, (%0)(sp)" : : "i"(13 * REGBYTES));
    __asm volatile(LOAD "       x14, (%0)(sp)" : : "i"(14 * REGBYTES));
    __asm volatile(LOAD "       x15, (%0)(sp)" : : "i"(15 * REGBYTES));
#ifndef __riscv_32e
    __asm volatile(LOAD "       x16, (%0)(sp)" : : "i"(16 * REGBYTES));
    __asm volatile(LOAD "       x17, (%0)(sp)" : : "i"(17 * REGBYTES));
    __asm volatile(LOAD "       x18, (%0)(sp)" : : "i"(18 * REGBYTES));
    __asm volatile(LOAD "       x19, (%0)(sp)" : : "i"(19 * REGBYTES));
    __asm volatile(LOAD "       x20, (%0)(sp)" : : "i"(20 * REGBYTES));
    __asm volatile(LOAD "       x21, (%0)(sp)" : : "i"(21 * REGBYTES));
    __asm volatile(LOAD "       x22, (%0)(sp)" : : "i"(22 * REGBYTES));
    __asm volatile(LOAD "       x23, (%0)(sp)" : : "i"(23 * REGBYTES));
    __asm volatile(LOAD "       x24, (%0)(sp)" : : "i"(24 * REGBYTES));
    __asm volatile(LOAD "       x25, (%0)(sp)" : : "i"(25 * REGBYTES));
    __asm volatile(LOAD "       x26, (%0)(sp)" : : "i"(26 * REGBYTES));
    __asm volatile(LOAD "       x27, (%0)(sp)" : : "i"(27 * REGBYTES));
    __asm volatile(LOAD "       x28, (%0)(sp)" : : "i"(28 * REGBYTES));
    __asm volatile(LOAD "       x29, (%0)(sp)" : : "i"(29 * REGBYTES));
    __asm volatile(LOAD "       x30, (%0)(sp)" : : "i"(30 * REGBYTES));
    __asm volatile(LOAD "       x31, (%0)(sp)" : : "i"(31 * REGBYTES));

    __asm volatile("addi        sp, sp, (%0)" : : "i"(32 * REGBYTES));
#else
    __asm volatile("addi        sp, sp, (%0)" : : "i"(16 * REGBYTES));
#endif

#ifdef __riscv_flen
    __asm volatile(FPLOAD "     f0,  (%0)(sp)" : : "i"(0 * FPREGBYTES));
    __asm volatile(FPLOAD "     f1,  (%0)(sp)" : : "i"(1 * FPREGBYTES));
    __asm volatile(FPLOAD "     f2,  (%0)(sp)" : : "i"(2 * FPREGBYTES));
    __asm volatile(FPLOAD "     f3,  (%0)(sp)" : : "i"(3 * FPREGBYTES));
    __asm volatile(FPLOAD "     f4,  (%0)(sp)" : : "i"(4 * FPREGBYTES));
    __asm volatile(FPLOAD "     f5,  (%0)(sp)" : : "i"(5 * FPREGBYTES));
    __asm volatile(FPLOAD "     f6,  (%0)(sp)" : : "i"(6 * FPREGBYTES));
    __asm volatile(FPLOAD "     f7,  (%0)(sp)" : : "i"(7 * FPREGBYTES));
    __asm volatile(FPLOAD "     f8,  (%0)(sp)" : : "i"(8 * FPREGBYTES));
    __asm volatile(FPLOAD "     f9,  (%0)(sp)" : : "i"(9 * FPREGBYTES));
    __asm volatile(FPLOAD "     f10, (%0)(sp)" : : "i"(10 * FPREGBYTES));
    __asm volatile(FPLOAD "     f11, (%0)(sp)" : : "i"(11 * FPREGBYTES));
    __asm volatile(FPLOAD "     f12, (%0)(sp)" : : "i"(12 * FPREGBYTES));
    __asm volatile(FPLOAD "     f13, (%0)(sp)" : : "i"(13 * FPREGBYTES));
    __asm volatile(FPLOAD "     f14, (%0)(sp)" : : "i"(14 * FPREGBYTES));
    __asm volatile(FPLOAD "     f15, (%0)(sp)" : : "i"(15 * FPREGBYTES));
    __asm volatile(FPLOAD "     f16, (%0)(sp)" : : "i"(16 * FPREGBYTES));
    __asm volatile(FPLOAD "     f17, (%0)(sp)" : : "i"(17 * FPREGBYTES));
    __asm volatile(FPLOAD "     f18, (%0)(sp)" : : "i"(18 * FPREGBYTES));
    __asm volatile(FPLOAD "     f19, (%0)(sp)" : : "i"(19 * FPREGBYTES));
    __asm volatile(FPLOAD "     f20, (%0)(sp)" : : "i"(20 * FPREGBYTES));
    __asm volatile(FPLOAD "     f21, (%0)(sp)" : : "i"(21 * FPREGBYTES));
    __asm volatile(FPLOAD "     f22, (%0)(sp)" : : "i"(22 * FPREGBYTES));
    __asm volatile(FPLOAD "     f23, (%0)(sp)" : : "i"(23 * FPREGBYTES));
    __asm volatile(FPLOAD "     f24, (%0)(sp)" : : "i"(24 * FPREGBYTES));
    __asm volatile(FPLOAD "     f25, (%0)(sp)" : : "i"(25 * FPREGBYTES));
    __asm volatile(FPLOAD "     f26, (%0)(sp)" : : "i"(26 * FPREGBYTES));
    __asm volatile(FPLOAD "     f27, (%0)(sp)" : : "i"(27 * FPREGBYTES));
    __asm volatile(FPLOAD "     f28, (%0)(sp)" : : "i"(28 * FPREGBYTES));
    __asm volatile(FPLOAD "     f29, (%0)(sp)" : : "i"(29 * FPREGBYTES));
    __asm volatile(FPLOAD "     f30, (%0)(sp)" : : "i"(30 * FPREGBYTES));
    __asm volatile(FPLOAD "     f31, (%0)(sp)" : : "i"(31 * FPREGBYTES));

    __asm volatile("addi        sp, sp, (%0)" : : "i"(32 * FPREGBYTES));
#endif

    __asm volatile("mret" : : : "memory");
}

/* CoreFunction ------------------------------------------------------------ */
inline intptr_t MDS_CoreInterruptCurrent(void)
{
    intptr_t mcause;

    __asm volatile("csrr        %0, mcause" : "=r"(mcause));

    return (mcause);
}

inline MDS_Item_t MDS_CoreInterruptLock(void)
{
    register intptr_t result;

    __asm volatile("csrrci      %0, mstatus, %1" : "=r"(result) : "i"(MSTATUS_MIE));

    return (result);
}

inline void MDS_CoreInterruptRestore(MDS_Item_t lock)
{
    __asm volatile("csrw        mstatus, %0" : : "r"(lock) : "memory");
}

inline void MDS_CoreIdleSleep(void)
{
    __asm volatile("wfi");
}

/* CoreThread -------------------------------------------------------------- */
void *MDS_CoreThreadStackInit(void *stackBase, size_t stackSize, void *entry, void *arg, void *exit)
{
#ifndef __riscv_32e
    uintptr_t sp = VALUE_ALIGN((uintptr_t)(stackBase) + stackSize, sizeof(uint64_t) + sizeof(uint64_t));
#else
    uintptr_t sp = VALUE_ALIGN((uintptr_t)(stackBase) + stackSize, sizeof(uint32_t));
#endif
    struct StackFrame *stack = (struct StackFrame *)(sp - sizeof(struct StackFrame));

    MDS_MemBuffSet(stackBase, '@', stackSize);
    for (size_t idx = 0; idx < (sizeof(struct StackFrame) / sizeof(uint32_t)); idx++) {
        ((uint32_t *)stack)[idx] = 0xDEADBEEF;
    }

    stack->a0 = (uintptr_t)(arg);
    stack->mepc = (uintptr_t)(entry);
    stack->ra = (uintptr_t)(exit);

#ifdef __riscv_flen
    stack->fcsr = 0;
    stack->mstatus = MSTATUS_FS | MSTATUS_MPP | MSTATUS_MPIE;
#else
    stack->mstatus = MSTATUS_MPP | MSTATUS_MPIE;
#endif

    return (stack);
}

bool MDS_CoreThreadStackCheck(MDS_Thread_t *thread)
{
    MDS_ASSERT(thread != NULL);

    if (((*(uint8_t *)(thread->stackBase)) != '@') ||
        ((uintptr_t)(thread->stackPoint) <= (uintptr_t)(thread->stackBase)) ||
        ((uintptr_t)(thread->stackPoint) > ((uintptr_t)(thread->stackBase) + (uintptr_t)(thread->stackSize)))) {
        MDS_LOG_F("[CORE] thread(%p) entry:%p stackbase:%p stacksize:%u overflow", thread, thread->entry,
                  thread->stackPoint, thread->stackBase, thread->stackSize);
        return (false);
    }

    return (true);
}

/* CoreScheduler ----------------------------------------------------------- */
#if (defined(MDS_THREAD_PRIORITY_NUMS) && (MDS_THREAD_PRIORITY_NUMS > 0))
static struct CoreScheduler {
    uintptr_t swflag;
    uintptr_t *fromSP;
    uintptr_t *toSP;
} g_coreScheduler;

__attribute__((weak)) bool MDS_CoreSchedulerTrigSwitch(void)
{
    return (false);
}

__attribute__((naked)) void MDS_CoreSchedulerSwitchHandler(void)
{
    ContextStackSave();

    if (g_coreScheduler.swflag != false) {
        g_coreScheduler.swflag = false;

        __asm volatile(STORE "      sp, (%0)" : : "r"(g_coreScheduler.fromSP));

        __asm volatile(LOAD "       sp, (%0)" : : "r"(g_coreScheduler.toSP));
    }

    __asm volatile("j       %0" : : "i"(ContextStackExit));
}

void Trap_Handler(void);
void MDS_CoreSchedulerStartup(void *toSP)
{
#if defined(__IAR_SYSTEMS_ICC__)
    const uintptr_t __StackTop[] = __section_end(".stack");
#else
    extern void __StackTop();
#endif
    register MDS_Item_t lock = MDS_CoreInterruptLock();

    __asm volatile("csrw    mscratch, %0" : : "r"(__StackTop));
    __asm volatile("csrw    mtvec, %0" : : "r"(Trap_Handler));

    g_coreScheduler.swflag = true;
    g_coreScheduler.fromSP = NULL;
    g_coreScheduler.toSP = toSP;

    MDS_CoreInterruptRestore(lock);

    if (MDS_CoreSchedulerTrigSwitch() == false) {
        g_coreScheduler.swflag = false;

        __asm volatile(LOAD "       sp, (%0)" : : "r"(toSP));

        __asm volatile("j           %0" : : "i"(ContextStackExit));
    }
}

__attribute__((naked)) static void CoreSchedulerSwitchProcess(void *fromSP, void *toSP)
{
    ContextStackSave();

    __asm volatile(STORE "      ra, (%0)(sp)" : : "i"(0 * REGBYTES));

    __asm volatile(STORE "      sp, (%0)" : : "r"(fromSP));

    __asm volatile(LOAD "       sp, (%0)" : : "r"(toSP));

    __asm volatile("j           %0" : : "i"(ContextStackExit));
}

void MDS_CoreSchedulerSwitch(void *fromSP, void *toSP)
{
    if (g_coreScheduler.swflag == false) {
        g_coreScheduler.swflag = true;
        g_coreScheduler.fromSP = fromSP;
    }

    g_coreScheduler.toSP = toSP;

    if ((MDS_CoreSchedulerTrigSwitch() == false) && (MDS_CoreInterruptCurrent() == 0)) {
        g_coreScheduler.swflag = false;

        CoreSchedulerSwitchProcess(fromSP, toSP);
    }
}
#endif

/* Backtrace --------------------------------------------------------------- */
#if (defined(MDS_CORE_BACKTRACE) && (MDS_CORE_BACKTRACE > 0))
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
#endif

/* Exception --------------------------------------------------------------- */
__attribute__((weak)) void MDS_CoreExceptionCallback(void)
{
}

#if (defined(MDS_CORE_BACKTRACE) && (MDS_CORE_BACKTRACE > 0))
static struct StackFrame *g_exceptionContext = NULL;

__attribute__((noreturn)) void Exception_Handler(uintptr_t sp)
{
    register uintptr_t mcause, mepc, mtval;
    __asm volatile("csrr        %0, mcause" : "=r"(mcause));
    __asm volatile("csrr        %0, mepc" : "=r"(mepc));
    __asm volatile("csrr        %0, mtval" : "=r"(mtval));

    MDS_LOG_F("mcause:%d mepc:0x%x mtval:0x%x", mcause, mepc, mtval);

    if (g_exceptionContext == NULL) {
        g_exceptionContext = (struct StackFrame *)sp;
    }

    MDS_CoreExceptionCallback();

    for (;;) {
    }
}

#else
__attribute__((noreturn)) void Exception_Handler(uintptr_t sp)
{
    (void)(sp);

    MDS_CoreExceptionCallback();

    for (;;) {
    }
}
#endif

/* Trap -------------------------------------------------------------------- */
#if (defined(MDS_THREAD_PRIORITY_NUMS) && (MDS_THREAD_PRIORITY_NUMS > 0))
__attribute__((weak)) void Interrupt_Handler(uintptr_t cause)
{
    UNUSED(cause);
}

__attribute__((naked, __aligned__(0x04))) void Trap_Handler(void)
{
    ContextStackSave();

    register intptr_t mcause, mscratch;
    __asm volatile("csrr        %0, mcause" : "=r"(mcause));
    __asm volatile("csrr        %0, mscratch" : "=r"(mscratch));
    if (mscratch != 0) {
        __asm volatile("csrrw       sp, mscratch, sp");
    }

    MDS_HOOK_CALL(INTERRUPT_ENTER, mcause);
    if ((mcause & MCAUSE_INT) == 0U) {
        Exception_Handler(mscratch);
    } else {
        Interrupt_Handler(mcause & MCAUSE_CAUSE);
    }
    MDS_HOOK_CALL(INTERRUPT_EXIT, mcause);

    if (mscratch != 0) {
        __asm volatile("csrrw       sp, mscratch, sp");
    }

    if (g_coreScheduler.swflag) {
        g_coreScheduler.swflag = false;

        __asm volatile(STORE "      sp, (%0)" : : "r"(g_coreScheduler.fromSP));

        __asm volatile(LOAD "       sp, (%0)" : : "r"(g_coreScheduler.toSP));
    }

    __asm volatile("j           %0" : : "i"(ContextStackExit));
}
#endif
