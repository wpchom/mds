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
#include "dev_spi.h"

/* SPI adaptr -------------------------------------------------------------- */
MDS_Err_t DEV_SPI_AdaptrInit(DEV_SPI_Adaptr_t *spi, const char *name, const DEV_SPI_Driver_t *driver,
                             MDS_DevHandle_t *handle, const MDS_Arg_t *init)
{
    return (MDS_DevAdaptrInit((MDS_DevAdaptr_t *)spi, name, (const MDS_DevDriver_t *)driver, handle, init));
}

MDS_Err_t DEV_SPI_AdaptrDeInit(DEV_SPI_Adaptr_t *spi)
{
    return (MDS_DevAdaptrDeInit((MDS_DevAdaptr_t *)spi));
}

DEV_SPI_Adaptr_t *DEV_SPI_AdaptrCreate(const char *name, const DEV_SPI_Driver_t *driver, const MDS_Arg_t *init)
{
    return ((DEV_SPI_Adaptr_t *)MDS_DevAdaptrCreate(sizeof(DEV_SPI_Adaptr_t), name, (const MDS_DevDriver_t *)driver,
                                                    init));
}

MDS_Err_t DEV_SPI_AdaptrDestroy(DEV_SPI_Adaptr_t *spi)
{
    return (MDS_DevAdaptrDestroy((MDS_DevAdaptr_t *)spi));
}

/* SPI periph -------------------------------------------------------------- */
MDS_Err_t DEV_SPI_PeriphInit(DEV_SPI_Periph_t *periph, const char *name, DEV_SPI_Adaptr_t *spi)
{
    MDS_Err_t err = MDS_DevPeriphInit((MDS_DevPeriph_t *)periph, name, (MDS_DevAdaptr_t *)spi);
    if (err == MDS_EOK) {
        periph->object.optick = MDS_DEVICE_PERIPH_TIMEOUT;
    }

    return (err);
}

MDS_Err_t DEV_SPI_PeriphDeInit(DEV_SPI_Periph_t *periph)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)periph));
}

DEV_SPI_Periph_t *DEV_SPI_PeriphCreate(const char *name, DEV_SPI_Adaptr_t *spi)
{
    DEV_SPI_Periph_t *periph = (DEV_SPI_Periph_t *)MDS_DevPeriphCreate(sizeof(DEV_SPI_Periph_t), name,
                                                                       (MDS_DevAdaptr_t *)spi);
    if (periph != NULL) {
        periph->object.optick = MDS_DEVICE_PERIPH_TIMEOUT;
    }

    return (periph);
}

MDS_Err_t DEV_SPI_PeriphDestroy(DEV_SPI_Periph_t *periph)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)periph));
}

MDS_Err_t DEV_SPI_PeriphOpen(DEV_SPI_Periph_t *periph, MDS_Tick_t timeout)
{
    return (MDS_DevPeriphOpen((MDS_DevPeriph_t *)periph, timeout));
}

MDS_Err_t DEV_SPI_PeriphClose(DEV_SPI_Periph_t *periph)
{
    return (MDS_DevPeriphClose((MDS_DevPeriph_t *)periph));
}

void DEV_SPI_PeriphCallback(
    DEV_SPI_Periph_t *periph,
    void (*callback)(const DEV_SPI_Periph_t *, MDS_Arg_t *, const uint8_t *, uint8_t *, size_t, size_t), MDS_Arg_t *arg)
{
    periph->callback = callback;
    periph->arg = arg;
}

static void DEV_SPI_PeriphCS(DEV_SPI_Periph_t *periph, bool chosen)
{
    if ((periph->object.nss != NULL) && (periph->object.busCS != DEV_SPI_BUSCS_NO)) {
        if (((chosen) && (periph->object.busCS != DEV_SPI_BUSCS_LOW)) ||
            ((!chosen) && (periph->object.busCS == DEV_SPI_BUSCS_LOW))) {
            DEV_GPIO_PinHigh(periph->object.nss);
        } else {
            DEV_GPIO_PinLow(periph->object.nss);
        }
    }
}

MDS_Err_t DEV_SPI_PeriphTransferMsg(DEV_SPI_Periph_t *periph, const DEV_SPI_Msg_t *msg)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->transfer != NULL);

    MDS_Err_t err = MDS_EINVAL;
    const DEV_SPI_Adaptr_t *spi = periph->mount;

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    for (size_t retry = 0; (err != MDS_EOK) && (retry <= periph->object.retry); retry++) {
        const DEV_SPI_Msg_t *cur = msg;

        DEV_SPI_PeriphCS(periph, true);
        while (cur != NULL) {
            err = spi->driver->transfer(periph, cur->tx, cur->rx, cur->size);
            if (err != MDS_EOK) {
                break;
            }
            cur = cur->next;
        }
        DEV_SPI_PeriphCS(periph, false);
    }

    return (err);
}

MDS_Err_t DEV_SPI_PeriphTransfer(DEV_SPI_Periph_t *periph, const uint8_t *tx, uint8_t *rx, size_t size)
{
    DEV_SPI_Msg_t msg = {
        .tx = tx,
        .rx = rx,
        .size = size,
        .next = NULL,
    };

    return (DEV_SPI_PeriphTransferMsg(periph, &msg));
}
