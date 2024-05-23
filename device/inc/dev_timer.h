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
#ifndef __DEV_TIMER_H__
#define __DEV_TIMER_H__

/* Include ----------------------------------------------------------------- */
#include "mds_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef enum DEV_TIMER_CounterType {
    DEV_TIMER_COUNTERTYPE_MS,
    DEV_TIMER_COUNTERTYPE_US,
    DEV_TIMER_COUNTERTYPE_NS,
    DEV_TIMER_COUNTERTYPE_COUNT,
    DEV_TIMER_COUNTERTYPE_HZ,
} DEV_TIMER_CounterType_t;

typedef enum DEV_TIMER_CounterMode {
    DEV_TIMER_COUNTERMODE_UP,
    DEV_TIMER_COUNTERMODE_DOWN,
    DEV_TIMER_COUNTERMODE_CENTER_UP,
    DEV_TIMER_COUNTERMODE_CENTER_DOWN,
    DEV_TIMER_COUNTERMODE_CENTER_UP_DOWN,
} DEV_TIMER_CounterMode_t;

typedef enum DEV_TIMER_Period {
    DEV_TIMER_PERIOD_DISABLE,
    DEV_TIMER_PERIOD_ENABLE,
} DEV_TIMER_Period_t;

typedef struct DEV_TIMER_Config {
    DEV_TIMER_CounterType_t type : 8;
    DEV_TIMER_CounterMode_t mode : 8;
    DEV_TIMER_Period_t period    : 8;
} DEV_TIMER_Config_t;

// Output Compare
typedef enum DEV_TIMER_OC_Polarity {
    DEV_TIMER_OC_POLARITY_NONE,
    DEV_TIMER_OC_POLARITY_HIGH,
    DEV_TIMER_OC_POLARITY_LOW,
} DEV_TIMER_OC_Polarity_t;

typedef struct DEV_TIMER_OC_Config {
    DEV_TIMER_OC_Polarity_t pola  : 8;
    DEV_TIMER_OC_Polarity_t npola : 8;
} DEV_TIMER_OC_Config_t;

// Input Capture
typedef enum DEV_TIMER_IC_Type {
    DEV_TIMER_IC_TYPE_NOTHING,
    DEV_TIMER_IC_TYPE_RISING,
    DEV_TIMER_IC_TYPE_FALLING,
    DEV_TIMER_IC_TYPE_BOTH,

    DEV_TIMER_IC_TYPE_DECODE,
} DEV_TIMER_IC_Type_t;

typedef struct DEV_TIMER_IC_Config {
    DEV_TIMER_IC_Type_t type : 8;
} DEV_TIMER_IC_Config_t;

enum DEV_TIMER_Cmd {
    DEV_TIMER_CMD_START = MDS_DEVICE_CMD_DRIVER,
    DEV_TIMER_CMD_STOP,
    DEV_TIMER_CMD_WAIT,

    DEV_TIMER_CMD_OC_CONFIG,
    DEV_TIMER_CMD_OC_ENABLE,
    DEV_TIMER_CMD_OC_DUTY,

    DEV_TIMER_CMD_IC_CONFIG,
    DEV_TIMER_CMD_IC_ENABLE,
    DEV_TIMER_CMD_IC_GET,
    DEV_TIMER_CMD_IC_SET,
};

typedef struct DEV_TIMER_Device DEV_TIMER_Device_t;
typedef struct DEV_TIMER_OC_Channel DEV_TIMER_OC_Channel_t;
typedef struct DEV_TIMER_IC_Channel DEV_TIMER_IC_Channel_t;

typedef struct DEV_TIMER_Driver {
    MDS_Err_t (*control)(const DEV_TIMER_Device_t *timer, MDS_Item_t cmd, MDS_Arg_t *arg);
    MDS_Err_t (*config)(const DEV_TIMER_Device_t *timer, const DEV_TIMER_Config_t *config, MDS_Tick_t timeout);
    MDS_Err_t (*oc)(const DEV_TIMER_OC_Channel_t *oc, MDS_Item_t cmd, const DEV_TIMER_OC_Config_t *config,
                    size_t value);
    MDS_Err_t (*ic)(const DEV_TIMER_IC_Channel_t *ic, MDS_Item_t cmd, const DEV_TIMER_IC_Config_t *config,
                    size_t value);
} DEV_TIMER_Driver_t;

struct DEV_TIMER_Device {
    const MDS_Device_t device;
    const DEV_TIMER_Driver_t *driver;
    const MDS_DevHandle_t *handle;

    void (*callback)(const DEV_TIMER_Device_t *timer, MDS_Arg_t *arg);
    MDS_Arg_t *arg;
};

typedef struct DEV_TIMER_OC_Object {
    uint32_t channel;
    uint32_t channelN;
} DEV_TIMER_OC_Object_t;

struct DEV_TIMER_OC_Channel {
    const MDS_Device_t device;
    const DEV_TIMER_Device_t *mount;

    DEV_TIMER_OC_Object_t object;

    void (*callback)(const DEV_TIMER_OC_Channel_t *oc, MDS_Arg_t *arg);
    MDS_Arg_t *arg;
};

typedef struct DEV_TIMER_IC_Object {
    uint32_t channel;
} DEV_TIMER_IC_Object_t;

struct DEV_TIMER_IC_Channel {
    const MDS_Device_t device;
    const DEV_TIMER_Device_t *mount;

    DEV_TIMER_IC_Object_t object;

    void (*callback)(const DEV_TIMER_IC_Channel_t *ic, MDS_Arg_t *arg);
    MDS_Arg_t *arg;
};

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DEV_TIMER_DeviceInit(DEV_TIMER_Device_t *timer, const char *name, const DEV_TIMER_Driver_t *driver,
                                      MDS_DevHandle_t *handle, const MDS_Arg_t *init);
extern MDS_Err_t DEV_TIMER_DeviceDeInit(DEV_TIMER_Device_t *timer);
extern DEV_TIMER_Device_t *DEV_TIMER_DeviceCreate(const char *name, const DEV_TIMER_Driver_t *driver,
                                                  const MDS_Arg_t *init);
extern MDS_Err_t DEV_TIMER_DeviceDestroy(DEV_TIMER_Device_t *timer);

extern void DEV_TIMER_DeviceCallback(DEV_TIMER_Device_t *timer,
                                     void (*callback)(const DEV_TIMER_Device_t *, MDS_Arg_t *), MDS_Arg_t *arg);
extern MDS_Err_t DEV_TIMER_DeviceConfig(DEV_TIMER_Device_t *timer, const DEV_TIMER_Config_t *config,
                                        MDS_Tick_t timeout);
extern MDS_Err_t DEV_TIMER_DeviceStart(DEV_TIMER_Device_t *timer);
extern MDS_Err_t DEV_TIMER_DeviceStop(DEV_TIMER_Device_t *timer);
extern MDS_Err_t DEV_TIMER_DeviceWait(DEV_TIMER_Device_t *timer, MDS_Tick_t timeout);

extern MDS_Err_t DEV_TIMER_OC_ChannelInit(DEV_TIMER_OC_Channel_t *oc, const char *name, DEV_TIMER_Device_t *timer);
extern MDS_Err_t DEV_TIMER_OC_ChannelDeInit(DEV_TIMER_OC_Channel_t *oc);
extern DEV_TIMER_OC_Channel_t *DEV_TIMER_OC_ChannelCreate(const char *name, DEV_TIMER_Device_t *timer);
extern MDS_Err_t DEV_TIMER_OC_ChannelDestroy(DEV_TIMER_OC_Channel_t *oc);

extern void DEV_TIMER_OC_ChannelCallback(DEV_TIMER_OC_Channel_t *oc,
                                         void (*callback)(const DEV_TIMER_OC_Channel_t *, MDS_Arg_t *), MDS_Arg_t *arg);
extern MDS_Err_t DEV_TIMER_OC_ChannelConfig(DEV_TIMER_OC_Channel_t *oc, const DEV_TIMER_OC_Config_t *config);
extern MDS_Err_t DEV_TIMER_OC_ChannelEnable(DEV_TIMER_OC_Channel_t *oc, bool enabled);
extern MDS_Err_t DEV_TIMER_OC_ChannelDuty(DEV_TIMER_OC_Channel_t *oc, size_t nume, size_t deno);

extern MDS_Err_t DEV_TIMER_IC_ChannelInit(DEV_TIMER_IC_Channel_t *ic, const char *name, DEV_TIMER_Device_t *timer);
extern MDS_Err_t DEV_TIMER_IC_ChannelDeInit(DEV_TIMER_IC_Channel_t *ic);
extern DEV_TIMER_IC_Channel_t *DEV_TIMER_IC_ChannelCreate(const char *name, DEV_TIMER_Device_t *timer);
extern MDS_Err_t DEV_TIMER_IC_ChannelDestroy(DEV_TIMER_IC_Channel_t *ic);

extern void DEV_TIMER_IC_ChannelCallback(DEV_TIMER_IC_Channel_t *ic,
                                         void (*callback)(const DEV_TIMER_IC_Channel_t *, MDS_Arg_t *), MDS_Arg_t *arg);
extern MDS_Err_t DEV_TIMER_IC_ChannelConfig(DEV_TIMER_IC_Channel_t *ic, const DEV_TIMER_IC_Config_t *config);
extern MDS_Err_t DEV_TIMER_IC_ChannelEnable(DEV_TIMER_IC_Channel_t *ic, bool enabled);
extern MDS_Err_t DEV_TIMER_IC_ChannelGetCount(DEV_TIMER_IC_Channel_t *ic, size_t *count);
extern MDS_Err_t DEV_TIMER_IC_ChannelSetCount(DEV_TIMER_IC_Channel_t *ic, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_TIMER_H__ */
