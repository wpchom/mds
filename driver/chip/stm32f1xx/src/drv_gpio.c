/**
 * @copyright   Copyright (c) 2024 Pchom & licensed under Mulan PSL v2
 * @file        drv_gpio.c
 * @brief       stm32f1xx gpio driver for mds device
 * @date        2024-05-30
 */
/* Include ----------------------------------------------------------------- */
#include "drv_gpio.h"

/* Function ---------------------------------------------------------------- */
static void DRV_GPIO_PinWrite(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin, uint32_t val)
{
    uint32_t shift = __CLZ(__RBIT(GPIO_Pin));
    if (shift < GPIO_BSRR_BR0_Pos) {
        uint32_t set = (val << shift) & GPIO_Pin;  // 00(101)11 & 00(111)00 => 00(101)00
        uint32_t clr = (~set) & GPIO_Pin;          // 11(010)00 & 00(111)00 => 00(010)00
        WRITE_REG(GPIOx->BSRR, clr << GPIO_BSRR_BR0_Pos | set);
    }
}

MDS_Err_t DRV_GPIO_PinConfig(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin, const DEV_GPIO_Config_t *config)
{
    MDS_ASSERT(config != NULL);

    GPIO_InitTypeDef init = {0};

    init.Pin = GPIO_Pin;

    if (config->mode == DEV_GPIO_MODE_INPUT) {
        init.Mode = GPIO_MODE_INPUT;
    } else if (config->mode == DEV_GPIO_MODE_OUTPUT) {
        init.Mode = GPIO_MODE_OUTPUT_PP;
    } else if (config->mode == DEV_GPIO_MODE_ALTERNATE) {
        init.Mode = GPIO_MODE_AF_PP;
    } else {  // DEV_GPIO_MODE_ANALOG
        init.Mode = GPIO_MODE_ANALOG;
    }

    if (config->type == DEV_GPIO_TYPE_OD) {
        if ((config->mode == DEV_GPIO_MODE_OUTPUT) || (config->mode == DEV_GPIO_MODE_ALTERNATE)) {
            init.Mode |= (GPIO_MODE_OUTPUT_OD & GPIO_MODE_AF_OD);
        }
    } else if (config->type == DEV_GPIO_TYPE_PP_UP) {
        init.Pull = GPIO_PULLUP;
    } else if (config->type == DEV_GPIO_TYPE_PP_DOWN) {
        init.Pull = GPIO_PULLDOWN;
    } else {  // DEV_GPIO_TYPE_PP_NO
        init.Pull = GPIO_NOPULL;
    }

    if ((config->intr & DEV_GPIO_INTR_RISING) != 0) {
        init.Mode |= GPIO_MODE_IT_RISING;
    }
    if ((config->intr & DEV_GPIO_INTR_FALLING) != 0) {
        init.Mode |= GPIO_MODE_IT_FALLING;
    }

    HAL_GPIO_Init(GPIOx, &init);

    return (MDS_EOK);
}

uint32_t DRV_GPIO_PortReadInput(GPIO_TypeDef *GPIOx)
{
    return (LL_GPIO_ReadInputPort(GPIOx));
}

uint32_t DRV_GPIO_PortReadOutput(GPIO_TypeDef *GPIOx)
{
    return (LL_GPIO_ReadOutputPort(GPIOx));
}

void DRV_GPIO_PortWrite(GPIO_TypeDef *GPIOx, uint32_t val)
{
    LL_GPIO_WriteOutputPort(GPIOx, val);
}

void DRV_GPIO_PinHigh(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin)
{
    LL_GPIO_SetOutputPin(GPIOx, GPIO_Pin);
}

void DRV_GPIO_PinLow(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin)
{
    LL_GPIO_ResetOutputPin(GPIOx, GPIO_Pin);
}

void DRV_GPIO_PinToggle(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin)
{
    LL_GPIO_TogglePin(GPIOx, GPIO_Pin);
}

void DRV_GPIO_PinIRQHandler(DEV_GPIO_Object_t *object)
{
    if (LL_EXTI_ReadFlag_0_31(object->pinMask) != 0x00U) {
        LL_EXTI_ClearFlag_0_31(object->pinMask);

        DEV_GPIO_Pin_t *pin = (DEV_GPIO_Pin_t *)(object->parent);
        if ((pin != NULL) && (pin->callback != NULL)) {
            pin->callback(pin, pin->arg);
        }
    }
}

/* Driver ------------------------------------------------------------------ */
static MDS_Err_t DDRV_GPIO_PortControl(const DEV_GPIO_Module_t *gpio, MDS_Item_t cmd, MDS_Arg_t *arg)
{
    MDS_ASSERT(gpio != NULL);

    switch (cmd) {
        case MDS_DEVICE_CMD_INIT:
        case MDS_DEVICE_CMD_DEINIT:
        case MDS_DEVICE_CMD_HANDLESZ:
            return (MDS_EOK);
    }

    if (cmd == DEV_GPIO_CMD_PIN_TOGGLE) {
        DEV_GPIO_Pin_t *pin = (DEV_GPIO_Pin_t *)arg;
        DRV_GPIO_PinToggle(pin->object.GPIOx, pin->object.pinMask);
        return (MDS_EOK);
    }

    return (MDS_EACCES);
}

static MDS_Err_t DDRV_GPIO_PinConfig(const DEV_GPIO_Pin_t *pin, const DEV_GPIO_Config_t *config)
{
    GPIO_TypeDef *GPIOx = (GPIO_TypeDef *)(pin->object.GPIOx);

    if (config->mode == DEV_GPIO_MODE_OUTPUT) {
        DRV_GPIO_PinWrite(GPIOx, pin->object.pinMask, pin->object.initVal);
    }

    return (DRV_GPIO_PinConfig(GPIOx, pin->object.pinMask, config));
}

static MDS_Mask_t DDRV_GPIO_PinRead(const DEV_GPIO_Pin_t *pin)
{
    GPIO_TypeDef *GPIOx = (GPIO_TypeDef *)(pin->object.GPIOx);

    MDS_Mask_t read = DRV_GPIO_PortReadInput(GPIOx);

    return (read >> __CLZ(__RBIT(pin->object.pinMask)));
}

static void DDRV_GPIO_PinWrite(const DEV_GPIO_Pin_t *pin, MDS_Mask_t val)
{
    GPIO_TypeDef *GPIOx = (GPIO_TypeDef *)(pin->object.GPIOx);

    DRV_GPIO_PinWrite(GPIOx, pin->object.pinMask, val);
}

const DEV_GPIO_Driver_t G_DRV_STM32F1XX_GPIO = {
    .control = DDRV_GPIO_PortControl,
    .config = DDRV_GPIO_PinConfig,
    .read = DDRV_GPIO_PinRead,
    .write = DDRV_GPIO_PinWrite,
};
