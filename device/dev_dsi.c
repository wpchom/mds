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
#include "dev_dsi.h"

/* DSI adaptr -------------------------------------------------------------- */
MDS_Err_t DEV_DSI_AdaptrInit(DEV_DSI_Adaptr_t *dsi, const char *name,
                             const DEV_DSI_Driver_t *driver, MDS_DevHandle_t handle, MDS_Arg_t init)
{
    return (MDS_DevAdaptrInit((MDS_DevAdaptr_t *)dsi, name, (const MDS_DevDriver_t *)driver, handle,
                              init));
}

MDS_Err_t DEV_DSI_AdaptrDeInit(DEV_DSI_Adaptr_t *dsi)
{
    return (MDS_DevAdaptrDeInit((MDS_DevAdaptr_t *)dsi));
}

DEV_DSI_Adaptr_t *DEV_DSI_AdaptrCreate(const char *name, const DEV_DSI_Driver_t *driver,
                                       MDS_Arg_t init)
{
    return ((DEV_DSI_Adaptr_t *)MDS_DevAdaptrCreate(sizeof(DEV_DSI_Adaptr_t), name,
                                                    (const MDS_DevDriver_t *)driver, init));
}

MDS_Err_t DEV_DSI_AdaptrDestroy(DEV_DSI_Adaptr_t *dsi)
{
    return (MDS_DevAdaptrDestroy((MDS_DevAdaptr_t *)dsi));
}

MDS_Err_t DEV_DSI_AdaptrPhyInit(DEV_DSI_Adaptr_t *dsi, const DEV_DSI_PhyConfig_t *phyCfg)
{
    MDS_ASSERT(dsi != NULL);
    MDS_ASSERT(phyCfg != NULL);

    MDS_Err_t err = MDS_EIO;

    if ((dsi->driver != NULL) && (dsi->driver->control != NULL)) {
        err = dsi->driver->control(dsi, DEV_DSI_CMD_PHY_INIT, MDS_ARG_WITH(phyCfg));
    }

    if (MDS_ErrIsSame(err, MDS_EOK)) {
        dsi->phyCfg = *phyCfg;
    }

    return (err);
}

MDS_Err_t DEV_DSI_AdaptrEnterULPM(DEV_DSI_Adaptr_t *dsi)
{
    MDS_ASSERT(dsi != NULL);

    if ((dsi->driver != NULL) && (dsi->driver->control != NULL)) {
        return (dsi->driver->control(dsi, DEV_DSI_CMD_ENTER_ULPM, MDS_ARG_WITH(NULL)));
    }

    return (MDS_EIO);
}

MDS_Err_t DEV_DSI_AdaptrExitULPM(DEV_DSI_Adaptr_t *dsi)
{
    MDS_ASSERT(dsi != NULL);

    if ((dsi->driver != NULL) && (dsi->driver->control != NULL)) {
        return (dsi->driver->control(dsi, DEV_DSI_CMD_EXIT_ULPM, MDS_ARG_WITH(NULL)));
    }

    return (MDS_EIO);
}

MDS_Err_t DEV_DSI_AdaptrEnterULPMData(DEV_DSI_Adaptr_t *dsi)
{
    MDS_ASSERT(dsi != NULL);

    if ((dsi->driver != NULL) && (dsi->driver->control != NULL)) {
        return (dsi->driver->control(dsi, DEV_DSI_CMD_ENTER_ULPM_DATA, MDS_ARG_WITH(NULL)));
    }

    return (MDS_EIO);
}

MDS_Err_t DEV_DSI_AdaptrExitULPMData(DEV_DSI_Adaptr_t *dsi)
{
    MDS_ASSERT(dsi != NULL);

    if ((dsi->driver != NULL) && (dsi->driver->control != NULL)) {
        return (dsi->driver->control(dsi, DEV_DSI_CMD_EXIT_ULPM_DATA, MDS_ARG_WITH(NULL)));
    }

    return (MDS_EIO);
}

/* DSI periph -------------------------------------------------------------- */
MDS_Err_t DEV_DSI_PeriphInit(DEV_DSI_Periph_t *periph, const char *name, DEV_DSI_Adaptr_t *dsi)
{
    MDS_Err_t err = MDS_DevPeriphInit((MDS_DevPeriph_t *)periph, name, (MDS_DevAdaptr_t *)dsi);

    return (err);
}

MDS_Err_t DEV_DSI_PeriphDeInit(DEV_DSI_Periph_t *periph)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)periph));
}

DEV_DSI_Periph_t *DEV_DSI_PeriphCreate(const char *name, DEV_DSI_Adaptr_t *dsi)
{
    DEV_DSI_Periph_t *periph = (DEV_DSI_Periph_t *)MDS_DevPeriphCreate(
        sizeof(DEV_DSI_Periph_t), name, (MDS_DevAdaptr_t *)dsi);

    return (periph);
}

MDS_Err_t DEV_DSI_PeriphDestroy(DEV_DSI_Periph_t *periph)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)periph));
}

MDS_Err_t DEV_DSI_PeriphOpen(DEV_DSI_Periph_t *periph, MDS_Timeout_t timeout)
{
    return (MDS_DevPeriphOpen((MDS_DevPeriph_t *)periph, timeout));
}

MDS_Err_t DEV_DSI_PeriphClose(DEV_DSI_Periph_t *periph)
{
    return (MDS_DevPeriphClose((MDS_DevPeriph_t *)periph));
}

MDS_Err_t DEV_DSI_PeriphRefresh(DEV_DSI_Periph_t *periph)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->control != NULL);

    const DEV_DSI_Adaptr_t *dsi = (periph->mount);

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)(periph))) {
        return (MDS_EACCES);
    }

    return (dsi->driver->control(dsi, DEV_DSI_CMD_REFRESH, MDS_ARG_WITH(NULL)));
}

MDS_Err_t DEV_DSI_PeriphWrite(DEV_DSI_Periph_t *periph, DEV_DSI_DataType_t type, const uint8_t *tx,
                              size_t nums)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->write != NULL);

    const DEV_DSI_Adaptr_t *dsi = (periph->mount);

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)(periph))) {
        return (MDS_EACCES);
    }

    return (dsi->driver->write(periph, type, tx, nums));
}

MDS_Err_t DEV_DSI_PeriphRead(DEV_DSI_Periph_t *periph, DEV_DSI_DataType_t type, const uint8_t *tx,
                             uint8_t *rx, size_t nums)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->read != NULL);

    const DEV_DSI_Adaptr_t *dsi = (periph->mount);

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)(periph))) {
        return (MDS_EACCES);
    }

    return (dsi->driver->read(periph, type, tx, rx, nums));
}
