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
#ifndef __DEV_GPIO_H__
#define __DEV_GPIO_H__

/* Include ----------------------------------------------------------------- */
#include "mds_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef enum DEV_GPIO_Level {
    DEV_GPIO_LEVEL_LOW = 0,
    DEV_GPIO_LEVEL_HIGH = !DEV_GPIO_LEVEL_LOW,
} DEV_GPIO_Level_t;

typedef enum DEV_GPIO_Mode {
    DEV_GPIO_MODE_INPUT,
    DEV_GPIO_MODE_OUTPUT,
    DEV_GPIO_MODE_ANALOG,
    DEV_GPIO_MODE_ALTERNATE,
} DEV_GPIO_Mode_t;

typedef enum DEV_GPIO_Type {
    DEV_GPIO_TYPE_PP_NO,
    DEV_GPIO_TYPE_PP_UP,
    DEV_GPIO_TYPE_PP_DOWN,
    DEV_GPIO_TYPE_OD,
} DEV_GPIO_Type_t;

typedef enum DEV_GPIO_Interrupt {
    DEV_GPIO_INT_DISABLE = 0x00U,
    DEV_GPIO_INT_FALLING = 0x01U,
    DEV_GPIO_INT_RISING = 0x02U,
    DEV_GPIO_INT_BOTH = 0x03U,
} DEV_GPIO_Interrupt_t;

typedef struct DEV_GPIO_Config {
    MDS_Mask_t initVal;
    uint8_t alternate;
    DEV_GPIO_Interrupt_t interrupt : 8;
    DEV_GPIO_Mode_t mode           : 8;
    DEV_GPIO_Type_t type           : 8;
} DEV_GPIO_Config_t;

enum DEV_GPIO_Cmd {
    DEV_GPIO_CMD_PIN_TOGGLE = MDS_DEVICE_CMD_DRIVER,
};

typedef struct DEV_GPIO_Module DEV_GPIO_Module_t;
typedef struct DEV_GPIO_Pin DEV_GPIO_Pin_t;

typedef struct DEV_GPIO_Driver {
    MDS_Err_t (*control)(const DEV_GPIO_Module_t *gpio, MDS_Item_t cmd, MDS_Arg_t *arg);
    MDS_Err_t (*config)(const DEV_GPIO_Pin_t *pin, const DEV_GPIO_Config_t *config);
    MDS_Mask_t (*read)(const DEV_GPIO_Pin_t *pin);
    void (*write)(const DEV_GPIO_Pin_t *pin, MDS_Mask_t val);
} DEV_GPIO_Driver_t;

struct DEV_GPIO_Module {
    const MDS_Device_t device;
    const DEV_GPIO_Driver_t *driver;
    const MDS_DevHandle_t *handle;
};

typedef struct DEV_GPIO_Object {
    void *GPIOx;
    MDS_Mask_t pinMask;

    MDS_Arg_t *parent;
} DEV_GPIO_Object_t;

struct DEV_GPIO_Pin {
    const MDS_Device_t device;
    const DEV_GPIO_Module_t *mount;

    DEV_GPIO_Config_t config;
    DEV_GPIO_Object_t object;

    void (*extiCallback)(const DEV_GPIO_Pin_t *pin, MDS_Arg_t *arg);
    MDS_Arg_t *arg;
};

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DEV_GPIO_ModuleInit(DEV_GPIO_Module_t *gpio, const char *name, const DEV_GPIO_Driver_t *driver,
                                     MDS_DevHandle_t *handle, const MDS_Arg_t *init);
extern MDS_Err_t DEV_GPIO_ModuleDeInit(DEV_GPIO_Module_t *gpio);
extern DEV_GPIO_Module_t *DEV_GPIO_ModuleCreate(const char *name, const DEV_GPIO_Driver_t *driver,
                                                const MDS_Arg_t *init);
extern MDS_Err_t DEV_GPIO_ModuleDestroy(DEV_GPIO_Module_t *gpio);

extern MDS_Err_t DEV_GPIO_PinInit(DEV_GPIO_Pin_t *pin, const char *name, DEV_GPIO_Module_t *gpio);
extern MDS_Err_t DEV_GPIO_PinDeInit(DEV_GPIO_Pin_t *pin);
extern DEV_GPIO_Pin_t *DEV_GPIO_PinCreate(const char *name, DEV_GPIO_Module_t *gpio);
extern MDS_Err_t DEV_GPIO_PinDestroy(DEV_GPIO_Pin_t *pin);

extern MDS_Err_t DEV_GPIO_PinConfig(DEV_GPIO_Pin_t *pin, const DEV_GPIO_Config_t *config);
extern void DEV_GPIO_PinInterruptCallback(DEV_GPIO_Pin_t *pin, void (*callback)(const DEV_GPIO_Pin_t *, MDS_Arg_t *),
                                          MDS_Arg_t *arg);
extern MDS_Mask_t DEV_GPIO_PinRead(const DEV_GPIO_Pin_t *pin);
extern void DEV_GPIO_PinWrite(DEV_GPIO_Pin_t *pin, MDS_Mask_t val);
extern void DEV_GPIO_PinToggle(DEV_GPIO_Pin_t *pin);
extern void DEV_GPIO_PinLow(DEV_GPIO_Pin_t *pin);
extern void DEV_GPIO_PinHigh(DEV_GPIO_Pin_t *pin);
extern void DEV_GPIO_PinActive(DEV_GPIO_Pin_t *pin, bool actived);
extern bool DEV_GPIO_PinIsActived(const DEV_GPIO_Pin_t *pin);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_GPIO_H__ */
