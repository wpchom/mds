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
#include "drv_i2c_simulate.h"

/* Define ------------------------------------------------------------------ */
#define I2C_DATA_BYTE_MSB 0x80U
#define I2C_DATA_BYTE_LSB 0x01U

/* Function ---------------------------------------------------------------- */
static void I2C_SimulateDirect(DEV_GPIO_Pin_t *sda, DEV_GPIO_Mode_t mode)
{
    const DEV_GPIO_Config_t sdaConfig = {
        .mode = mode,
        .type = DEV_GPIO_TYPE_OD,
        .alternate = 0,
    };

    DEV_GPIO_PinConfig(sda, &sdaConfig);
}

static void I2C_SimulateStart(DRV_I2C_SimulateHandle_t *hi2c)
{
    I2C_SimulateDirect(hi2c->sda, DEV_GPIO_MODE_OUTPUT);

    MDS_Item_t lock = MDS_CoreInterruptLock();

    DEV_GPIO_PinHigh(hi2c->sda);
    DEV_GPIO_PinHigh(hi2c->scl);
    MDS_SysCountDelay(hi2c->delay);
    DEV_GPIO_PinLow(hi2c->sda);
    MDS_SysCountDelay(hi2c->delay);
    DEV_GPIO_PinLow(hi2c->scl);
    MDS_SysCountDelay(hi2c->delay);

    MDS_CoreInterruptRestore(lock);
}

static void I2C_SimulateStop(DRV_I2C_SimulateHandle_t *hi2c)
{
    I2C_SimulateDirect(hi2c->sda, DEV_GPIO_MODE_OUTPUT);

    MDS_Item_t lock = MDS_CoreInterruptLock();

    DEV_GPIO_PinLow(hi2c->sda);
    DEV_GPIO_PinHigh(hi2c->scl);
    MDS_SysCountDelay(hi2c->delay);
    DEV_GPIO_PinHigh(hi2c->sda);

    MDS_CoreInterruptRestore(lock);
}

static void I2C_SimulateAck(DRV_I2C_SimulateHandle_t *hi2c)
{
    I2C_SimulateDirect(hi2c->sda, DEV_GPIO_MODE_OUTPUT);

    MDS_Item_t lock = MDS_CoreInterruptLock();

    DEV_GPIO_PinLow(hi2c->sda);
    MDS_SysCountDelay(hi2c->delay);
    DEV_GPIO_PinHigh(hi2c->scl);
    MDS_SysCountDelay(hi2c->delay);
    DEV_GPIO_PinLow(hi2c->scl);
    MDS_SysCountDelay(hi2c->delay);
    DEV_GPIO_PinHigh(hi2c->sda);

    MDS_CoreInterruptRestore(lock);
}

static void I2C_SimulateNack(DRV_I2C_SimulateHandle_t *hi2c)
{
    I2C_SimulateDirect(hi2c->sda, DEV_GPIO_MODE_OUTPUT);

    MDS_Item_t lock = MDS_CoreInterruptLock();

    DEV_GPIO_PinHigh(hi2c->sda);
    MDS_SysCountDelay(hi2c->delay);
    DEV_GPIO_PinHigh(hi2c->scl);
    MDS_SysCountDelay(hi2c->delay);
    DEV_GPIO_PinLow(hi2c->scl);
    MDS_SysCountDelay(hi2c->delay);

    MDS_CoreInterruptRestore(lock);
}

static void I2C_SimulateWriteByte(DRV_I2C_SimulateHandle_t *hi2c, uint8_t dat)
{
    MDS_Item_t lock = MDS_CoreInterruptLock();

    for (size_t cnt = 0; cnt < MDS_BITS_OF_BYTE; cnt++) {
        MDS_SysCountDelay(hi2c->delay);
        if ((dat & I2C_DATA_BYTE_MSB) != 0U) {
            DEV_GPIO_PinHigh(hi2c->sda);
        } else {
            DEV_GPIO_PinLow(hi2c->sda);
        }
        MDS_SysCountDelay(hi2c->delay);
        DEV_GPIO_PinHigh(hi2c->scl);
        MDS_SysCountDelay(hi2c->delay);
        DEV_GPIO_PinLow(hi2c->scl);
        dat <<= 1;
    }
    DEV_GPIO_PinHigh(hi2c->sda);
    MDS_CoreInterruptRestore(lock);
}

static uint8_t I2C_SimulateReadByte(DRV_I2C_SimulateHandle_t *hi2c)
{
    uint8_t value = 0x00U;
    MDS_Item_t lock = MDS_CoreInterruptLock();

    for (size_t cnt = 0; cnt < MDS_BITS_OF_BYTE; cnt++) {
        MDS_SysCountDelay(hi2c->delay);
        DEV_GPIO_PinHigh(hi2c->scl);
        MDS_SysCountDelay(hi2c->delay);
        value <<= 1;
        if (DEV_GPIO_PinRead(hi2c->sda) == DEV_GPIO_LEVEL_HIGH) {
            value |= I2C_DATA_BYTE_LSB;
        }
        DEV_GPIO_PinLow(hi2c->scl);
    }
    MDS_CoreInterruptRestore(lock);

    return value;
}

static DEV_GPIO_Level_t I2C_SimulateWaitAck(DRV_I2C_SimulateHandle_t *hi2c, MDS_Tick_t tickstart, MDS_Tick_t timeout)
{
    DEV_GPIO_Level_t ret;

    DEV_GPIO_PinHigh(hi2c->sda);
    MDS_SysCountDelay(hi2c->delay);
    DEV_GPIO_PinHigh(hi2c->scl);
    MDS_SysCountDelay(hi2c->delay);

    I2C_SimulateDirect(hi2c->sda, DEV_GPIO_MODE_INPUT);
    do {
        ret = DEV_GPIO_PinRead(hi2c->sda);
        if (ret == DEV_GPIO_LEVEL_LOW) {
            break;
        }
    } while (!((MDS_SysTickGetCount() - tickstart) > timeout));

    DEV_GPIO_PinLow(hi2c->scl);
    MDS_SysCountDelay(hi2c->delay);

    return (ret);
}

static MDS_Err_t I2C_SimulateStartAddr(DRV_I2C_SimulateHandle_t *hi2c, MDS_Tick_t tickstart,
                                       const DEV_I2C_Object_t *obj, DEV_I2C_Msg_t *msg)
{
    if ((msg->flags & DEV_I2C_MSGFLAG_NO_START) != 0U) {
        return (MDS_EOK);
    }

    I2C_SimulateStart(hi2c);

    if (obj->devAddrBit == DEV_I2C_DEVADDRBITS_10) {
        // 10bit
    } else {
        uint8_t devAddr = (obj->devAddress << 1) & 0xFFU;
        if ((msg->flags & DEV_I2C_MSGFLAG_RD) != 0U) {
            devAddr |= 0x01U;
        }
        I2C_SimulateWriteByte(hi2c, devAddr);
        if (I2C_SimulateWaitAck(hi2c, tickstart, obj->timeout) == DEV_GPIO_LEVEL_HIGH) {
            return (MDS_ETIME);
        }
    }

    return (MDS_EOK);
}

static MDS_Err_t I2C_SimulateRead(DRV_I2C_SimulateHandle_t *hi2c, MDS_Tick_t tickstart, const DEV_I2C_Object_t *obj,
                                  DEV_I2C_Msg_t *msg)
{
    uint8_t *buff = (uint8_t *)(msg->buff);

    I2C_SimulateDirect(hi2c->sda, DEV_GPIO_MODE_INPUT);

    for (size_t cnt = 0; cnt < (msg->len - 1); cnt++) {
        buff[cnt] = I2C_SimulateReadByte(hi2c);
        if ((msg->flags & DEV_I2C_MSGFLAG_NOREAD_ACK) == 0U) {
            I2C_SimulateAck(hi2c);
        } else {
            I2C_SimulateNack(hi2c);
        }
    }
    buff[msg->len - 1] = I2C_SimulateReadByte(hi2c);
    I2C_SimulateNack(hi2c);
    if ((msg->flags & DEV_I2C_MSGFLAG_NO_STOP) == 0U) {
        I2C_SimulateStop(hi2c);
    }

    return (MDS_EOK);
}

static MDS_Err_t I2C_SimulateWrite(DRV_I2C_SimulateHandle_t *hi2c, MDS_Tick_t tickstart, const DEV_I2C_Object_t *obj,
                                   DEV_I2C_Msg_t *msg)
{
    const uint8_t *buff = msg->buff;

    I2C_SimulateDirect(hi2c->sda, DEV_GPIO_MODE_OUTPUT);

    for (size_t cnt = 0; cnt < msg->len; cnt++) {
        I2C_SimulateWriteByte(hi2c, buff[cnt]);
        if ((msg->flags & DEV_I2C_MSGFLAG_IGNORE_ACK) != 0U) {
            (void)I2C_SimulateWaitAck(hi2c, tickstart, 0);
        } else {
            if (I2C_SimulateWaitAck(hi2c, tickstart, obj->timeout) == DEV_GPIO_LEVEL_HIGH) {
                return (MDS_EIO);
            }
        }
    }

    if ((msg->flags & DEV_I2C_MSGFLAG_NO_STOP) == 0U) {
        I2C_SimulateStop(hi2c);
    }
    return (MDS_EOK);
}

MDS_Err_t DRV_I2C_SimulateInit(DRV_I2C_SimulateHandle_t *hi2c)
{
    MDS_ASSERT(hi2c != NULL);

    static const DEV_GPIO_Config_t gpioConfig = {
        .mode = DEV_GPIO_MODE_OUTPUT,
        .type = DEV_GPIO_TYPE_OD,
        .alternate = 0,
    };

    DEV_GPIO_PinConfig(hi2c->scl, &gpioConfig);
    DEV_GPIO_PinConfig(hi2c->sda, &gpioConfig);

    return (MDS_EOK);
}

MDS_Err_t DRV_I2C_SimulateDeInit(DRV_I2C_SimulateHandle_t *hi2c)
{
    MDS_ASSERT(hi2c != NULL);

    DEV_GPIO_PinDeInit(hi2c->sda);
    DEV_GPIO_PinDeInit(hi2c->scl);

    return (MDS_EOK);
}

MDS_Err_t DRV_I2C_SimulateTransfer(DRV_I2C_SimulateHandle_t *hi2c, const DEV_I2C_Object_t *obj, DEV_I2C_Msg_t *msg)
{
    MDS_Err_t err;
    MDS_Tick_t tickstart = MDS_SysTickGetCount();

    err = I2C_SimulateStartAddr(hi2c, tickstart, obj, msg);
    if (err != MDS_EOK) {
        return (err);
    }

    if ((msg->flags & DEV_I2C_MSGFLAG_RD) != 0U) {
        return (I2C_SimulateRead(hi2c, tickstart, obj, msg));
    } else {
        return (I2C_SimulateWrite(hi2c, tickstart, obj, msg));
    }
}

MDS_Err_t DRV_I2C_SimulateTransferAbort(DRV_I2C_SimulateHandle_t *hi2c)
{
    MDS_ASSERT(hi2c != NULL);

    I2C_SimulateReadByte(hi2c);
    I2C_SimulateWaitAck(hi2c, 0, 0);
    I2C_SimulateStop(hi2c);

    return (MDS_EOK);
}

/* Driver ------------------------------------------------------------------ */
static MDS_Err_t DDRV_I2C_Control(const DEV_I2C_Adaptr_t *i2c, MDS_Item_t cmd, MDS_Arg_t *arg)
{
    DRV_I2C_SimulateHandle_t *hi2c = (DRV_I2C_SimulateHandle_t *)(i2c->handle);

    switch (cmd) {
        case MDS_DEVICE_CMD_INIT:
            return (DRV_I2C_SimulateInit(hi2c));
        case MDS_DEVICE_CMD_DEINIT:
            return (DRV_I2C_SimulateDeInit(hi2c));
        case MDS_DEVICE_CMD_HANDLESZ:
            MDS_DEVICE_ARG_HANDLE_SIZE(arg, DRV_I2C_SimulateHandle_t);
            return (MDS_EOK);
        case MDS_DEVICE_CMD_OPEN:
        case MDS_DEVICE_CMD_CLOSE:
            return (MDS_EOK);
        default:
            break;
    }

    return (MDS_EPERM);
}

static MDS_Err_t DDRV_I2C_Transfer(const DEV_I2C_Periph_t *periph, DEV_I2C_Msg_t *msg)
{
    DRV_I2C_SimulateHandle_t *hi2c = (DRV_I2C_SimulateHandle_t *)(periph->mount->handle);

    return (DRV_I2C_SimulateTransfer(hi2c, &(periph->object), msg));
}

const DEV_I2C_Driver_t G_DRV_I2C_SIMULATE = {
    .control = DDRV_I2C_Control,
    .transfer = DDRV_I2C_Transfer,
};
