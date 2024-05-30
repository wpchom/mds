/**
 * @copyright   Copyright (c) 2024 Pchom & licensed under Mulan PSL v2
 * @file        startup_stm32f1xx.c
 * @brief       stm32f1xx startup source
 * @date        2024-05-30
 */

/* Include ----------------------------------------------------------------- */
/** @addtogroup Startup_Include
 * @{
 */

#if defined(STM32F100xB)
#include "startup/stm32f100xb.in"
#elif defined(STM32F100xE)
#include "startup/stm32f100xe.in"
#elif defined(STM32F101x6)
#include "startup/stm32f101x6.in"
#elif defined(STM32F101xB)
#include "startup/stm32f101xb.in"
#elif defined(STM32F101xE)
#include "startup/stm32f101xe.in"
#elif defined(STM32F101xG)
#include "startup/stm32f101xg.in"
#elif defined(STM32F102x6)
#include "startup/stm32f102x6.in"
#elif defined(STM32F102xB)
#include "startup/stm32f102xb.in"
#elif defined(STM32F103x6)
#include "startup/stm32f103x6.in"
#elif defined(STM32F103xB)
#include "startup/stm32f103xb.in"
#elif defined(STM32F103xE)
#include "startup/stm32f103xe.in"
#elif defined(STM32F103xG)
#include "startup/stm32f103xg.in"
#elif defined(STM32F105xC)
#include "startup/stm32f105xc.in"
#elif defined(STM32F107xC)
#include "startup/stm32f107xc.in"
#else
#error "Please select first the target STM32F1xx device used in your application (in stm32f1xx.h file)"
#endif

/**
 * @}
 */

/* Function ---------------------------------------------------------------- */
__attribute__((weak, naked, noreturn)) void _start(void)
{
    __asm volatile("bl     main");
    __asm volatile("b      .");
}

__attribute__((weak, naked, noreturn)) void _exit(int res)
{
    (void)(res);

    __asm volatile("b      .");
}

__attribute__((weak)) void VectorInit(uintptr_t vectorAddress)
{
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;

    for (uint32_t idx = 0; idx < (sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0])); idx++) {
        NVIC->ICER[idx] = 0xFFFFFFFF;
    }

    SCB->VTOR = vectorAddress;
}

__attribute__((noreturn)) void Reset_Handler(void)
{
    __disable_irq();

    __set_MSP((uint32_t)__INITIAL_SP);

    SystemInit();

    VectorInit((uintptr_t)__VECTOR_TABLE);

    __enable_irq();

    __PROGRAM_START();
}

__attribute__((noreturn)) void BOOT_JumpIntoVectorTable(uintptr_t vectorAddress)
{
    typedef void (*vector_t)(void);
    vector_t *vectorTable = (vector_t *)(vectorAddress);

    __disable_irq();

    __set_MSP((uint32_t)(vectorTable[0]));

    VectorInit(vectorAddress);

    __enable_irq();

    vectorTable[1]();

    for (;;) {
    }
}

__attribute__((__noreturn__)) void BOOT_JumpIntoDFU(void)
{
    BOOT_JumpIntoVectorTable(0x1FFF0000);
}

__attribute__((__noreturn__)) void BOOT_SystemReset(void)
{
    NVIC_SystemReset();
}

void MDS_CoreInterruptRequestEnable(intptr_t irq)
{
    NVIC_EnableIRQ(irq);
}

void MDS_CoreInterruptRequestDisable(intptr_t irq)
{
    NVIC_DisableIRQ(irq);
}

void MDS_CoreInterruptRequestClearPending(intptr_t irq)
{
    NVIC_ClearPendingIRQ(irq);
}

void MDS_CoreInterruptRequestPriority(intptr_t irq, uintptr_t priority)
{
    NVIC_SetPriority(irq, priority);
}
