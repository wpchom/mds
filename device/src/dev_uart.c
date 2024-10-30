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
/* Include ----------------------------------------------------------------- */
#include "dev_uart.h"

/* UART adaptr ------------------------------------------------------------- */
MDS_Err_t DEV_UART_AdaptrInit(DEV_UART_Adaptr_t *uart, const char *name, const DEV_UART_Driver_t *driver,
                              MDS_DevHandle_t *handle, const MDS_Arg_t *init)
{
    return (MDS_DevAdaptrInit((MDS_DevAdaptr_t *)uart, name, (const MDS_DevDriver_t *)driver, handle, init));
}

MDS_Err_t DEV_UART_AdaptrDeInit(DEV_UART_Adaptr_t *uart)
{
    return (MDS_DevAdaptrDeInit((MDS_DevAdaptr_t *)uart));
}

DEV_UART_Adaptr_t *DEV_UART_AdaptrCreate(const char *name, const DEV_UART_Driver_t *driver, const MDS_Arg_t *init)
{
    return ((DEV_UART_Adaptr_t *)MDS_DevAdaptrCreate(sizeof(DEV_UART_Adaptr_t), name, (const MDS_DevDriver_t *)driver,
                                                     init));
}

MDS_Err_t DEV_UART_AdaptrDestroy(DEV_UART_Adaptr_t *uart)
{
    return (MDS_DevAdaptrDestroy((MDS_DevAdaptr_t *)uart));
}

/* UART periph ------------------------------------------------------------- */
MDS_Err_t DEV_UART_PeriphInit(DEV_UART_Periph_t *periph, const char *name, DEV_UART_Adaptr_t *uart)
{
    MDS_Err_t err = MDS_DevPeriphInit((MDS_DevPeriph_t *)periph, name, (MDS_DevAdaptr_t *)uart);

    return (err);
}

MDS_Err_t DEV_UART_PeriphDeInit(DEV_UART_Periph_t *periph)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)periph));
}

DEV_UART_Periph_t *DEV_UART_PeriphCreate(const char *name, DEV_UART_Adaptr_t *uart)
{
    DEV_UART_Periph_t *periph = (DEV_UART_Periph_t *)MDS_DevPeriphCreate(sizeof(DEV_UART_Periph_t), name,
                                                                         (MDS_DevAdaptr_t *)uart);

    return (periph);
}

MDS_Err_t DEV_UART_PeriphDestroy(DEV_UART_Periph_t *periph)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)periph));
}

MDS_Err_t DEV_UART_PeriphOpen(DEV_UART_Periph_t *periph, MDS_Tick_t timeout)
{
    return (MDS_DevPeriphOpen((MDS_DevPeriph_t *)periph, timeout));
}

MDS_Err_t DEV_UART_PeriphClose(DEV_UART_Periph_t *periph)
{
    return (MDS_DevPeriphClose((MDS_DevPeriph_t *)periph));
}

void DEV_UART_PeriphRxCallback(DEV_UART_Periph_t *periph,
                               void (*callback)(DEV_UART_Periph_t *, MDS_Arg_t *, uint8_t *, size_t, size_t),
                               MDS_Arg_t *arg)
{
    MDS_ASSERT(periph != NULL);

    periph->callback = callback;
    periph->arg = arg;
}

MDS_Err_t DEV_UART_PeriphTransmitMsg(DEV_UART_Periph_t *periph, const MDS_MsgList_t *msg)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->transmit != NULL);

    MDS_Err_t err = MDS_EINVAL;
    const DEV_UART_Adaptr_t *uart = periph->mount;

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    if ((periph->object.direct & DEV_UART_DIRECT_HALF) != 0U) {
        MDS_Mask_t dir = DEV_UART_DIRECT_HALF | DEV_UART_DIRECT_TX;
        uart->driver->control(uart, DEV_UART_CMD_DIRECT, (MDS_Arg_t *)(&dir));
    }
    for (const MDS_MsgList_t *cur = msg; cur != NULL; cur = cur->next) {
        MDS_Tick_t optick = (periph->object.optick > 0) ? (periph->object.optick)
                                                        : (msg->len / ((periph->object.baudrate >> 0x0E) + 0x01));
        err = uart->driver->transmit(periph, cur->buff, cur->len, optick);
        if (err != MDS_EOK) {
            break;
        }
    }
    if ((periph->object.direct & DEV_UART_DIRECT_HALF) != 0U) {
        MDS_Mask_t dir = DEV_UART_DIRECT_HALF | DEV_UART_DIRECT_RX;
        uart->driver->control(uart, DEV_UART_CMD_DIRECT, (MDS_Arg_t *)(&dir));
    }

    return (err);
}

MDS_Err_t DEV_UART_PeriphTransmit(DEV_UART_Periph_t *periph, const uint8_t *buff, size_t len)
{
    const MDS_MsgList_t msg = {
        .buff = buff,
        .len = len,
        .next = NULL,
    };

    return (DEV_UART_PeriphTransmitMsg(periph, &msg));
}

MDS_Err_t DEV_UART_PeriphReceive(DEV_UART_Periph_t *periph, uint8_t *buff, size_t size, MDS_Tick_t timeout)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->receive != NULL);

    const DEV_UART_Adaptr_t *uart = periph->mount;

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    if ((periph->object.direct & DEV_UART_DIRECT_HALF) != 0U) {
        MDS_Mask_t dir = DEV_UART_DIRECT_HALF | DEV_UART_DIRECT_RX;
        uart->driver->control(uart, DEV_UART_CMD_DIRECT, (MDS_Arg_t *)(&dir));
    }

    return (periph->mount->driver->receive(periph, buff, size, timeout));
}
