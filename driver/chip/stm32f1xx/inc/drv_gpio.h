/**
 * @copyright   Copyright (c) 2024 Pchom & licensed under Mulan PSL v2
 * @file        drv_gpio.h
 * @brief       stm32f1xx gpio driver for mds device
 * @date        2024-05-30
 */
#ifndef __DRV_GPIO_H__
#define __DRV_GPIO_H__

/* Include ----------------------------------------------------------------- */
#include "dev_gpio.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_exti.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DRV_GPIO_PinConfig(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin, const DEV_GPIO_Config_t *config);

extern uint32_t DRV_GPIO_PortReadInput(GPIO_TypeDef *GPIOx);
extern uint32_t DRV_GPIO_PortReadOutput(GPIO_TypeDef *GPIOx);
extern void DRV_GPIO_PortWrite(GPIO_TypeDef *GPIOx, uint32_t val);
extern void DRV_GPIO_PinHigh(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin);
extern void DRV_GPIO_PinLow(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin);
extern void DRV_GPIO_PinToggle(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin);

extern void DRV_GPIO_PinIRQHandler(DEV_GPIO_Object_t *object);

/* Driver ------------------------------------------------------------------ */
extern const DEV_GPIO_Driver_t G_DRV_STM32F1XX_GPIO;

#ifdef __cplusplus
}
#endif

#endif /* __DRV_GPIO_H__ */
