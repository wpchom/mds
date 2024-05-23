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
#ifndef __DEV_NTC_H__
#define __DEV_NTC_H__

/* Include ----------------------------------------------------------------- */
#include "mds_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct DEV_NTC_Value {
    uint32_t resistance;  // ohm
    int32_t temperature;  // C
} DEV_NTC_Value_t;

enum DEV_NTC_Cmd {
    DEV_NTC_CMD_GET_VALUE = MDS_DEVICE_CMD_DRIVER,
};

typedef struct DEV_NTC_Device DEV_NTC_Device_t;
typedef struct DEV_NTC_Driver {
    MDS_Err_t (*control)(const DEV_NTC_Device_t *ntc, MDS_Item_t cmd, MDS_Arg_t *arg);
} DEV_NTC_Driver_t;

struct DEV_NTC_Device {
    const MDS_Device_t device;
    const DEV_NTC_Driver_t *driver;
    const MDS_DevHandle_t *handle;

    void (*compensation)(DEV_NTC_Value_t *);
};

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DEV_NTC_DeviceInit(DEV_NTC_Device_t *ntc, const char *name, const DEV_NTC_Driver_t *driver,
                                    MDS_DevHandle_t *handle, const MDS_Arg_t *init);
extern MDS_Err_t DEV_NTC_DeviceDeInit(DEV_NTC_Device_t *ntc);
extern DEV_NTC_Device_t *DEV_NTC_DeviceCreate(const char *name, const DEV_NTC_Driver_t *driver, const MDS_Arg_t *init);
extern MDS_Err_t DEV_NTC_DeviceDestroy(DEV_NTC_Device_t *ntc);

extern void DEV_NTC_DeviceCompensation(DEV_NTC_Device_t *ntc, void (*compensation)(DEV_NTC_Value_t *));
extern MDS_Err_t DEV_NTC_DeviceGetValue(DEV_NTC_Device_t *ntc, DEV_NTC_Value_t *value);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_NTC_H__ */
