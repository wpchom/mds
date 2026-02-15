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
#ifndef __DEV_I2C_H__
#define __DEV_I2C_H__

/* Include------------------------------------------------------------------ */
#include "mds_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
enum DEV_I2C_MsgFlag {
    DEV_I2C_MSGFLAG_WR = 0x00U,
    DEV_I2C_MSGFLAG_RD = 0x01U,
    DEV_I2C_MSGFLAG_NO_START = 0x04U,
    DEV_I2C_MSGFLAG_NO_STOP = 0x08U,
};

typedef struct DEV_I2C_Msg {
    uint8_t *buff;
    size_t len;
    MDS_Mask_t flag;
} DEV_I2C_Msg_t;

enum DEV_I2C_Baudrate {
    DEV_I2C_BAUDRATE_100K = 100000UL,
    DEV_I2C_BAUDRATE_400K = 400000UL,
    DEV_I2C_BAUDRATE_1000K = 1000000UL,
};

typedef enum DEV_I2C_DevAddrBits {
    DEV_I2C_DEVADDRBITS_7 = 7,
    DEV_I2C_DEVADDRBITS_10 = 10,
} __attribute__((packed)) DEV_I2C_DevAddrBits_t;

typedef struct DEV_I2C_Config {
    uint32_t clock;
    uint16_t devAddress;
    DEV_I2C_DevAddrBits_t devAddrBit : 8;
} DEV_I2C_Config_t;

typedef struct DEV_I2C_Object {
    MDS_Timeout_t timeout;
    uint8_t retry;
} DEV_I2C_Object_t;

typedef struct DEV_I2C_Adaptr DEV_I2C_Adaptr_t;
typedef struct DEV_I2C_Periph DEV_I2C_Periph_t;

typedef struct DEV_I2C_Driver {
    MDS_Err_t (*control)(const DEV_I2C_Adaptr_t *i2c, MDS_DevCmd_t cmd, MDS_Arg_t arg);
    MDS_Err_t (*master)(const DEV_I2C_Periph_t *periph, const DEV_I2C_Msg_t *msg,
                        MDS_Timeout_t timeout);
    MDS_Err_t (*slave)(const DEV_I2C_Periph_t *periph, DEV_I2C_Msg_t *msg, size_t *len,
                       MDS_Timeout_t timeout);
} DEV_I2C_Driver_t;

struct DEV_I2C_Adaptr {
    const MDS_Device_t device;
    const DEV_I2C_Driver_t *driver;
    const MDS_DevHandle_t *handle;
    const DEV_I2C_Periph_t *owner;
    const MDS_Mutex_t mutex;
};

struct DEV_I2C_Periph {
    const MDS_Device_t device;
    const DEV_I2C_Adaptr_t *mount;

    DEV_I2C_Object_t object;
    DEV_I2C_Config_t config;

    void (*callback)(DEV_I2C_Periph_t *periph, MDS_Arg_t arg, MDS_Mask_t flag);
    MDS_Arg_t arg;
};

/* Function ---------------------------------------------------------------- */
MDS_Err_t DEV_I2C_AdaptrInit(DEV_I2C_Adaptr_t *i2c, const char *name,
                             const DEV_I2C_Driver_t *driver, MDS_DevHandle_t *handle,
                             MDS_Arg_t init);
MDS_Err_t DEV_I2C_AdaptrDeInit(DEV_I2C_Adaptr_t *i2c);
DEV_I2C_Adaptr_t *DEV_I2C_AdaptrCreate(const char *name, const DEV_I2C_Driver_t *driver,
                                       MDS_Arg_t init);
MDS_Err_t DEV_I2C_AdaptrDestroy(DEV_I2C_Adaptr_t *i2c);

MDS_Err_t DEV_I2C_PeriphInit(DEV_I2C_Periph_t *periph, const char *name, DEV_I2C_Adaptr_t *i2c);
MDS_Err_t DEV_I2C_PeriphDeInit(DEV_I2C_Periph_t *periph);
DEV_I2C_Periph_t *DEV_I2C_PeriphCreate(const char *name, DEV_I2C_Adaptr_t *i2c);
MDS_Err_t DEV_I2C_PeriphDestroy(DEV_I2C_Periph_t *periph);

MDS_Err_t DEV_I2C_PeriphOpen(DEV_I2C_Periph_t *periph, MDS_Timeout_t timeout);
MDS_Err_t DEV_I2C_PeriphClose(DEV_I2C_Periph_t *periph);

void DEV_I2C_PeriphSlaveCallback(DEV_I2C_Periph_t *periph,
                                 void (*callback)(DEV_I2C_Periph_t *, MDS_Arg_t , MDS_Mask_t),
                                 MDS_Arg_t arg);
MDS_Err_t DEV_I2C_PeriphSlaveListen(DEV_I2C_Periph_t *periph, MDS_Timeout_t timeout);
MDS_Err_t DEV_I2C_PeriphSlaveTransfer(DEV_I2C_Periph_t *periph, DEV_I2C_Msg_t *msg, size_t *len,
                                      MDS_Timeout_t timeout);
MDS_Err_t DEV_I2C_PeriphMasterTransfer(DEV_I2C_Periph_t *periph, const DEV_I2C_Msg_t msg[],
                                       size_t num);
MDS_Err_t DEV_I2C_PeriphMasterTransmit(DEV_I2C_Periph_t *periph, const uint8_t *buff, size_t len);
MDS_Err_t DEV_I2C_PeriphMasterReceive(DEV_I2C_Periph_t *periph, uint8_t *buff, size_t size);
MDS_Err_t DEV_I2C_PeriphMasterWriteMem(DEV_I2C_Periph_t *periph, uint32_t memAddr,
                                       uint8_t memAddrSz, const uint8_t *buff, size_t len);
MDS_Err_t DEV_I2C_PeriphMasterReadMem(DEV_I2C_Periph_t *periph, uint32_t memAddr, uint8_t memAddrSz,
                                      uint8_t *buff, size_t len);
MDS_Err_t DEV_I2C_PeriphMasterModifyMem(DEV_I2C_Periph_t *periph, uint32_t memAddr,
                                        uint32_t memAddrSz, uint8_t *buff, size_t len,
                                        const uint8_t *clr, const uint8_t *set);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_I2C_H__ */
