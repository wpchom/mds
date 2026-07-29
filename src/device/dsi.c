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
#include "mds/device/dsi.h"

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

MDS_Err_t DEV_DSI_PeriphWorkModeConfig(DEV_DSI_Periph_t *periph, DEV_DSI_VirtualChannel_t vc,
                                       DEV_DSI_WorkMode_t workMode,
                                       const DEV_DSI_WorkModeConfig_t *modeCfg)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->workmode != NULL);

    const DEV_DSI_Adaptr_t *dsi = (periph->mount);

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)(periph))) {
        return (MDS_EACCES);
    }

    return (dsi->driver->workmode(periph, vc, workMode, modeCfg));
}

MDS_Err_t DEV_DSI_PeriphWriteDcs(DEV_DSI_Periph_t *periph, DEV_DSI_VirtualChannel_t vc, uint8_t cmd,
                                 const uint8_t *arg, size_t len)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->write != NULL);

    const DEV_DSI_Adaptr_t *dsi = (periph->mount);

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)(periph))) {
        return (MDS_EACCES);
    }

    if ((arg == NULL) || (len == 0)) {
        return (dsi->driver->write(periph, vc, DEV_DSI_DCS_SHORT_PKT_WRITE_P0, cmd, arg, len));
    } else if (len == sizeof(uint8_t)) {
        return (dsi->driver->write(periph, vc, DEV_DSI_DCS_SHORT_PKT_WRITE_P1, cmd, arg, len));
    } else {
        return (dsi->driver->write(periph, vc, DEV_DSI_DCS_LONG_PKT_WRITE, cmd, arg, len));
    }
}

MDS_Err_t DEV_DSI_PeriphReadDcs(DEV_DSI_Periph_t *periph, DEV_DSI_VirtualChannel_t vc, uint8_t cmd,
                                const uint8_t *arg, size_t len, uint8_t *rx, size_t size)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->read != NULL);

    const DEV_DSI_Adaptr_t *dsi = (periph->mount);

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)(periph))) {
        return (MDS_EACCES);
    }

    if ((arg == NULL) || (len == 0)) {
        return (dsi->driver->read(periph, vc, DEV_DSI_DCS_SHORT_PKT_READ_P0, cmd, arg, rx, size));
    } else if (len == sizeof(uint8_t)) {
        return (dsi->driver->read(periph, vc, DEV_DSI_DCS_SHORT_PKT_READ_P1, cmd, arg, rx, size));
    } else {
        return (MDS_EINVAL);
    }
}

MDS_Err_t DEV_DSI_PeriphWriteGen(DEV_DSI_Periph_t *periph, DEV_DSI_VirtualChannel_t vc,
                                 const uint8_t *tx, size_t len)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->write != NULL);

    const DEV_DSI_Adaptr_t *dsi = (periph->mount);

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)(periph))) {
        return (MDS_EACCES);
    }

    if ((tx == NULL) || (len == 0)) {
        return (dsi->driver->write(periph, vc, DEV_DSI_GEN_SHORT_PKT_WRITE_P0, 0, tx, len));
    } else if (len == sizeof(uint8_t)) {
        return (dsi->driver->write(periph, vc, DEV_DSI_GEN_SHORT_PKT_WRITE_P1, 0, tx, len));
    } else if (len == sizeof(uint16_t)) {
        return (dsi->driver->write(periph, vc, DEV_DSI_GEN_SHORT_PKT_WRITE_P2, 0, tx, len));
    } else {
        return (dsi->driver->write(periph, vc, DEV_DSI_GEN_LONG_PKT_WRITE, 0, tx, len));
    }
}

MDS_Err_t DEV_DSI_PeriphReadGen(DEV_DSI_Periph_t *periph, DEV_DSI_VirtualChannel_t vc,
                                const uint8_t *tx, size_t len, uint8_t *rx, size_t size)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->read != NULL);

    const DEV_DSI_Adaptr_t *dsi = (periph->mount);

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)(periph))) {
        return (MDS_EACCES);
    }

    if ((tx == NULL) || (len == 0)) {
        return (dsi->driver->read(periph, vc, DEV_DSI_GEN_SHORT_PKT_READ_P0, 0, tx, rx, size));
    } else if (len == sizeof(uint8_t)) {
        return (dsi->driver->read(periph, vc, DEV_DSI_GEN_SHORT_PKT_READ_P1, 0, tx, rx, size));
    } else if (len == sizeof(uint16_t)) {
        return (dsi->driver->read(periph, vc, DEV_DSI_GEN_SHORT_PKT_READ_P2, 0, tx, rx, size));
    } else {
        return (MDS_EINVAL);
    }
}

MDS_Err_t DEV_DSI_PeriphWriteDcsCmd(DEV_DSI_Periph_t *periph, DEV_DSI_VirtualChannel_t vc,
                                    const DEV_DSI_Command_t *cmd)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->write != NULL);
    MDS_ASSERT(cmd != NULL);

    const DEV_DSI_Adaptr_t *dsi = (periph->mount);

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)(periph))) {
        return (MDS_EACCES);
    }

    if ((cmd->arg == NULL) || (cmd->len == 0)) {
        return (dsi->driver->write(periph, vc, DEV_DSI_DCS_SHORT_PKT_WRITE_P0, cmd->cmd, cmd->arg,
                                   cmd->len));
    } else if (cmd->len == sizeof(uint8_t)) {
        return (dsi->driver->write(periph, vc, DEV_DSI_DCS_SHORT_PKT_WRITE_P1, cmd->cmd, cmd->arg,
                                   cmd->len));
    } else {
        return (dsi->driver->write(periph, vc, DEV_DSI_DCS_LONG_PKT_WRITE, cmd->cmd, cmd->arg,
                                   cmd->len));
    }
}

MDS_Err_t DEV_DSI_PeriphReadDcsCmd(DEV_DSI_Periph_t *periph, DEV_DSI_VirtualChannel_t vc,
                                   const DEV_DSI_Command_t *cmd, uint8_t *rx, size_t size)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->read != NULL);
    MDS_ASSERT(cmd != NULL);

    const DEV_DSI_Adaptr_t *dsi = (periph->mount);

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)(periph))) {
        return (MDS_EACCES);
    }

    if ((cmd->arg == NULL) || (cmd->len == 0)) {
        return (dsi->driver->read(periph, vc, DEV_DSI_DCS_SHORT_PKT_READ_P0, cmd->cmd, cmd->arg, rx,
                                  size));
    } else if (cmd->len == sizeof(uint8_t)) {
        return (dsi->driver->read(periph, vc, DEV_DSI_DCS_SHORT_PKT_READ_P1, cmd->cmd, cmd->arg, rx,
                                  size));
    } else {
        return (MDS_EINVAL);
    }
}

MDS_Err_t DEV_DSI_PeriphWriteGenCmd(DEV_DSI_Periph_t *periph, DEV_DSI_VirtualChannel_t vc,
                                    const DEV_DSI_Command_t *cmd)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->write != NULL);

    const DEV_DSI_Adaptr_t *dsi = (periph->mount);

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)(periph))) {
        return (MDS_EACCES);
    }

    if ((cmd == NULL) || (cmd->arg == NULL) || (cmd->len == 0)) {
        return (
            dsi->driver->write(periph, vc, DEV_DSI_GEN_SHORT_PKT_WRITE_P0, 0, cmd->arg, cmd->len));
    } else if (cmd->len == sizeof(uint8_t)) {
        return (
            dsi->driver->write(periph, vc, DEV_DSI_GEN_SHORT_PKT_WRITE_P1, 0, cmd->arg, cmd->len));
    } else if (cmd->len == sizeof(uint16_t)) {
        return (
            dsi->driver->write(periph, vc, DEV_DSI_GEN_SHORT_PKT_WRITE_P2, 0, cmd->arg, cmd->len));
    } else {
        return (dsi->driver->write(periph, vc, DEV_DSI_GEN_LONG_PKT_WRITE, 0, cmd->arg, cmd->len));
    }
}

MDS_Err_t DEV_DSI_PeriphReadGenCmd(DEV_DSI_Periph_t *periph, DEV_DSI_VirtualChannel_t vc,
                                   const DEV_DSI_Command_t *cmd, uint8_t *rx, size_t size)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->read != NULL);

    const DEV_DSI_Adaptr_t *dsi = (periph->mount);

    if (!MDS_DevPeriphIsAccessable((MDS_DevPeriph_t *)(periph))) {
        return (MDS_EACCES);
    }

    if ((cmd == NULL) || (cmd->arg == NULL) || (cmd->len == 0)) {
        return (
            dsi->driver->read(periph, vc, DEV_DSI_GEN_SHORT_PKT_READ_P0, 0, cmd->arg, rx, size));
    } else if (cmd->len == sizeof(uint8_t)) {
        return (
            dsi->driver->read(periph, vc, DEV_DSI_GEN_SHORT_PKT_READ_P1, 0, cmd->arg, rx, size));
    } else if (cmd->len == sizeof(uint16_t)) {
        return (
            dsi->driver->read(periph, vc, DEV_DSI_GEN_SHORT_PKT_READ_P2, 0, cmd->arg, rx, size));
    } else {
        return (MDS_EINVAL);
    }
}
