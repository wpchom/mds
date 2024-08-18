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
#ifndef __DEV_FPGA_H__
#define __DEV_FPGA_H__

/* Include ----------------------------------------------------------------- */
#include "dev_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct DEV_FPGA_Adaptr DEV_FPGA_Adaptr_t;
typedef struct DEV_FPGA_Periph DEV_FPGA_Periph_t;

typedef struct DEV_FPGA_Driver {
    MDS_Err_t (*control)(const DEV_FPGA_Adaptr_t *fpga, MDS_Item_t cmd, MDS_Arg_t *arg);
    MDS_Err_t (*start)(const DEV_FPGA_Periph_t *periph);
    MDS_Err_t (*transmit)(const DEV_FPGA_Periph_t *periph, const uint8_t *buff, size_t len);
    MDS_Err_t (*finish)(const DEV_FPGA_Periph_t *periph);
} DEV_FPGA_Driver_t;

struct DEV_FPGA_Adaptr {
    const MDS_Device_t device;
    const DEV_FPGA_Driver_t *driver;
    const MDS_DevHandle_t *handle;
    const DEV_FPGA_Periph_t *owner;
    const MDS_Mutex_t mutex;
};

typedef struct DEV_FPGA_Object {
    MDS_Tick_t optick;
    MDS_Tick_t clock;
    DEV_GPIO_Pin_t *prog_b;
    DEV_GPIO_Pin_t *init_b;
    DEV_GPIO_Pin_t *done;
} DEV_FPGA_Object_t;

struct DEV_FPGA_Periph {
    const MDS_Device_t device;
    const DEV_FPGA_Adaptr_t *mount;

    DEV_FPGA_Object_t object;
};

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DEV_FPGA_AdaptrInit(DEV_FPGA_Adaptr_t *fpga, const char *name, const DEV_FPGA_Driver_t *driver,
                                     MDS_DevHandle_t *handle, const MDS_Arg_t *init);
extern MDS_Err_t DEV_FPGA_AdaptrDeInit(DEV_FPGA_Adaptr_t *fpga);
extern DEV_FPGA_Adaptr_t *DEV_FPGA_AdaptrCreate(const char *name, const DEV_FPGA_Driver_t *driver,
                                                const MDS_Arg_t *init);
extern MDS_Err_t DEV_FPGA_AdaptrDestroy(DEV_FPGA_Adaptr_t *fpga);

extern MDS_Err_t DEV_FPGA_PeriphInit(DEV_FPGA_Periph_t *periph, const char *name, DEV_FPGA_Adaptr_t *fpga);
extern MDS_Err_t DEV_FPGA_PeriphDeInit(DEV_FPGA_Periph_t *periph);
extern DEV_FPGA_Periph_t *DEV_FPGA_PeriphCreate(const char *name, DEV_FPGA_Adaptr_t *fpga);
extern MDS_Err_t DEV_FPGA_PeriphDestroy(DEV_FPGA_Periph_t *periph);
extern MDS_Err_t DEV_FPGA_PeriphOpen(DEV_FPGA_Periph_t *periph, MDS_Tick_t timeout);
extern MDS_Err_t DEV_FPGA_PeriphClose(DEV_FPGA_Periph_t *periph);

extern MDS_Err_t DEV_FPGA_PeriphStart(DEV_FPGA_Periph_t *periph);
extern MDS_Err_t DEV_FPGA_PeriphTransmit(DEV_FPGA_Periph_t *periph, const uint8_t *buff, size_t len);
extern MDS_Err_t DEV_FPGA_PeriphFinish(DEV_FPGA_Periph_t *periph);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_FPGA_H__ */
