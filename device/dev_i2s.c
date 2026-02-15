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
#include "dev_i2s.h"

/* I2S adaptr -------------------------------------------------------------- */
MDS_Err_t DEV_I2S_AdaptrInit(DEV_I2S_Adaptr_t *i2s, const char *name,
                             const DEV_I2S_Driver_t *driver, MDS_DevHandle_t *handle,
                             MDS_Arg_t init)
{
    return (MDS_DevAdaptrInit((MDS_DevAdaptr_t *)i2s, name, (const MDS_DevDriver_t *)driver, handle,
                              init));
}

MDS_Err_t DEV_I2S_AdaptrDeInit(DEV_I2S_Adaptr_t *i2s)
{
    return (MDS_DevAdaptrDeInit((MDS_DevAdaptr_t *)i2s));
}

DEV_I2S_Adaptr_t *DEV_I2S_AdaptrCreate(const char *name, const DEV_I2S_Driver_t *driver,
                                       MDS_Arg_t init)
{
    return ((DEV_I2S_Adaptr_t *)MDS_DevAdaptrCreate(sizeof(DEV_I2S_Adaptr_t), name,
                                                    (const MDS_DevDriver_t *)driver, init));
}

MDS_Err_t DEV_I2S_AdaptrDestroy(DEV_I2S_Adaptr_t *i2s)
{
    return (MDS_DevAdaptrDestroy((MDS_DevAdaptr_t *)i2s));
}

/* I2S periph -------------------------------------------------------------- */
MDS_Err_t DEV_I2S_PeriphInit(DEV_I2S_Periph_t *periph, const char *name, DEV_I2S_Adaptr_t *i2s)
{
    MDS_Err_t err = MDS_DevPeriphInit((MDS_DevPeriph_t *)periph, name, (MDS_DevAdaptr_t *)i2s);

    return (err);
}

MDS_Err_t DEV_I2S_PeriphDeInit(DEV_I2S_Periph_t *periph)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)periph));
}

DEV_I2S_Periph_t *DEV_I2S_PeriphCreate(const char *name, DEV_I2S_Adaptr_t *i2s)
{
    DEV_I2S_Periph_t *periph = (DEV_I2S_Periph_t *)MDS_DevPeriphCreate(
        sizeof(DEV_I2S_Periph_t), name, (MDS_DevAdaptr_t *)i2s);

    return (periph);
}

MDS_Err_t DEV_I2S_PeriphDestroy(DEV_I2S_Periph_t *periph)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)periph));
}

MDS_Err_t DEV_I2S_PeriphOpen(DEV_I2S_Periph_t *periph, MDS_Timeout_t timeout)
{
    return (MDS_DevPeriphOpen((MDS_DevPeriph_t *)periph, timeout));
}

MDS_Err_t DEV_I2S_PeriphClose(DEV_I2S_Periph_t *periph)
{
    return (MDS_DevPeriphClose((MDS_DevPeriph_t *)periph));
}

void DEV_I2S_PeriphTxCallback(DEV_I2S_Periph_t *periph,
                              void (*callback)(DEV_I2S_Periph_t *, MDS_Arg_t , const uint8_t *,
                                               size_t, size_t),
                              MDS_Arg_t arg)
{
    MDS_ASSERT(periph != NULL);

    periph->txCallback = callback;
    periph->txArg = arg;
}

void DEV_I2S_PeriphRxCallback(DEV_I2S_Periph_t *periph,
                              void (*callback)(DEV_I2S_Periph_t *, MDS_Arg_t , uint8_t *, size_t,
                                               size_t),
                              MDS_Arg_t arg)
{
    MDS_ASSERT(periph != NULL);

    periph->rxCallback = callback;
    periph->rxArg = arg;
}

MDS_Err_t DEV_I2S_PeriphTransmit(DEV_I2S_Periph_t *periph, const uint8_t *buff, size_t size)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->transmit != NULL);

    MDS_Err_t err = MDS_EINVAL;
    const DEV_I2S_Adaptr_t *i2s = periph->mount;

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)periph)) {
        return (MDS_EACCES);
    }

    MDS_Tick_t optick = (periph->object.timeout.ticks > 0)
        ? (periph->object.timeout.ticks)
        : (size / ((periph->config.audioFreq >> 0x0E) + 0x01));
    err = i2s->driver->transmit(periph, buff, size, MDS_TIMEOUT_TICKS(optick));

    return (err);
}

MDS_Err_t DEV_I2S_PeriphReceive(DEV_I2S_Periph_t *periph, uint8_t *buff, size_t size, size_t *recv,
                                MDS_Timeout_t timeout)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->receive != NULL);

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)periph)) {
        return (MDS_EACCES);
    }

    return (periph->mount->driver->receive(periph, buff, size, recv, timeout));
}
