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
#include "dev_i2c.h"

/* I2C adaptr -------------------------------------------------------------- */
MDS_Err_t DEV_I2C_AdaptrInit(DEV_I2C_Adaptr_t *i2c, const char *name, const DEV_I2C_Driver_t *driver,
                             MDS_DevHandle_t *handle, const MDS_Arg_t *init)
{
    return (MDS_DevAdaptrInit((MDS_DevAdaptr_t *)i2c, name, (const MDS_DevDriver_t *)driver, handle, init));
}

MDS_Err_t DEV_I2C_AdaptrDeInit(DEV_I2C_Adaptr_t *i2c)
{
    return (MDS_DevAdaptrDeInit((MDS_DevAdaptr_t *)i2c));
}

DEV_I2C_Adaptr_t *DEV_I2C_AdaptrCreate(const char *name, const DEV_I2C_Driver_t *driver, const MDS_Arg_t *init)
{
    return ((DEV_I2C_Adaptr_t *)MDS_DevAdaptrCreate(sizeof(DEV_I2C_Adaptr_t), name, (const MDS_DevDriver_t *)driver,
                                                    init));
}

MDS_Err_t DEV_I2C_AdaptrDestroy(DEV_I2C_Adaptr_t *i2c)
{
    return (MDS_DevAdaptrDestroy((MDS_DevAdaptr_t *)i2c));
}

/* I2C periph -------------------------------------------------------------- */
MDS_Err_t DEV_I2C_PeriphInit(DEV_I2C_Periph_t *periph, const char *name, DEV_I2C_Adaptr_t *i2c)
{
    MDS_Err_t err = MDS_DevPeriphInit((MDS_DevPeriph_t *)periph, name, (MDS_DevAdaptr_t *)i2c);
    if (err == MDS_EOK) {
        periph->object.optick = MDS_DEVICE_PERIPH_TIMEOUT;
    }
    return (err);
}

MDS_Err_t DEV_I2C_PeriphDeInit(DEV_I2C_Periph_t *periph)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)periph));
}

DEV_I2C_Periph_t *DEV_I2C_PeriphCreate(const char *name, DEV_I2C_Adaptr_t *i2c)
{
    DEV_I2C_Periph_t *periph = (DEV_I2C_Periph_t *)MDS_DevPeriphCreate(sizeof(DEV_I2C_Periph_t), name,
                                                                       (MDS_DevAdaptr_t *)i2c);
    if (periph != NULL) {
        periph->object.optick = MDS_DEVICE_PERIPH_TIMEOUT;
    }

    return (periph);
}

MDS_Err_t DEV_I2C_PeriphDestroy(DEV_I2C_Periph_t *periph)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)periph));
}

MDS_Err_t DEV_I2C_PeriphOpen(DEV_I2C_Periph_t *periph, MDS_Tick_t timeout)
{
    return (MDS_DevPeriphOpen((MDS_DevPeriph_t *)periph, timeout));
}

MDS_Err_t DEV_I2C_PeriphClose(DEV_I2C_Periph_t *periph)
{
    return (MDS_DevPeriphClose((MDS_DevPeriph_t *)periph));
}

void DEV_I2C_PeriphCallback(DEV_I2C_Periph_t *periph,
                            void (*callback)(const DEV_I2C_Periph_t *, MDS_Arg_t *, uint8_t *, size_t), MDS_Arg_t *arg)
{
    periph->callback = callback;
    periph->arg = arg;
}

MDS_Err_t DEV_I2C_PeriphSlaveReceive(DEV_I2C_Periph_t *periph, uint8_t *buff, size_t size, MDS_Tick_t timeout)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->slave != NULL);

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    return (periph->mount->driver->slave(periph, true, buff, size, timeout));
}

MDS_Err_t DEV_I2C_PeriphSlaveTransmit(DEV_I2C_Periph_t *periph, const uint8_t *buff, size_t len, MDS_Tick_t timeout)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->slave != NULL);

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    return (periph->mount->driver->slave(periph, false, (uint8_t *)buff, len, timeout));
}

MDS_Err_t DEV_I2C_PeriphMasterTransfer(DEV_I2C_Periph_t *periph, const DEV_I2C_Msg_t msg[], size_t num)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->master != NULL);

    MDS_Err_t err = MDS_EINVAL;
    const DEV_I2C_Adaptr_t *i2c = periph->mount;

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    for (size_t retry = 0; (err != MDS_EOK) && (retry <= periph->object.retry); retry++) {
        for (size_t cnt = 0; cnt < num; cnt++) {
            err = i2c->driver->master(periph, &(msg[cnt]));
            if (err != MDS_EOK) {
                break;
            }
        }
    }

    return (err);
}

MDS_Err_t DEV_I2C_PeriphMasterTransmit(DEV_I2C_Periph_t *periph, const uint8_t *buff, size_t len)
{
    DEV_I2C_Msg_t msg[] = {
        {.flags = DEV_I2C_MSGFLAG_WR, .buff = (uint8_t *)buff, .len = len},
    };

    return (DEV_I2C_PeriphMasterTransfer(periph, msg, ARRAY_SIZE(msg)));
}

MDS_Err_t DEV_I2C_PeriphMasterReceive(DEV_I2C_Periph_t *periph, uint8_t *buff, size_t size)
{
    DEV_I2C_Msg_t msg[] = {
        {.flags = DEV_I2C_MSGFLAG_RD, .buff = buff, .len = size},
    };

    return (DEV_I2C_PeriphMasterTransfer(periph, msg, ARRAY_SIZE(msg)));
}

MDS_Err_t DEV_I2C_PeriphMasterWriteMem(DEV_I2C_Periph_t *periph, uint32_t memAddr, uint8_t memAddrSz,
                                       const uint8_t *buff, size_t len)
{
    MDS_ASSERT(memAddrSz <= sizeof(uint32_t));

    size_t idx;
    uint8_t reg[sizeof(uint32_t)];

    for (idx = 0; idx < memAddrSz; idx++) {
        reg[idx] = (uint8_t)(memAddr >> (MDS_BITS_OF_BYTE * (memAddrSz - idx - 1)));
    }
    DEV_I2C_Msg_t msg[] = {
        {.flags = DEV_I2C_MSGFLAG_WR | DEV_I2C_MSGFLAG_NO_STOP, .buff = reg, .len = memAddrSz},
        {.flags = DEV_I2C_MSGFLAG_WR | DEV_I2C_MSGFLAG_NO_START, .buff = (uint8_t *)buff, .len = len},
    };

    return (DEV_I2C_PeriphMasterTransfer(periph, msg, ARRAY_SIZE(msg)));
}

MDS_Err_t DEV_I2C_PeriphMasterReadMem(DEV_I2C_Periph_t *periph, uint32_t memAddr, uint8_t memAddrSz, uint8_t *buff,
                                      size_t len)
{
    MDS_ASSERT(memAddrSz <= sizeof(uint32_t));

    size_t idx;
    uint8_t reg[sizeof(uint32_t)];

    for (idx = 0; idx < memAddrSz; idx++) {
        reg[idx] = (uint8_t)(memAddr >> (MDS_BITS_OF_BYTE * (memAddrSz - idx - 1)));
    }
    DEV_I2C_Msg_t msg[] = {
        {.flags = DEV_I2C_MSGFLAG_WR | DEV_I2C_MSGFLAG_NO_STOP, .buff = reg, .len = memAddrSz},
        {.flags = DEV_I2C_MSGFLAG_RD, .buff = buff, .len = len},
    };

    return (DEV_I2C_PeriphMasterTransfer(periph, msg, ARRAY_SIZE(msg)));
}

MDS_Err_t DEV_I2C_PeriphMasterModifyMem(DEV_I2C_Periph_t *periph, uint32_t memAddr, uint32_t memAddrSz, uint8_t *buff,
                                        size_t len, const uint8_t *clr, const uint8_t *set)
{
    MDS_ASSERT(memAddrSz <= sizeof(uint32_t));

    MDS_Err_t err = DEV_I2C_PeriphMasterReadMem(periph, memAddr, memAddrSz, buff, len);
    if (err == MDS_EOK) {
        for (size_t idx = 0; idx < len; idx++) {
            if (clr != NULL) {
                buff[idx] &= (uint8_t)(~clr[idx]);
            }
            if (set != NULL) {
                buff[idx] |= (uint8_t)(set[idx]);
            }
        }
        err = DEV_I2C_PeriphMasterWriteMem(periph, memAddr, memAddrSz, buff, len);
    }

    return (err);
}
