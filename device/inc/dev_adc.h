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
#ifndef __DEV_ADC_H__
#define __DEV_ADC_H__

/* Include ----------------------------------------------------------------- */
#include "mds_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef enum DEV_ADC_Resolution {
    DEV_ADC_RESOLUTION_6 = 6U,
    DEV_ADC_RESOLUTION_8 = 8U,
    DEV_ADC_RESOLUTION_10 = 10U,
    DEV_ADC_RESOLUTION_12 = 12U,
    DEV_ADC_RESOLUTION_13 = 13U,
    DEV_ADC_RESOLUTION_16 = 16U,
} DEV_ADC_Resolution_t;

typedef enum DEV_ADC_InputMode {
    DEV_ADC_INPUTMODE_SINGLE,
    DEV_ADC_INPUTMODE_DIFF,
} DEV_ADC_InputMode_t;

typedef struct DEV_ADC_Object {
    MDS_Tick_t optick;
    uint32_t channelP;
    uint32_t channelN;
    uint32_t samptime;
    DEV_ADC_Resolution_t resolution : 8;
    DEV_ADC_InputMode_t inputMode   : 8;
    uint16_t averages;
} DEV_ADC_Object_t;

typedef struct DEV_ADC_Adaptr DEV_ADC_Adaptr_t;
typedef struct DEV_ADC_Periph DEV_ADC_Periph_t;

typedef struct DEV_ADC_Driver {
    MDS_Err_t (*control)(const DEV_ADC_Adaptr_t *adc, MDS_Item_t cmd, MDS_Arg_t *arg);
    MDS_Err_t (*convert)(const DEV_ADC_Periph_t *periph, int32_t *val);
} DEV_ADC_Driver_t;

struct DEV_ADC_Adaptr {
    const MDS_Device_t device;
    const DEV_ADC_Driver_t *driver;
    const MDS_DevHandle_t *handle;
    const DEV_ADC_Periph_t *owner;
    const MDS_Mutex_t mutex;

    uint32_t refVoltage;  // mV
};

struct DEV_ADC_Periph {
    const MDS_Device_t device;
    const DEV_ADC_Adaptr_t *mount;

    DEV_ADC_Object_t object;
};

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DEV_ADC_AdaptrInit(DEV_ADC_Adaptr_t *adc, const char *name, const DEV_ADC_Driver_t *driver,
                                    MDS_DevHandle_t *handle, const MDS_Arg_t *init);
extern MDS_Err_t DEV_ADC_AdaptrDeInit(DEV_ADC_Adaptr_t *adc);
extern DEV_ADC_Adaptr_t *DEV_ADC_AdaptrCreate(const char *name, const DEV_ADC_Driver_t *driver, const MDS_Arg_t *init);
extern MDS_Err_t DEV_ADC_AdaptrDestroy(DEV_ADC_Adaptr_t *adc);

extern MDS_Err_t DEV_ADC_PeriphInit(DEV_ADC_Periph_t *periph, const char *name, DEV_ADC_Adaptr_t *adc);
extern MDS_Err_t DEV_ADC_PeriphDeInit(DEV_ADC_Periph_t *periph);
extern DEV_ADC_Periph_t *DEV_ADC_PeriphCreate(const char *name, DEV_ADC_Adaptr_t *adc);
extern MDS_Err_t DEV_ADC_PeriphDestroy(DEV_ADC_Periph_t *periph);

extern MDS_Err_t DEV_ADC_PeriphOpen(DEV_ADC_Periph_t *periph, MDS_Tick_t timeout);
extern MDS_Err_t DEV_ADC_PeriphClose(DEV_ADC_Periph_t *periph);
extern MDS_Err_t DEV_ADC_PeriphConvert(DEV_ADC_Periph_t *periph, int32_t *value, int32_t *voltage);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_ADC_H__ */
