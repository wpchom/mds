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
#include "dev_led.h"

/* LED device -------------------------------------------------------------- */
MDS_Err_t DEV_LED_DeviceInit(DEV_LED_Device_t *led, const char *name, const DEV_LED_Driver_t *driver,
                             MDS_DevHandle_t *handle, const MDS_Arg_t *init)
{
    return (MDS_DevModuleInit((MDS_DevModule_t *)led, name, (const MDS_DevDriver_t *)driver, handle, init));
}

MDS_Err_t DEV_LED_DeviceDeInit(DEV_LED_Device_t *led)
{
    return (MDS_DevModuleDeInit((MDS_DevModule_t *)led));
}

DEV_LED_Device_t *DEV_LED_DeviceCreate(const char *name, const DEV_LED_Driver_t *driver, const MDS_Arg_t *init)
{
    return ((DEV_LED_Device_t *)MDS_DevModuleCreate(sizeof(DEV_LED_Device_t), name, (const MDS_DevDriver_t *)driver,
                                                    init));
}

MDS_Err_t DEV_LED_DeviceDestroy(DEV_LED_Device_t *led)
{
    return (MDS_DevModuleDestroy((MDS_DevModule_t *)led));
}

MDS_Err_t DEV_LED_DeviceLight(DEV_LED_Device_t *led, const DEV_LED_Color_t *color, const DEV_LED_Light_t *light)
{
    MDS_ASSERT(led != NULL);
    MDS_ASSERT(led->driver != NULL);
    MDS_ASSERT(led->driver->light != NULL);

    MDS_Err_t err = led->driver->light(led, color, light);
    if (err == MDS_EOK) {
        led->object.light = *light;
        led->object.colorEnum = DEV_LED_COLOR_NUMS;
    }

    return (err);
}

MDS_Err_t DEV_LED_DeviceColor(DEV_LED_Device_t *led, DEV_LED_ColorEnum_t colorEnum, const DEV_LED_Light_t *light)
{
    MDS_ASSERT(led != NULL);
    MDS_ASSERT(led->driver != NULL);
    MDS_ASSERT(led->driver->light != NULL);
    MDS_ASSERT(colorEnum < DEV_LED_COLOR_NUMS);

    MDS_Err_t err = led->driver->light(led, &(led->object.color[colorEnum]), light);
    if (err == MDS_EOK) {
        led->object.light = *light;
        led->object.colorEnum = colorEnum;
    }

    return (err);
}
