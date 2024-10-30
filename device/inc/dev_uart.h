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
#ifndef __DEV_UART_H__
#define __DEV_UART_H__

/* Include ----------------------------------------------------------------- */
#include "mds_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
enum DEV_UART_Baudrate {
    DEV_UART_BAUDRATE_2400 = 2400UL,
    DEV_UART_BAUDRATE_4800 = 4800UL,
    DEV_UART_BAUDRATE_9600 = 9600UL,
    DEV_UART_BAUDRATE_19200 = 19200UL,
    DEV_UART_BAUDRATE_38400 = 38400UL,
    DEV_UART_BAUDRATE_57600 = 57600UL,
    DEV_UART_BAUDRATE_115200 = 115200UL,
    DEV_UART_BAUDRATE_230400 = 230400UL,
    DEV_UART_BAUDRATE_460800 = 460800UL,
    DEV_UART_BAUDRATE_921600 = 921600UL,
};

typedef enum DEV_UART_DataBits {
    DEV_UART_DATABITS_7 = 7U,
    DEV_UART_DATABITS_8 = 8U,
    DEV_UART_DATABITS_9 = 9U,
} DEV_UART_DataBits_t;

typedef enum DEV_UART_StopBits {
    DEV_UART_STOPBITS_0_5,
    DEV_UART_STOPBITS_1,
    DEV_UART_STOPBITS_1_5,
    DEV_UART_STOPBITS_2,
} DEV_UART_StopBits_t;

typedef enum DEV_UART_Direct {
    DEV_UART_DIRECT_NONE = 0x00U,
    DEV_UART_DIRECT_TX = 0x01U,
    DEV_UART_DIRECT_RX = 0x02U,
    DEV_UART_DIRECT_FULL = 0x03U,
    DEV_UART_DIRECT_HALF = 0x08U,
} DEV_UART_Direct_t;

typedef enum DEV_UART_Parity {
    DEV_UART_PARITY_NONE = 0U,
    DEV_UART_PARITY_ODD = 1U,
    DEV_UART_PARITY_EVEN = 2U,
} DEV_UART_Parity_t;

typedef enum DEV_UART_HwFlowCtl {
    DEV_UART_HWFLOWCTL_NONE,
    DEV_UART_HWFLOWCTL_RTS,
    DEV_UART_HWFLOWCTL_CTS,
    DEV_UART_HWFLOWCTL_RTS_CTS,
} DEV_UART_HwFlowCtl_t;

typedef struct DEV_UART_Object {
    MDS_Tick_t optick;  // transmit
    uint32_t baudrate;
    DEV_UART_DataBits_t databits   : 4;
    DEV_UART_StopBits_t stopbits   : 4;
    DEV_UART_Direct_t direct       : 4;
    DEV_UART_Parity_t parity       : 2;
    DEV_UART_HwFlowCtl_t hwFlowCtl : 2;
} DEV_UART_Object_t;

enum DEV_UART_Cmd {
    DEV_UART_CMD_DIRECT = MDS_DEVICE_CMD_DRIVER,
};

typedef struct DEV_UART_Adaptr DEV_UART_Adaptr_t;
typedef struct DEV_UART_Periph DEV_UART_Periph_t;

typedef struct DEV_UART_Driver {
    MDS_Err_t (*control)(const DEV_UART_Adaptr_t *uart, MDS_Item_t cmd, MDS_Arg_t *arg);
    MDS_Err_t (*transmit)(const DEV_UART_Periph_t *periph, const uint8_t *buff, size_t len, MDS_Tick_t timeout);
    MDS_Err_t (*receive)(const DEV_UART_Periph_t *periph, uint8_t *buff, size_t size, MDS_Tick_t timeout);
} DEV_UART_Driver_t;

struct DEV_UART_Adaptr {
    const MDS_Device_t device;
    const DEV_UART_Driver_t *driver;
    const MDS_DevHandle_t *handle;
    const DEV_UART_Periph_t *owner;
    const MDS_Mutex_t mutex;
};

struct DEV_UART_Periph {
    const MDS_Device_t device;
    const DEV_UART_Adaptr_t *mount;

    DEV_UART_Object_t object;

    void (*callback)(DEV_UART_Periph_t *periph, MDS_Arg_t *arg, uint8_t *buff, size_t size, size_t recv);
    MDS_Arg_t *arg;
};

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DEV_UART_AdaptrInit(DEV_UART_Adaptr_t *uart, const char *name, const DEV_UART_Driver_t *driver,
                                     MDS_DevHandle_t *handle, const MDS_Arg_t *init);
extern MDS_Err_t DEV_UART_AdaptrDeInit(DEV_UART_Adaptr_t *uart);
extern DEV_UART_Adaptr_t *DEV_UART_AdaptrCreate(const char *name, const DEV_UART_Driver_t *driver,
                                                const MDS_Arg_t *init);
extern MDS_Err_t DEV_UART_AdaptrDestroy(DEV_UART_Adaptr_t *uart);

extern MDS_Err_t DEV_UART_PeriphInit(DEV_UART_Periph_t *periph, const char *name, DEV_UART_Adaptr_t *uart);
extern MDS_Err_t DEV_UART_PeriphDeInit(DEV_UART_Periph_t *periph);
extern DEV_UART_Periph_t *DEV_UART_PeriphCreate(const char *name, DEV_UART_Adaptr_t *uart);
extern MDS_Err_t DEV_UART_PeriphDestroy(DEV_UART_Periph_t *periph);

extern MDS_Err_t DEV_UART_PeriphOpen(DEV_UART_Periph_t *periph, MDS_Tick_t timeout);
extern MDS_Err_t DEV_UART_PeriphClose(DEV_UART_Periph_t *periph);

extern void DEV_UART_PeriphRxCallback(DEV_UART_Periph_t *periph,
                                      void (*callback)(DEV_UART_Periph_t *, MDS_Arg_t *, uint8_t *, size_t, size_t),
                                      MDS_Arg_t *arg);
extern MDS_Err_t DEV_UART_PeriphReceive(DEV_UART_Periph_t *periph, uint8_t *buff, size_t size, MDS_Tick_t timeout);

extern MDS_Err_t DEV_UART_PeriphTransmitMsg(DEV_UART_Periph_t *periph, const MDS_MsgList_t *msg);
extern MDS_Err_t DEV_UART_PeriphTransmit(DEV_UART_Periph_t *periph, const uint8_t *buff, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_UART_H__ */
