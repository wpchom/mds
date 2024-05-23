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
#ifndef __DEV_LED_H__
#define __DEV_LED_H__

/* Include ----------------------------------------------------------------- */
#include "mds_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef uint8_t DEV_LED_Bright_t;

typedef enum DEV_LED_ColorEnum {
    DEV_LED_COLOR_NONE = 0x00U,
    DEV_LED_COLOR_RED = 0x01U,
    DEV_LED_COLOR_GREEN = 0x02U,
    DEV_LED_COLOR_YELLOW = DEV_LED_COLOR_RED | DEV_LED_COLOR_GREEN,
    DEV_LED_COLOR_BLUE = 0x04U,
    DEV_LED_COLOR_PINK = DEV_LED_COLOR_RED | DEV_LED_COLOR_BLUE,
    DEV_LED_COLOR_CYAN = DEV_LED_COLOR_GREEN | DEV_LED_COLOR_BLUE,
    DEV_LED_COLOR_WHITE = DEV_LED_COLOR_RED | DEV_LED_COLOR_GREEN | DEV_LED_COLOR_BLUE,

    DEV_LED_COLOR_NUMS,
} DEV_LED_ColorEnum_t;

typedef struct DEV_LED_Light {
    uint16_t fullOn;
    uint16_t fullOff;
    uint16_t fadeOn;
    uint16_t fadeOff;
    DEV_LED_Bright_t levelOn;
    DEV_LED_Bright_t levelOff;
} DEV_LED_Light_t;

typedef struct DEV_LED_Color {
    DEV_LED_Bright_t bright[DEV_LED_COLOR_NUMS];
} DEV_LED_Color_t;

typedef struct DEV_LED_Config {
    DEV_LED_Color_t color[DEV_LED_COLOR_NUMS];
    DEV_LED_Light_t light;
    DEV_LED_ColorEnum_t colorEnum;
} DEV_LED_Config_t;

typedef struct DEV_LED_Device DEV_LED_Device_t;

typedef struct DEV_LED_Driver {
    MDS_Err_t (*control)(const DEV_LED_Device_t *led, MDS_Item_t cmd, MDS_Arg_t *arg);
    MDS_Err_t (*light)(const DEV_LED_Device_t *led, const DEV_LED_Color_t *color, const DEV_LED_Light_t *light);
} DEV_LED_Driver_t;

struct DEV_LED_Device {
    const MDS_Device_t device;
    const DEV_LED_Driver_t *driver;
    const MDS_DevHandle_t *handle;

    DEV_LED_Config_t config;
};

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DEV_LED_DeviceInit(DEV_LED_Device_t *led, const char *name, const DEV_LED_Driver_t *driver,
                                    MDS_DevHandle_t *handle, const MDS_Arg_t *init);
extern MDS_Err_t DEV_LED_DeviceDeInit(DEV_LED_Device_t *led);
extern DEV_LED_Device_t *DEV_LED_DeviceCreate(const char *name, const DEV_LED_Driver_t *driver, const MDS_Arg_t *init);
extern MDS_Err_t DEV_LED_DeviceDestroy(DEV_LED_Device_t *led);

extern MDS_Err_t DEV_LED_DeviceLight(DEV_LED_Device_t *led, const DEV_LED_Color_t *color, const DEV_LED_Light_t *light);
extern MDS_Err_t DEV_LED_DeviceColor(DEV_LED_Device_t *led, DEV_LED_ColorEnum_t colorEnum,
                                     const DEV_LED_Light_t *light);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_LED_H__ */
