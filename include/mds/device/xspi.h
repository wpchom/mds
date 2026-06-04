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
#ifndef __DEV_XSPI_H__
#define __DEV_XSPI_H__

/* Include ----------------------------------------------------------------- */
#include "mds/dev.h"
#include "mds/device/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef enum DEV_XSPI_ClockMode {
    DEV_XSPI_CLKMODE_0, // CPOL_0, CPHA_0
    DEV_XSPI_CLKMODE_3, // CPOL_1, CPHA_1
} __attribute__((packed)) DEV_XSPI_ClockMode_t;

typedef struct DEV_XSPI_Config {
    uint32_t clock; // Hz
    DEV_XSPI_ClockMode_t clkMode : 8;
    uint8_t csIndex;
} DEV_XSPI_Config_t;

typedef struct DEV_XSPI_Object {
    MDS_Timeout_t timeout;
} DEV_XSPI_Object_t;

typedef enum DEV_XSPI_CmdLine {
    DEV_XSPI_CMDLINE_0,
    DEV_XSPI_CMDLINE_1,
    DEV_XSPI_CMDLINE_2,
    DEV_XSPI_CMDLINE_4,
    DEV_XSPI_CMDLINE_8,
} __attribute__((packed)) DEV_XSPI_CmdLine_t;

typedef enum DEV_XSPI_CmdSize {
    DEV_XSPI_CMDSIZE_1B,
    DEV_XSPI_CMDSIZE_2B,
    DEV_XSPI_CMDSIZE_3B,
    DEV_XSPI_CMDSIZE_4B,
    DEV_XSPI_CMDSIZE_8B,
} __attribute__((packed)) DEV_XSPI_CmdSize_t;

typedef enum DEV_XSPI_SIOOMode {
    DEV_XSPI_SIOOMODE_INST_EVERY,
    DEV_XSPI_SIOOMODE_INST_FIRST,
} __attribute__((packed)) DEV_XSPI_SIOOMode_t;

typedef enum DEV_XSPI_DDRMode {
    DEV_XSPI_DDRMODE_DISABLE,
    DEV_XSPI_DDRMODE_ENABLE,
} __attribute__((packed)) DEV_XSPI_DDRMode_t;

typedef enum DEV_XSPI_MatchMode {
    DEV_XSPI_MATCHMODE_AND,
    DEV_XSPI_MATCHMODE_OR,
} __attribute__((packed)) DEV_XSPI_MatchMode_t;

typedef enum DEV_XSPI_AutoStop {
    DEV_XSPI_AUTOMODE_DISABLE,
    DEV_XSPI_AUTOMODE_ENABLE,
} __attribute__((packed)) DEV_XSPI_AutoStop_t;

typedef struct DEV_XSPI_Command {
    uint8_t instruction;
    DEV_XSPI_CmdLine_t instructionLine : 4;
    DEV_XSPI_CmdLine_t addressLine : 4;
    DEV_XSPI_CmdSize_t addressSize : 4;
    DEV_XSPI_CmdLine_t alternateLine : 4;
    DEV_XSPI_CmdSize_t alternateSize : 4;
    DEV_XSPI_CmdLine_t dataLine : 4;
    uint32_t address;
    uint32_t alternate;
    uint32_t dataSize;
    uint8_t dummyCycles;
    DEV_XSPI_SIOOMode_t siooMode : 8;
    DEV_XSPI_DDRMode_t ddrMode : 8;
    uint8_t ddrHoldHalfCycle;
} DEV_XSPI_Command_t;

typedef struct DEV_XSPI_Polling {
    uint32_t match;
    uint32_t mask;
    uint32_t interval : 20;
    uint32_t statusSize : 4;
    DEV_XSPI_MatchMode_t matchMode : 4;
    DEV_XSPI_AutoStop_t autoStop : 4;
    MDS_Timeout_t timeout;
} DEV_XSPI_Polling_t;

typedef struct DEV_XSPI_Adaptr DEV_XSPI_Adaptr_t;
typedef struct DEV_XSPI_Periph DEV_XSPI_Periph_t;

typedef struct DEV_XSPI_Driver {
    MDS_Err_t (*control)(const DEV_XSPI_Adaptr_t *xspi, MDS_DevCmd_t cmd, MDS_Arg_t arg);
    MDS_Err_t (*command)(const DEV_XSPI_Periph_t *periph, const DEV_XSPI_Command_t *cmd,
                         const DEV_XSPI_Polling_t *poll);
    MDS_Err_t (*transmit)(const DEV_XSPI_Periph_t *periph, const uint8_t *tx, size_t size,
                          MDS_Timeout_t timeout);
    MDS_Err_t (*recvice)(const DEV_XSPI_Periph_t *periph, uint8_t *rx, size_t size,
                         MDS_Timeout_t timeout);
} DEV_XSPI_Driver_t;

struct DEV_XSPI_Adaptr {
    const MDS_Device_t device;
    const MDS_DevHandle_t handle;
    const DEV_XSPI_Driver_t *driver;
    const DEV_XSPI_Periph_t *owner;
    const MDS_Mutex_t mutex;
};

struct DEV_XSPI_Periph {
    const MDS_Device_t device;
    const DEV_XSPI_Adaptr_t *mount;

    DEV_XSPI_Object_t object;
    DEV_XSPI_Config_t config;

    void (*callback)(DEV_XSPI_Periph_t *periph, MDS_Arg_t arg, const uint8_t *tx, uint8_t *rx,
                     size_t size, size_t trans);
    MDS_Arg_t arg;
};

/* Function ---------------------------------------------------------------- */
MDS_Err_t DEV_XSPI_AdaptrInit(DEV_XSPI_Adaptr_t *xspi, const char *name,
                              const DEV_XSPI_Driver_t *driver, MDS_DevHandle_t handle,
                              MDS_Arg_t init);
MDS_Err_t DEV_XSPI_AdaptrDeInit(DEV_XSPI_Adaptr_t *xspi);
DEV_XSPI_Adaptr_t *DEV_XSPI_AdaptrCreate(const char *name, const DEV_XSPI_Driver_t *driver,
                                         MDS_Arg_t init);
MDS_Err_t DEV_XSPI_AdaptrDestroy(DEV_XSPI_Adaptr_t *xspi);

MDS_Err_t DEV_XSPI_PeriphInit(DEV_XSPI_Periph_t *periph, const char *name, DEV_XSPI_Adaptr_t *xspi);
MDS_Err_t DEV_XSPI_PeriphDeInit(DEV_XSPI_Periph_t *periph);
DEV_XSPI_Periph_t *DEV_XSPI_PeriphCreate(const char *name, DEV_XSPI_Adaptr_t *xspi);
MDS_Err_t DEV_XSPI_PeriphDestroy(DEV_XSPI_Periph_t *periph);

MDS_Err_t DEV_XSPI_PeriphOpen(DEV_XSPI_Periph_t *periph, MDS_Timeout_t timeout);
MDS_Err_t DEV_XSPI_PeriphClose(DEV_XSPI_Periph_t *periph);
void DEV_XSPI_PeriphCallback(DEV_XSPI_Periph_t *periph,
                             void (*callback)(DEV_XSPI_Periph_t *, MDS_Arg_t, const uint8_t *,
                                              uint8_t *, size_t, size_t),
                             MDS_Arg_t arg);
MDS_Err_t DEV_XSPI_PeriphCommand(DEV_XSPI_Periph_t *periph, const DEV_XSPI_Command_t *cmd);
MDS_Err_t DEV_XSPI_PeriphPolling(DEV_XSPI_Periph_t *periph, const DEV_XSPI_Command_t *cmd,
                                 const DEV_XSPI_Polling_t *poll);
MDS_Err_t DEV_XSPI_PeriphTransmit(DEV_XSPI_Periph_t *periph, const uint8_t *tx, size_t size);
MDS_Err_t DEV_XSPI_PeriphReceive(DEV_XSPI_Periph_t *periph, uint8_t *rx, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_XSPI_H__ */
