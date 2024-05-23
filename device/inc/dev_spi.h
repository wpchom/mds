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
#ifndef __DEV_SPI_H__
#define __DEV_SPI_H__

/* Include ----------------------------------------------------------------- */
#include "mds_dev.h"
#include "dev_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct DEV_SPI_Msg {
    const uint8_t *tx;
    uint8_t *rx;
    size_t size;
    struct DEV_SPI_Msg *next;
} DEV_SPI_Msg_t;

typedef enum DEV_SPI_BusMode {
    DEV_SPI_BUSMODE_MASTER,
    DEV_SPI_BUSMODE_MASTER_HALF,
    DEV_SPI_BUSMODE_SLAVE,
    DEV_SPI_BUSMODE_SLAVE_HALF,
} DEV_SPI_BusMode_t;

typedef enum DEV_SPI_ClkMode {
    DEV_SPI_CLKMODE_0,  // CPOL_0, CPHA_0
    DEV_SPI_CLKMODE_1,  // CPOL_0, CPHA_1
    DEV_SPI_CLKMODE_2,  // CPOL_1, CPHA_0
    DEV_SPI_CLKMODE_3,  // CPOL_1, CPHA_1
} DEV_SPI_ClkMode_t;

typedef enum DEV_SPI_DataBits {
    DEV_SPI_DATABITS_8 = 8,
    DEV_SPI_DATABITS_16 = 16,
    DEV_SPI_DATABITS_32 = 32,
} DEV_SPI_DataBits_t;

typedef enum DEV_SPI_FirstBit {
    DEV_SPI_FIRSTBIT_MSB,
    DEV_SPI_FIRSTBIT_LSB,
} DEV_SPI_FirstBit_t;

typedef enum DEV_SPI_BusCS {
    DEV_SPI_BUSCS_LOW,
    DEV_SPI_BUSCS_HIGH,
    DEV_SPI_BUSCS_NO,
} DEV_SPI_BusCS_t;

typedef struct DEV_SPI_Config {
    uint32_t clock;  // Hz
    DEV_SPI_BusMode_t busMode   : 8;
    DEV_SPI_DataBits_t dataBits : 8;
    DEV_SPI_ClkMode_t clkMode   : 8;
    DEV_SPI_FirstBit_t firstBit : 8;
} DEV_SPI_Config_t;

typedef struct DEV_SPI_Adaptr DEV_SPI_Adaptr_t;
typedef struct DEV_SPI_Periph DEV_SPI_Periph_t;

typedef struct DEV_SPI_Driver {
    MDS_Err_t (*control)(const DEV_SPI_Adaptr_t *spi, MDS_Item_t cmd, MDS_Arg_t *arg);
    MDS_Err_t (*transfer)(const DEV_SPI_Periph_t *periph, const uint8_t *tx, uint8_t *rx, size_t size);
} DEV_SPI_Driver_t;

struct DEV_SPI_Adaptr {
    const MDS_Device_t device;
    const DEV_SPI_Driver_t *driver;
    const MDS_DevHandle_t *handle;
    const DEV_SPI_Periph_t *owner;
    const MDS_Mutex_t mutex;
};

typedef struct DEV_SPI_Object {
    MDS_Tick_t timeout;
    DEV_GPIO_Pin_t *nss;
    DEV_SPI_BusCS_t busCS : 8;
    uint8_t retry;
} DEV_SPI_Object_t;

struct DEV_SPI_Periph {
    const MDS_Device_t device;
    const DEV_SPI_Adaptr_t *mount;

    DEV_SPI_Config_t config;
    DEV_SPI_Object_t object;

    void (*callback)(const DEV_SPI_Periph_t *periph, MDS_Arg_t *arg, const uint8_t *tx, uint8_t *rx, size_t size,
                     size_t trans);
    MDS_Arg_t *arg;
};

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DEV_SPI_AdaptrInit(DEV_SPI_Adaptr_t *spi, const char *name, const DEV_SPI_Driver_t *driver,
                                    MDS_DevHandle_t *handle, const MDS_Arg_t *init);
extern MDS_Err_t DEV_SPI_AdaptrDeInit(DEV_SPI_Adaptr_t *spi);
extern DEV_SPI_Adaptr_t *DEV_SPI_AdaptrCreate(const char *name, const DEV_SPI_Driver_t *driver, const MDS_Arg_t *init);
extern MDS_Err_t DEV_SPI_AdaptrDestroy(DEV_SPI_Adaptr_t *spi);

extern MDS_Err_t DEV_SPI_PeriphInit(DEV_SPI_Periph_t *periph, const char *name, DEV_SPI_Adaptr_t *spi);
extern MDS_Err_t DEV_SPI_PeriphDeInit(DEV_SPI_Periph_t *periph);
extern DEV_SPI_Periph_t *DEV_SPI_PeriphCreate(const char *name, DEV_SPI_Adaptr_t *spi);
extern MDS_Err_t DEV_SPI_PeriphDestroy(DEV_SPI_Periph_t *periph);

extern MDS_Err_t DEV_SPI_PeriphOpen(DEV_SPI_Periph_t *periph, MDS_Tick_t timeout);
extern MDS_Err_t DEV_SPI_PeriphClose(DEV_SPI_Periph_t *periph);
extern void DEV_SPI_PeriphCallback(DEV_SPI_Periph_t *periph,
                                   void (*callback)(const DEV_SPI_Periph_t *, MDS_Arg_t *, const uint8_t *, uint8_t *,
                                                    size_t, size_t),
                                   MDS_Arg_t *arg);
extern MDS_Err_t DEV_SPI_PeriphTransferMsg(DEV_SPI_Periph_t *periph, const DEV_SPI_Msg_t *msg);
extern MDS_Err_t DEV_SPI_PeriphTransfer(DEV_SPI_Periph_t *periph, const uint8_t *tx, uint8_t *rx, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_SPI_H__ */
