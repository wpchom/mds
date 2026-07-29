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
#include "mds/device/xspi.h"

/* SPI adaptr -------------------------------------------------------------- */
MDS_Err_t DEV_XSPI_AdaptrInit(DEV_XSPI_Adaptr_t *xspi, const char *name,
                              const DEV_XSPI_Driver_t *driver, MDS_DevHandle_t handle,
                              MDS_Arg_t init)
{
    return (MDS_DevAdaptrInit((MDS_DevAdaptr_t *)xspi, name, (const MDS_DevDriver_t *)driver,
                              handle, init));
}

MDS_Err_t DEV_XSPI_AdaptrDeInit(DEV_XSPI_Adaptr_t *xspi)
{
    return (MDS_DevAdaptrDeInit((MDS_DevAdaptr_t *)xspi));
}

DEV_XSPI_Adaptr_t *DEV_XSPI_AdaptrCreate(const char *name, const DEV_XSPI_Driver_t *driver,
                                         const MDS_Arg_t init)
{
    return ((DEV_XSPI_Adaptr_t *)MDS_DevAdaptrCreate(sizeof(DEV_XSPI_Adaptr_t), name,
                                                     (const MDS_DevDriver_t *)driver, init));
}

MDS_Err_t DEV_XSPI_AdaptrDestroy(DEV_XSPI_Adaptr_t *xspi)
{
    return (MDS_DevAdaptrDestroy((MDS_DevAdaptr_t *)xspi));
}

/* SPI periph -------------------------------------------------------------- */
MDS_Err_t DEV_XSPI_PeriphInit(DEV_XSPI_Periph_t *periph, const char *name, DEV_XSPI_Adaptr_t *xspi)
{
    MDS_Err_t err = MDS_DevPeriphInit((MDS_DevPeriph_t *)periph, name, (MDS_DevAdaptr_t *)xspi);

    return (err);
}

MDS_Err_t DEV_XSPI_PeriphDeInit(DEV_XSPI_Periph_t *periph)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)periph));
}

DEV_XSPI_Periph_t *DEV_XSPI_PeriphCreate(const char *name, DEV_XSPI_Adaptr_t *xspi)
{
    DEV_XSPI_Periph_t *periph = (DEV_XSPI_Periph_t *)MDS_DevPeriphCreate(
        sizeof(DEV_XSPI_Periph_t), name, (MDS_DevAdaptr_t *)xspi);

    return (periph);
}

MDS_Err_t DEV_XSPI_PeriphDestroy(DEV_XSPI_Periph_t *periph)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)periph));
}

MDS_Err_t DEV_XSPI_PeriphOpen(DEV_XSPI_Periph_t *periph, MDS_Timeout_t timeout)
{
    return (MDS_DevPeriphOpen((MDS_DevPeriph_t *)periph, timeout));
}

MDS_Err_t DEV_XSPI_PeriphClose(DEV_XSPI_Periph_t *periph)
{
    return (MDS_DevPeriphClose((MDS_DevPeriph_t *)periph));
}

void DEV_XSPI_PeriphCallback(DEV_XSPI_Periph_t *periph,
                             void (*callback)(DEV_XSPI_Periph_t *, MDS_Arg_t, const uint8_t *,
                                              uint8_t *, size_t, size_t),
                             MDS_Arg_t arg)
{
    periph->callback = callback;
    periph->arg = arg;
}

MDS_Err_t DEV_XSPI_PeriphCommand(DEV_XSPI_Periph_t *periph, const DEV_XSPI_Command_t *cmd)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->command != NULL);

    const DEV_XSPI_Adaptr_t *xspi = periph->mount;

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)periph)) {
        return (MDS_EACCES);
    }

    MDS_Err_t err = xspi->driver->command(periph, cmd, NULL);

    return (err);
}

MDS_Err_t DEV_XSPI_PeriphPolling(DEV_XSPI_Periph_t *periph, const DEV_XSPI_Command_t *cmd,
                                 const DEV_XSPI_Polling_t *poll)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->command != NULL);

    const DEV_XSPI_Adaptr_t *xspi = periph->mount;

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)periph)) {
        return (MDS_EACCES);
    }

    MDS_Err_t err = xspi->driver->command(periph, cmd, poll);

    return (err);
}

MDS_Err_t DEV_XSPI_PeriphTransmit(DEV_XSPI_Periph_t *periph, const uint8_t *tx, size_t size)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->transmit != NULL);

    const DEV_XSPI_Adaptr_t *xspi = periph->mount;

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)periph)) {
        return (MDS_EACCES);
    }

    MDS_Tick_t optick = size / ((periph->config.clock / 0x0E) + 0x01);

    MDS_Err_t err = xspi->driver->transmit(periph, tx, size, MDS_TIMEOUT_TICKS(optick));

    return (err);
}

MDS_Err_t DEV_XSPI_PeriphReceive(DEV_XSPI_Periph_t *periph, uint8_t *rx, size_t size)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->recvice != NULL);

    const DEV_XSPI_Adaptr_t *xspi = periph->mount;

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)periph)) {
        return (MDS_EACCES);
    }

    MDS_Tick_t optick = size / ((periph->config.clock / 0x0E) + 0x01);

    MDS_Err_t err = xspi->driver->recvice(periph, rx, size, MDS_TIMEOUT_TICKS(optick));

    return (err);
}
