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
#ifndef __DEV_QSPI_H__
#define __DEV_QSPI_H__

/* Include ----------------------------------------------------------------- */
#include "mds_dev.h"
#include "dev_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef enum DEV_QSPI_ClockMode {
    DEV_QSPI_CLKMODE_0,  // CPOL_0, CPHA_0
    DEV_QSPI_CLKMODE_1,  // CPOL_0, CPHA_1
    DEV_QSPI_CLKMODE_2,  // CPOL_1, CPHA_0
    DEV_QSPI_CLKMODE_3,  // CPOL_1, CPHA_1
} DEV_QSPI_ClockMode_t;

typedef enum DEV_QSPI_BusCS {
    DEV_QSPI_BUSCS_LOW,
    DEV_QSPI_BUSCS_HIGH,
    DEV_QSPI_BUSCS_NO,
} DEV_QSPI_BusCS_t;

typedef struct DEV_QSPI_Object {
    DEV_GPIO_Pin_t *cs;
    uint32_t clock;  // Hz
    DEV_QSPI_BusCS_t busCS       : 8;
    DEV_QSPI_ClockMode_t clkMode : 8;
} DEV_QSPI_Object_t;

typedef enum DEV_QSPI_CmdLine {
    DEV_QSPI_CMDLINE_0,
    DEV_QSPI_CMDLINE_1,
    DEV_QSPI_CMDLINE_2,
    DEV_QSPI_CMDLINE_4,
} DEV_QSPI_CmdLine_t;

typedef enum DEV_QSPI_CmdSize {
    DEV_QSPI_CMDSIZE_1B,
    DEV_QSPI_CMDSIZE_2B,
    DEV_QSPI_CMDSIZE_3B,
    DEV_QSPI_CMDSIZE_4B,
} DEV_QSPI_CmdSize_t;

typedef enum DEV_QSPI_SIOOMode {
    DEV_QSPI_SIOOMODE_INST_EVERY,
    DEV_QSPI_SIOOMODE_INST_FIRST,
} DEV_QSPI_SIOOMode_t;

typedef enum DEV_QSPI_DDRMode {
    DEV_QSPI_DDRMODE_DISABLE,
    DEV_QSPI_DDRMODE_ENABLE,
} DEV_QSPI_DDRMode_t;

typedef enum DEV_QSPI_MatchMode {
    DEV_QSPI_MATCHMODE_AND,
    DEV_QSPI_MATCHMODE_OR,
} DEV_QSPI_MatchMode_t;

typedef enum DEV_QSPI_AutoStop {
    DEV_QSPI_AUTOMODE_DISABLE,
    DEV_QSPI_AUTOMODE_ENABLE,
} DEV_QSPI_AutoStop_t;

typedef struct DEV_QSPI_Command {
    uint8_t instruction;
    DEV_QSPI_CmdLine_t instructionLine : 4;
    DEV_QSPI_CmdLine_t addressLine     : 4;
    DEV_QSPI_CmdSize_t addressSize     : 4;
    DEV_QSPI_CmdLine_t alternateLine   : 4;
    DEV_QSPI_CmdSize_t alternateSize   : 4;
    DEV_QSPI_CmdLine_t dataLine        : 4;
    uint32_t address;
    uint32_t alternate;
    uint32_t dataSize;
    uint8_t dummyCycles;
    DEV_QSPI_SIOOMode_t siooMode : 8;
    DEV_QSPI_DDRMode_t ddrMode   : 8;
    uint8_t ddrHoldHalfCycle;
} DEV_QSPI_Command_t;

typedef struct DEV_QSPI_Polling {
    uint32_t match;
    uint32_t mask;
    uint32_t interval              : 20;
    DEV_QSPI_CmdSize_t statusSize  : 4;
    DEV_QSPI_MatchMode_t matchMode : 4;
    DEV_QSPI_AutoStop_t autoStop   : 4;
    MDS_Tick_t timeout;
} DEV_QSPI_Polling_t;

typedef struct DEV_QSPI_Adaptr DEV_QSPI_Adaptr_t;
typedef struct DEV_QSPI_Periph DEV_QSPI_Periph_t;

typedef struct DEV_QSPI_Driver {
    MDS_Err_t (*control)(const DEV_QSPI_Adaptr_t *qspi, MDS_Item_t cmd, MDS_Arg_t *arg);
    MDS_Err_t (*command)(const DEV_QSPI_Periph_t *periph, const DEV_QSPI_Command_t *cmd,
                         const DEV_QSPI_Polling_t *poll);
    MDS_Err_t (*transmit)(const DEV_QSPI_Periph_t *periph, const uint8_t *tx, size_t size, MDS_Tick_t timeout);
    MDS_Err_t (*recvice)(const DEV_QSPI_Periph_t *periph, uint8_t *rx, size_t size, MDS_Tick_t timeout);
} DEV_QSPI_Driver_t;

struct DEV_QSPI_Adaptr {
    const MDS_Device_t device;
    const DEV_QSPI_Driver_t *driver;
    const MDS_DevHandle_t *handle;
    const DEV_QSPI_Periph_t *owner;
    const MDS_Mutex_t mutex;
};

struct DEV_QSPI_Periph {
    const MDS_Device_t device;
    const DEV_QSPI_Adaptr_t *mount;

    DEV_QSPI_Object_t object;

    void (*callback)(DEV_QSPI_Periph_t *periph, MDS_Arg_t *arg, const uint8_t *tx, uint8_t *rx, size_t size,
                     size_t trans);
    MDS_Arg_t *arg;
};

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DEV_QSPI_AdaptrInit(DEV_QSPI_Adaptr_t *qspi, const char *name, const DEV_QSPI_Driver_t *driver,
                                     MDS_DevHandle_t *handle, const MDS_Arg_t *init);
extern MDS_Err_t DEV_QSPI_AdaptrDeInit(DEV_QSPI_Adaptr_t *qspi);
extern DEV_QSPI_Adaptr_t *DEV_QSPI_AdaptrCreate(const char *name, const DEV_QSPI_Driver_t *driver,
                                                const MDS_Arg_t *init);
extern MDS_Err_t DEV_QSPI_AdaptrDestroy(DEV_QSPI_Adaptr_t *qspi);

extern MDS_Err_t DEV_QSPI_PeriphInit(DEV_QSPI_Periph_t *periph, const char *name, DEV_QSPI_Adaptr_t *qspi);
extern MDS_Err_t DEV_QSPI_PeriphDeInit(DEV_QSPI_Periph_t *periph);
extern DEV_QSPI_Periph_t *DEV_QSPI_PeriphCreate(const char *name, DEV_QSPI_Adaptr_t *qspi);
extern MDS_Err_t DEV_QSPI_PeriphDestroy(DEV_QSPI_Periph_t *periph);

extern MDS_Err_t DEV_QSPI_PeriphOpen(DEV_QSPI_Periph_t *periph, MDS_Tick_t timeout);
extern MDS_Err_t DEV_QSPI_PeriphClose(DEV_QSPI_Periph_t *periph);
extern void DEV_QSPI_PeriphCallback(
    DEV_QSPI_Periph_t *periph,
    void (*callback)(DEV_QSPI_Periph_t *, MDS_Arg_t *, const uint8_t *, uint8_t *, size_t, size_t), MDS_Arg_t *arg);
extern MDS_Err_t DEV_QSPI_PeriphCommand(DEV_QSPI_Periph_t *periph, const DEV_QSPI_Command_t *cmd);
extern MDS_Err_t DEV_QSPI_PeriphPolling(DEV_QSPI_Periph_t *periph, const DEV_QSPI_Command_t *cmd,
                                        const DEV_QSPI_Polling_t *poll);
extern MDS_Err_t DEV_QSPI_PeriphTransmit(DEV_QSPI_Periph_t *periph, const uint8_t *tx, size_t size);
extern MDS_Err_t DEV_QSPI_PeriphReceive(DEV_QSPI_Periph_t *periph, uint8_t *rx, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_QSPI_H__ */
