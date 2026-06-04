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
#include "mds/device/display.h"

/* DISPLAY device ---------------------------------------------------------- */
MDS_Err_t DEV_DISPLAY_DeviceInit(DEV_DISPLAY_Device_t *display, const char *name,
                                 const DEV_DISPLAY_Driver_t *driver, MDS_DevHandle_t handle,
                                 MDS_Arg_t init)
{
    return (MDS_DevModuleInit((MDS_DevModule_t *)display, name, (const MDS_DevDriver_t *)driver,
                              handle, init));
}

MDS_Err_t DEV_DISPLAY_DeviceDeInit(DEV_DISPLAY_Device_t *display)
{
    return (MDS_DevModuleDeInit((MDS_DevModule_t *)display));
}

DEV_DISPLAY_Device_t *DEV_DISPLAY_DeviceCreate(const char *name, const DEV_DISPLAY_Driver_t *driver,
                                               MDS_Arg_t init)
{
    return ((DEV_DISPLAY_Device_t *)MDS_DevModuleCreate(sizeof(DEV_DISPLAY_Device_t), name,
                                                        (const MDS_DevDriver_t *)driver, init));
}

MDS_Err_t DEV_DISPLAY_DeviceDestroy(DEV_DISPLAY_Device_t *display)
{
    return (MDS_DevModuleDestroy((MDS_DevModule_t *)display));
}

MDS_Err_t DEV_DISPLAY_DeviceBright(DEV_DISPLAY_Device_t *display, DEV_DISPLAY_Bright_t bright)
{
    MDS_ASSERT(display != NULL);
    MDS_ASSERT(display->driver != NULL);
    MDS_ASSERT(display->driver->control != NULL);

    return (display->driver->control(display, DEV_DISPLAY_CMD_BRIGHT_SET, MDS_ARG_WITH(&bright)));
}

MDS_Err_t DEV_DISPLAY_DeviceFlush(DEV_DISPLAY_Device_t *display, const DEV_DISPLAY_Area_t *area,
                                  uint8_t *map)
{
    MDS_ASSERT(display != NULL);
    MDS_ASSERT(display->driver != NULL);
    MDS_ASSERT(display->driver->flush != NULL);

    MDS_Err_t err = display->driver->flush(display, area, map);

    return (err);
}
