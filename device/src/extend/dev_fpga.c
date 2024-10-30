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
#include "extend/dev_fpga.h"

/* FPGA adaptr ------------------------------------------------------------- */
MDS_Err_t DEV_FPGA_AdaptrInit(DEV_FPGA_Adaptr_t *fpga, const char *name, const DEV_FPGA_Driver_t *driver,
                              MDS_DevHandle_t *handle, const MDS_Arg_t *init)
{
    return (MDS_DevAdaptrInit((MDS_DevAdaptr_t *)fpga, name, (const MDS_DevDriver_t *)driver, handle, init));
}

MDS_Err_t DEV_FPGA_AdaptrDeInit(DEV_FPGA_Adaptr_t *fpga)
{
    return (MDS_DevAdaptrDeInit((MDS_DevAdaptr_t *)fpga));
}

DEV_FPGA_Adaptr_t *DEV_FPGA_AdaptrCreate(const char *name, const DEV_FPGA_Driver_t *driver, const MDS_Arg_t *init)
{
    return ((DEV_FPGA_Adaptr_t *)MDS_DevAdaptrCreate(sizeof(DEV_FPGA_Adaptr_t), name, (const MDS_DevDriver_t *)driver,
                                                     init));
}

MDS_Err_t DEV_FPGA_AdaptrDestroy(DEV_FPGA_Adaptr_t *fpga)
{
    return (MDS_DevAdaptrDestroy((MDS_DevAdaptr_t *)fpga));
}

/* FPGA periph ------------------------------------------------------------- */
MDS_Err_t DEV_FPGA_PeriphInit(DEV_FPGA_Periph_t *periph, const char *name, DEV_FPGA_Adaptr_t *fpga)
{
    MDS_Err_t err = MDS_DevPeriphInit((MDS_DevPeriph_t *)periph, name, (MDS_DevAdaptr_t *)fpga);

    return (err);
}

MDS_Err_t DEV_FPGA_PeriphDeInit(DEV_FPGA_Periph_t *periph)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)periph));
}

DEV_FPGA_Periph_t *DEV_FPGA_PeriphCreate(const char *name, DEV_FPGA_Adaptr_t *fpga)
{
    DEV_FPGA_Periph_t *periph = (DEV_FPGA_Periph_t *)MDS_DevPeriphCreate(sizeof(DEV_FPGA_Periph_t), name,
                                                                         (MDS_DevAdaptr_t *)fpga);

    return (periph);
}

MDS_Err_t DEV_FPGA_PeriphDestroy(DEV_FPGA_Periph_t *periph)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)periph));
}

MDS_Err_t DEV_FPGA_PeriphOpen(DEV_FPGA_Periph_t *periph, MDS_Tick_t timeout)
{
    return (MDS_DevPeriphOpen((MDS_DevPeriph_t *)periph, timeout));
}

MDS_Err_t DEV_FPGA_PeriphClose(DEV_FPGA_Periph_t *periph)
{
    return (MDS_DevPeriphClose((MDS_DevPeriph_t *)periph));
}

MDS_Err_t DEV_FPGA_PeriphStart(DEV_FPGA_Periph_t *periph)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->start != NULL);

    const DEV_FPGA_Adaptr_t *fpga = periph->mount;

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    return (fpga->driver->start(periph));
}

MDS_Err_t DEV_FPGA_PeriphTransmit(DEV_FPGA_Periph_t *periph, const uint8_t *buff, size_t len)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->transmit != NULL);

    const DEV_FPGA_Adaptr_t *fpga = periph->mount;

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    MDS_Tick_t optick = (periph->object.optick > 0) ? (periph->object.optick)
                                                    : (len / ((periph->object.clock >> 0x0E) + 0x01));

    return (fpga->driver->transmit(periph, buff, len, optick));
}

MDS_Err_t DEV_FPGA_PeriphFinish(DEV_FPGA_Periph_t *periph)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->finish != NULL);

    const DEV_FPGA_Adaptr_t *fpga = periph->mount;

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    return (fpga->driver->finish(periph));
}
