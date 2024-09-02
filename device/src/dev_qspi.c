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
#include "dev_qspi.h"

/* SPI adaptr -------------------------------------------------------------- */
MDS_Err_t DEV_QSPI_AdaptrInit(DEV_QSPI_Adaptr_t *qspi, const char *name, const DEV_QSPI_Driver_t *driver,
                              MDS_DevHandle_t *handle, const MDS_Arg_t *init)
{
    return (MDS_DevAdaptrInit((MDS_DevAdaptr_t *)qspi, name, (const MDS_DevDriver_t *)driver, handle, init));
}

MDS_Err_t DEV_QSPI_AdaptrDeInit(DEV_QSPI_Adaptr_t *qspi)
{
    return (MDS_DevAdaptrDeInit((MDS_DevAdaptr_t *)qspi));
}

DEV_QSPI_Adaptr_t *DEV_QSPI_AdaptrCreate(const char *name, const DEV_QSPI_Driver_t *driver, const MDS_Arg_t *init)
{
    return ((DEV_QSPI_Adaptr_t *)MDS_DevAdaptrCreate(sizeof(DEV_QSPI_Adaptr_t), name, (const MDS_DevDriver_t *)driver,
                                                     init));
}

MDS_Err_t DEV_QSPI_AdaptrDestroy(DEV_QSPI_Adaptr_t *qspi)
{
    return (MDS_DevAdaptrDestroy((MDS_DevAdaptr_t *)qspi));
}

/* SPI periph -------------------------------------------------------------- */
static void DEV_QSPI_PeriphCS(DEV_QSPI_Periph_t *periph, bool chosen)
{
    if ((periph->object.cs != NULL) && (periph->object.busCS != DEV_QSPI_BUSCS_NO)) {
        if (((chosen) && (periph->object.busCS != DEV_QSPI_BUSCS_LOW)) ||
            ((!chosen) && (periph->object.busCS == DEV_QSPI_BUSCS_LOW))) {
            DEV_GPIO_PinHigh(periph->object.cs);
        } else {
            DEV_GPIO_PinLow(periph->object.cs);
        }
    }
}

MDS_Err_t DEV_QSPI_PeriphInit(DEV_QSPI_Periph_t *periph, const char *name, DEV_QSPI_Adaptr_t *qspi)
{
    MDS_Err_t err = MDS_DevPeriphInit((MDS_DevPeriph_t *)periph, name, (MDS_DevAdaptr_t *)qspi);
    if (err == MDS_EOK) {
        periph->object.optick = MDS_DEVICE_PERIPH_TIMEOUT;
    }

    return (err);
}

MDS_Err_t DEV_QSPI_PeriphDeInit(DEV_QSPI_Periph_t *periph)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)periph));
}

DEV_QSPI_Periph_t *DEV_QSPI_PeriphCreate(const char *name, DEV_QSPI_Adaptr_t *qspi)
{
    DEV_QSPI_Periph_t *periph = (DEV_QSPI_Periph_t *)MDS_DevPeriphCreate(sizeof(DEV_QSPI_Periph_t), name,
                                                                         (MDS_DevAdaptr_t *)qspi);
    if (periph != NULL) {
        periph->object.optick = MDS_DEVICE_PERIPH_TIMEOUT;
    }

    return (periph);
}

MDS_Err_t DEV_QSPI_PeriphDestroy(DEV_QSPI_Periph_t *periph)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)periph));
}

MDS_Err_t DEV_QSPI_PeriphOpen(DEV_QSPI_Periph_t *periph, MDS_Tick_t timeout)
{
    MDS_Err_t err = MDS_DevPeriphOpen((MDS_DevPeriph_t *)periph, timeout);

    DEV_QSPI_PeriphCS(periph, true);

    return (err);
}

MDS_Err_t DEV_QSPI_PeriphClose(DEV_QSPI_Periph_t *periph)
{
    MDS_Err_t err = MDS_DevPeriphClose((MDS_DevPeriph_t *)periph);

    DEV_QSPI_PeriphCS(periph, false);

    return (err);
}

void DEV_QSPI_PeriphCallback(
    DEV_QSPI_Periph_t *periph,
    void (*callback)(DEV_QSPI_Periph_t *, MDS_Arg_t *, const uint8_t *, uint8_t *, size_t, size_t), MDS_Arg_t *arg)
{
    periph->callback = callback;
    periph->arg = arg;
}

MDS_Err_t DEV_QSPI_PeriphCommand(DEV_QSPI_Periph_t *periph, const DEV_QSPI_Command_t *cmd)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->command != NULL);

    const DEV_QSPI_Adaptr_t *qspi = periph->mount;

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    MDS_Err_t err = qspi->driver->command(periph, cmd, NULL);

    return (err);
}

MDS_Err_t DEV_QSPI_PeriphPolling(DEV_QSPI_Periph_t *periph, const DEV_QSPI_Command_t *cmd,
                                 const DEV_QSPI_Polling_t *poll)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->command != NULL);

    const DEV_QSPI_Adaptr_t *qspi = periph->mount;

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    MDS_Err_t err = qspi->driver->command(periph, cmd, poll);

    return (err);
}

MDS_Err_t DEV_QSPI_PeriphTransmit(DEV_QSPI_Periph_t *periph, const uint8_t *tx, size_t size)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->transmit != NULL);

    const DEV_QSPI_Adaptr_t *qspi = periph->mount;

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    MDS_Err_t err = qspi->driver->transmit(periph, tx, size);

    return (err);
}

MDS_Err_t DEV_QSPI_PeriphReceive(DEV_QSPI_Periph_t *periph, uint8_t *rx, size_t size)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->recvice != NULL);

    const DEV_QSPI_Adaptr_t *qspi = periph->mount;

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    MDS_Err_t err = qspi->driver->recvice(periph, rx, size);

    return (err);
}
