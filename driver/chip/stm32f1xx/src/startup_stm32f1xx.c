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
#include "startup/stm32f100xb.c"
#elif defined(STM32F100xE)
#include "startup/stm32f100xe.c"
#elif defined(STM32F101x6)
#include "startup/stm32f101x6.c"
#elif defined(STM32F101xB)
#include "startup/stm32f101xb.c"
#elif defined(STM32F101xE)
#include "startup/stm32f101xe.c"
#elif defined(STM32F101xG)
#include "startup/stm32f101xg.c"
#elif defined(STM32F102x6)
#include "startup/stm32f102x6.c"
#elif defined(STM32F102xB)
#include "startup/stm32f102xb.c"
#elif defined(STM32F103x6)
#include "startup/stm32f103x6.c"
#elif defined(STM32F103xB)
#include "startup/stm32f103xb.c"
#elif defined(STM32F103xE)
#include "startup/stm32f103xe.c"
#elif defined(STM32F103xG)
#include "startup/stm32f103xg.c"
#elif defined(STM32F105xC)
#include "startup/stm32f105xc.c"
#elif defined(STM32F107xC)
#include "startup/stm32f107xc.c"
#else
#error "Please select first the target STM32F1xx device used in your application (in stm32f1xx.h file)"
#endif

/**
 * @}
 */

/* Function ---------------------------------------------------------------- */
__attribute__((naked, noreturn)) void _start(void)
{
    __asm volatile("bl     main");
    __asm volatile("b      .");
}

__attribute__((naked, noreturn)) void _exit(int res)
{
    (void)(res);

    __asm volatile("b      .");
}

void VectorInit(uintptr_t vectorAddress)
{
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;

    for (uint32_t idx = 0; idx < (sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0])); idx++) {
        NVIC->ICER[idx] = 0xFFFFFFFF;
    }

    SCB->VTOR = vectorAddress;
}

__attribute__((naked, noreturn)) void Reset_Handler(void)
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
