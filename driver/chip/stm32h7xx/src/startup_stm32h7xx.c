/** @addtogroup Startup_Include
 * @{
 */

#if defined(STM32H743xx)
#include "startup/stm32h743xx.in"
#elif defined(STM32H753xx)
#include "startup/stm32h753xx.in"
#elif defined(STM32H750xx)
#include "startup/stm32h750xx.in"
#elif defined(STM32H742xx)
#include "startup/stm32h742xx.in"
#elif defined(STM32H745xx)
#include "startup/stm32h745xx.in"
#elif defined(STM32H745xG)
#include "startup/stm32h745xg.in"
#elif defined(STM32H755xx)
#include "startup/stm32h755xx.in"
#elif defined(STM32H747xx)
#include "startup/stm32h747xx.in"
#elif defined(STM32H747xG)
#include "startup/stm32h747xg.in"
#elif defined(STM32H757xx)
#include "startup/stm32h757xx.in"
#elif defined(STM32H7B0xx)
#include "startup/stm32h7b0xx.in"
#elif defined(STM32H7B0xxQ)
#include "startup/stm32h7b0xxq.in"
#elif defined(STM32H7A3xx)
#include "startup/stm32h7a3xx.in"
#elif defined(STM32H7B3xx)
#include "startup/stm32h7b3xx.in"
#elif defined(STM32H7A3xxQ)
#include "startup/stm32h7a3xxq.in"
#elif defined(STM32H7B3xxQ)
#include "startup/stm32h7b3xxq.in"
#elif defined(STM32H735xx)
#include "startup/stm32h735xx.in"
#elif defined(STM32H733xx)
#include "startup/stm32h733xx.in"
#elif defined(STM32H730xx)
#include "startup/stm32h730xx.in"
#elif defined(STM32H730xxQ)
#include "startup/stm32h730xxq.in"
#elif defined(STM32H725xx)
#include "startup/stm32h725xx.in"
#elif defined(STM32H723xx)
#include "startup/stm32h723xx.in"
#else
#error "Please select first the target STM32H7xx device used in your application (in stm32h7xx.h file)"
#endif

/**
 * @}
 */

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

