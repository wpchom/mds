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
#ifndef __DEV_I2S_H__
#define __DEV_I2S_H__

/* Include ----------------------------------------------------------------- */
#include "mds_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
enum DEV_I2S_AudioFreq {
    DEV_I2S_AUDIOFREQ_8K = 8000UL,
    DEV_I2S_AUDIOFREQ_11K = 11025UL,
    DEV_I2S_AUDIOFREQ_16K = 16000UL,
    DEV_I2S_AUDIOFREQ_22K = 22050UL,
    DEV_I2S_AUDIOFREQ_24K = 24000UL,
    DEV_I2S_AUDIOFREQ_32K = 32000UL,
    DEV_I2S_AUDIOFREQ_44K = 44100UL,
    DEV_I2S_AUDIOFREQ_48K = 48000UL,
    DEV_I2S_AUDIOFREQ_96K = 96000UL,
};

typedef enum DEV_I2S_Standard {
    DEV_I2S_STANDARD_PHILIPS,
    DEV_I2S_STANDARD_MSB,
    DEV_I2S_STANDARD_LSB,
    DEV_I2S_STANDARD_PCM_SHORT,
    DEV_I2S_STANDARD_PCM_LONG,
} DEV_I2S_Standard_t;

typedef enum DEV_I2S_BusMode {
    DEV_I2S_BUSMODE_MASTER = 0x00U,
    DEV_I2S_BUSMODE_SLAVE = 0x01U,
} DEV_I2S_BusMode_t;

typedef enum DEV_I2S_DataWidth {
    DEV_I2S_DATAWIDTH_16 = 16U,
    DEV_I2S_DATAWIDTH_24 = 24U,
    DEV_I2S_DATAWIDTH_32 = 32U,
} DEV_I2S_DataWidth_t;

typedef enum DEV_I2S_Channel {
    DEV_I2S_CHANNEL_MONO = 0x01U,
    DEV_I2S_CHANNEL_STEREO = 0x02U,
} DEV_I2S_Channel_t;

typedef enum DEV_I2S_ClkEdge {
    DEV_I2S_CLKEDGE_RISING = 0,
    DEV_I2S_CLKEDGE_FALLING = 1,
} DEV_I2S_ClkEdge_t;

typedef enum DEV_I2S_FirstBit {
    DEV_I2S_FIRSTBIT_MSB = 0,
    DEV_I2S_FIRSTBIT_LSB = 1,
} DEV_I2S_FirstBit_t;

typedef enum DEV_I2S_WsInversion {
    DEV_I2S_WSINVERSION_DISABLE = 0,
    DEV_I2S_WSINVERSION_ENABLE = 1,
} DEV_I2S_WsInversion_t;

typedef struct DEV_I2S_Object {
    MDS_Tick_t optick;
    uint32_t audioFreq;
    DEV_I2S_Standard_t standard   : 8;
    DEV_I2S_DataWidth_t dataWidth : 8;
    DEV_I2S_Channel_t channel     : 8;
    DEV_I2S_BusMode_t busMode     : 2;
    DEV_I2S_ClkEdge_t clkEdge     : 2;
    DEV_I2S_FirstBit_t firstBit   : 2;
    DEV_I2S_WsInversion_t ws      : 2;
} DEV_I2S_Object_t;

typedef struct DEV_I2S_Adaptr DEV_I2S_Adaptr_t;
typedef struct DEV_I2S_Periph DEV_I2S_Periph_t;

typedef struct DEV_I2S_Driver {
    MDS_Err_t (*control)(const DEV_I2S_Adaptr_t *i2s, MDS_Item_t cmd, MDS_Arg_t *arg);
    MDS_Err_t (*transmit)(const DEV_I2S_Periph_t *periph, const uint8_t *buff, size_t len);
    MDS_Err_t (*receive)(const DEV_I2S_Periph_t *periph, uint8_t *buff, size_t size, size_t *recv, MDS_Tick_t timeout);
} DEV_I2S_Driver_t;

struct DEV_I2S_Adaptr {
    const MDS_Device_t device;
    const DEV_I2S_Driver_t *driver;
    const MDS_DevHandle_t *handle;
    const DEV_I2S_Periph_t *owner;
    const MDS_Mutex_t mutex;
};

struct DEV_I2S_Periph {
    const MDS_Device_t device;
    const DEV_I2S_Adaptr_t *mount;

    DEV_I2S_Object_t object;

    void (*txCallback)(DEV_I2S_Periph_t *periph, MDS_Arg_t *arg, const uint8_t *buff, size_t size, size_t send);
    MDS_Arg_t *txArg;

    void (*rxCallback)(DEV_I2S_Periph_t *periph, MDS_Arg_t *arg, uint8_t *buff, size_t size, size_t recv);
    MDS_Arg_t *rxArg;
};

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DEV_I2S_AdaptrInit(DEV_I2S_Adaptr_t *i2s, const char *name, const DEV_I2S_Driver_t *driver,
                                    MDS_DevHandle_t *handle, const MDS_Arg_t *init);
extern MDS_Err_t DEV_I2S_AdaptrDeInit(DEV_I2S_Adaptr_t *i2s);
extern DEV_I2S_Adaptr_t *DEV_I2S_AdaptrCreate(const char *name, const DEV_I2S_Driver_t *driver, const MDS_Arg_t *init);
extern MDS_Err_t DEV_I2S_AdaptrDestroy(DEV_I2S_Adaptr_t *i2s);

extern MDS_Err_t DEV_I2S_PeriphInit(DEV_I2S_Periph_t *periph, const char *name, DEV_I2S_Adaptr_t *i2s);
extern MDS_Err_t DEV_I2S_PeriphDeInit(DEV_I2S_Periph_t *periph);
extern DEV_I2S_Periph_t *DEV_I2S_PeriphCreate(const char *name, DEV_I2S_Adaptr_t *i2s);
extern MDS_Err_t DEV_I2S_PeriphDestroy(DEV_I2S_Periph_t *periph);

extern MDS_Err_t DEV_I2S_PeriphOpen(DEV_I2S_Periph_t *periph, MDS_Tick_t timeout);
extern MDS_Err_t DEV_I2S_PeriphClose(DEV_I2S_Periph_t *periph);
extern void DEV_I2S_PeriphTxCallback(DEV_I2S_Periph_t *periph,
                                     void (*callback)(DEV_I2S_Periph_t *, MDS_Arg_t *, const uint8_t *, size_t, size_t),
                                     MDS_Arg_t *arg);
extern void DEV_I2S_PeriphRxCallback(DEV_I2S_Periph_t *periph,
                                     void (*callback)(DEV_I2S_Periph_t *, MDS_Arg_t *, uint8_t *, size_t, size_t),
                                     MDS_Arg_t *arg);
extern MDS_Err_t DEV_I2S_PeriphTransmit(DEV_I2S_Periph_t *periph, const uint8_t *buff, size_t len);
extern MDS_Err_t DEV_I2S_PeriphReceive(DEV_I2S_Periph_t *periph, uint8_t *buff, size_t size, size_t *recv,
                                       MDS_Tick_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_I2S_H__ */
