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
#ifndef __DEV_DISPLAY_H__
#define __DEV_DISPLAY_H__

/* Include ----------------------------------------------------------------- */
#include "mds/dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
enum DEV_DISPLAY_Cmd {
    DEV_DISPLAY_CMD_BRIGHT_SET = MDS_DEVICE_CMD_DRIVER,
};

typedef struct DEV_DISPLAY_Bright {
    uint8_t level;
} DEV_DISPLAY_Bright_t;

typedef struct DEV_DISPLAY_Pixel {
    int32_t x;
    int32_t y;
} DEV_DISPLAY_Pixel_t;

typedef struct DEV_DISPLAY_Area {
    DEV_DISPLAY_Pixel_t tl;
    DEV_DISPLAY_Pixel_t br;
} DEV_DISPLAY_Area_t;

typedef struct DEV_DISPLAY_Device DEV_DISPLAY_Device_t;

typedef struct DEV_DISPLAY_Driver {
    MDS_Err_t (*control)(const DEV_DISPLAY_Device_t *display, MDS_DevCmd_t cmd, MDS_Arg_t arg);
    MDS_Err_t (*flush)(const DEV_DISPLAY_Device_t *display, const DEV_DISPLAY_Area_t *area,
                       uint8_t *map);
} DEV_DISPLAY_Driver_t;

struct DEV_DISPLAY_Device {
    const MDS_Device_t device;
    const MDS_DevHandle_t handle;
    const DEV_DISPLAY_Driver_t *driver;
};

/* Function ---------------------------------------------------------------- */
MDS_Err_t DEV_DISPLAY_DeviceInit(DEV_DISPLAY_Device_t *display, const char *name,
                                 const DEV_DISPLAY_Driver_t *driver, MDS_DevHandle_t handle,
                                 MDS_Arg_t init);
MDS_Err_t DEV_DISPLAY_DeviceDeInit(DEV_DISPLAY_Device_t *display);
DEV_DISPLAY_Device_t *DEV_DISPLAY_DeviceCreate(const char *name, const DEV_DISPLAY_Driver_t *driver,
                                               MDS_Arg_t init);
MDS_Err_t DEV_DISPLAY_DeviceDestroy(DEV_DISPLAY_Device_t *display);

MDS_Err_t DEV_DISPLAY_DeviceBright(DEV_DISPLAY_Device_t *display, DEV_DISPLAY_Bright_t bright);
MDS_Err_t DEV_DISPLAY_DeviceFlush(DEV_DISPLAY_Device_t *display, const DEV_DISPLAY_Area_t *area,
                                  uint8_t *map);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_DISPLAY_H__ */
