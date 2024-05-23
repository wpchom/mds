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
#include "dev_gpio.h"

/* GPIO module ------------------------------------------------------------- */
MDS_Err_t DEV_GPIO_ModuleInit(DEV_GPIO_Module_t *gpio, const char *name, const DEV_GPIO_Driver_t *driver,
                              MDS_DevHandle_t *handle, const MDS_Arg_t *init)
{
    return (MDS_DevModuleInit((MDS_DevModule_t *)gpio, name, (const MDS_DevDriver_t *)driver, handle, init));
}

MDS_Err_t DEV_GPIO_ModuleDeInit(DEV_GPIO_Module_t *gpio)
{
    return (MDS_DevModuleDeInit((MDS_DevModule_t *)gpio));
}

DEV_GPIO_Module_t *DEV_GPIO_ModuleCreate(const char *name, const DEV_GPIO_Driver_t *driver, const MDS_Arg_t *init)
{
    return ((DEV_GPIO_Module_t *)MDS_DevModuleCreate(sizeof(DEV_GPIO_Module_t), name, (const MDS_DevDriver_t *)driver,
                                                     init));
}

MDS_Err_t DEV_GPIO_ModuleDestroy(DEV_GPIO_Module_t *gpio)
{
    return (MDS_DevModuleDestroy((MDS_DevModule_t *)gpio));
}

/* GPIO pin ---------------------------------------------------------------- */
MDS_Err_t DEV_GPIO_PinInit(DEV_GPIO_Pin_t *pin, const char *name, DEV_GPIO_Module_t *gpio)
{
    return (MDS_DevPeriphInit((MDS_DevPeriph_t *)pin, name, (MDS_DevAdaptr_t *)gpio));
}

MDS_Err_t DEV_GPIO_PinDeInit(DEV_GPIO_Pin_t *pin)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)pin));
}

DEV_GPIO_Pin_t *DEV_GPIO_PinCreate(const char *name, DEV_GPIO_Module_t *gpio)
{
    return ((DEV_GPIO_Pin_t *)MDS_DevPeriphCreate(sizeof(DEV_GPIO_Pin_t), name, (MDS_DevAdaptr_t *)gpio));
}

MDS_Err_t DEV_GPIO_PinDestroy(DEV_GPIO_Pin_t *pin)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)pin));
}

MDS_Err_t DEV_GPIO_PinConfig(DEV_GPIO_Pin_t *pin, const DEV_GPIO_Config_t *config)
{
    MDS_ASSERT(pin != NULL);
    MDS_ASSERT(pin->mount != NULL);
    MDS_ASSERT(pin->mount->driver != NULL);
    MDS_ASSERT(pin->mount->driver->config != NULL);

    MDS_Err_t err = pin->mount->driver->config(pin, config);
    if (err == MDS_EOK) {
        pin->config = *config;
    }

    return (err);
}

MDS_Err_t DEV_GPIO_PinExtiConfig(DEV_GPIO_Pin_t *pin, DEV_GPIO_Exti_t exti)
{
    MDS_ASSERT(pin != NULL);
    MDS_ASSERT(pin->mount != NULL);
    MDS_ASSERT(pin->mount->driver != NULL);
    MDS_ASSERT(pin->mount->driver->exti != NULL);

    return (pin->mount->driver->exti(pin, exti));
}

void DEV_GPIO_PinExtiCallback(DEV_GPIO_Pin_t *pin, void (*callback)(const DEV_GPIO_Pin_t *, MDS_Arg_t *),
                              MDS_Arg_t *arg)
{
    MDS_ASSERT(pin != NULL);

    pin->object.parent = (MDS_Arg_t *)pin;
    pin->extiCallback = callback;
    pin->arg = arg;
}

MDS_Err_t DEV_GPIO_PinToggle(DEV_GPIO_Pin_t *pin)
{
    MDS_ASSERT(pin != NULL);
    MDS_ASSERT(pin->mount != NULL);
    MDS_ASSERT(pin->mount->driver != NULL);
    MDS_ASSERT(pin->mount->driver->control != NULL);

    return (pin->mount->driver->control(pin->mount, DEV_GPIO_CMD_TOGGLE, (MDS_Arg_t *)pin));
}

MDS_Mask_t DEV_GPIO_PinRead(const DEV_GPIO_Pin_t *pin)
{
    MDS_ASSERT(pin != NULL);
    MDS_ASSERT(pin->mount != NULL);
    MDS_ASSERT(pin->mount->driver != NULL);
    MDS_ASSERT(pin->mount->driver->read != NULL);

    return (pin->mount->driver->read(pin));
}

MDS_Err_t DEV_GPIO_PinWrite(DEV_GPIO_Pin_t *pin, MDS_Mask_t val)
{
    MDS_ASSERT(pin != NULL);
    MDS_ASSERT(pin->mount != NULL);
    MDS_ASSERT(pin->mount->driver != NULL);
    MDS_ASSERT(pin->mount->driver->write != NULL);

    return (pin->mount->driver->write(pin, val));
}

MDS_Err_t DEV_GPIO_PinLow(DEV_GPIO_Pin_t *pin)
{
    return (DEV_GPIO_PinWrite(pin, (MDS_Mask_t)(0)));
}

MDS_Err_t DEV_GPIO_PinHigh(DEV_GPIO_Pin_t *pin)
{
    return (DEV_GPIO_PinWrite(pin, (MDS_Mask_t)(-1)));
}

MDS_Err_t DEV_GPIO_PinActive(DEV_GPIO_Pin_t *pin, bool actived)
{
    return (DEV_GPIO_PinWrite(pin, (actived) ? (~(pin->config.initVal)) : (pin->config.initVal)));
}

bool DEV_GPIO_PinIsActived(const DEV_GPIO_Pin_t *pin)
{
    return (DEV_GPIO_PinRead(pin) != pin->config.initVal);
}
