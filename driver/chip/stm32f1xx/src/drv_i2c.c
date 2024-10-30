/**
 * @copyright   Copyright (c) 2024 Pchom & licensed under Mulan PSL v2
 * @file        drv_i2c.c
 * @brief       stm32f1xx i2c driver for mds device
 * @date        2024-05-31
 */
/* Include ----------------------------------------------------------------- */
#include "drv_i2c.h"
#include "drv_chip.h"

/* Function ---------------------------------------------------------------- */
MDS_Err_t DRV_I2C_Init(DRV_I2C_Handle_t *hi2c, I2C_TypeDef *I2Cx)
{
    MDS_ASSERT(hi2c != NULL);

    hi2c->handle.Instance = I2Cx;

    return (MDS_SemaphoreInit(&(hi2c->sem), "i2c", 0, 1));
}

MDS_Err_t DRV_I2C_DeInit(DRV_I2C_Handle_t *hi2c)
{
    MDS_ASSERT(hi2c != NULL);

    return (MDS_SemaphoreDeInit(&(hi2c->sem)));
}

MDS_Err_t DRV_I2C_Open(DRV_I2C_Handle_t *hi2c, const DEV_I2C_Object_t *object)
{
    MDS_ASSERT(hi2c != NULL);
    MDS_ASSERT(object != NULL);

    hi2c->handle.Init.ClockSpeed = object->clock;
    hi2c->handle.Init.OwnAddress1 = object->devAddress << 1U;
    hi2c->handle.Init.AddressingMode = (object->devAddrBit == DEV_I2C_DEVADDRBITS_10) ? (I2C_ADDRESSINGMODE_10BIT)
                                                                                      : (I2C_ADDRESSINGMODE_7BIT);

    HAL_StatusTypeDef status = HAL_I2C_Init(&(hi2c->handle));

    return (DRV_HalStatusToMdsErr(status));
}

MDS_Err_t DRV_I2C_Close(DRV_I2C_Handle_t *hi2c)
{
    MDS_ASSERT(hi2c != NULL);

    HAL_StatusTypeDef status = HAL_I2C_DeInit(&(hi2c->handle));

    return (DRV_HalStatusToMdsErr(status));
}

static HAL_StatusTypeDef I2C_WaitOnFlagWithEscapeCheck(I2C_HandleTypeDef *hi2c, uint32_t waitFlag, uint32_t escFlag,
                                                       MDS_Tick_t tickout, MDS_Tick_t tickstart)
{
    while ((DRV_HalGetTick() - tickstart) < tickout) {
        if (__HAL_I2C_GET_FLAG(hi2c, waitFlag) != RESET) {
            return (HAL_OK);
        }

        if ((escFlag != 0x00) && (__HAL_I2C_GET_FLAG(hi2c, escFlag) != RESET)) {
            __HAL_I2C_CLEAR_FLAG(hi2c, escFlag);
            break;
        }
    }

    return (HAL_ERROR);
}

static HAL_StatusTypeDef I2C_MasterRequestWrite(I2C_HandleTypeDef *hi2c, uint16_t devAddr, MDS_Tick_t tickout,
                                                MDS_Tick_t tickstart)
{
    HAL_StatusTypeDef ret;

    SET_BIT(hi2c->Instance->CR1, I2C_CR1_START);

    ret = I2C_WaitOnFlagWithEscapeCheck(hi2c, I2C_FLAG_SB, 0x00, tickout, tickstart);
    if (ret != HAL_OK) {
        return (ret);
    }

    if (hi2c->Init.AddressingMode == I2C_ADDRESSINGMODE_7BIT) {
        hi2c->Instance->DR = I2C_7BIT_ADD_WRITE(devAddr);
    } else {
        hi2c->Instance->DR = I2C_10BIT_HEADER_WRITE(devAddr);
        ret = I2C_WaitOnFlagWithEscapeCheck(hi2c, I2C_FLAG_ADD10, I2C_FLAG_AF, tickout, tickstart);
        if (ret != HAL_OK) {
            return (ret);
        }

        hi2c->Instance->DR = I2C_10BIT_ADDRESS(devAddr);
    }

    ret = I2C_WaitOnFlagWithEscapeCheck(hi2c, I2C_FLAG_ADDR, I2C_FLAG_AF, tickout, tickstart);

    return (ret);
}

static HAL_StatusTypeDef I2C_MasterRequestRead(I2C_HandleTypeDef *hi2c, uint16_t devAddr, MDS_Tick_t tickout,
                                               MDS_Tick_t tickstart)
{
    HAL_StatusTypeDef ret;

    SET_BIT(hi2c->Instance->CR1, I2C_CR1_ACK);
    SET_BIT(hi2c->Instance->CR1, I2C_CR1_START);

    ret = I2C_WaitOnFlagWithEscapeCheck(hi2c, I2C_FLAG_SB, 0x00, tickout, tickstart);
    if (ret != HAL_OK) {
        return (ret);
    }

    if (hi2c->Init.AddressingMode == I2C_ADDRESSINGMODE_7BIT) {
        hi2c->Instance->DR = I2C_7BIT_ADD_READ(devAddr);
    } else {
        hi2c->Instance->DR = I2C_10BIT_HEADER_WRITE(devAddr);
        ret = I2C_WaitOnFlagWithEscapeCheck(hi2c, I2C_FLAG_ADD10, I2C_FLAG_AF, tickout, tickstart);
        if (ret != HAL_OK) {
            return (ret);
        }

        hi2c->Instance->DR = I2C_10BIT_ADDRESS(devAddr);
        ret = I2C_WaitOnFlagWithEscapeCheck(hi2c, I2C_FLAG_ADDR, I2C_FLAG_AF, tickout, tickstart);
        if (ret != HAL_OK) {
            return (ret);
        }

        __HAL_I2C_CLEAR_ADDRFLAG(hi2c);

        SET_BIT(hi2c->Instance->CR1, I2C_CR1_START);
        ret = I2C_WaitOnFlagWithEscapeCheck(hi2c, I2C_FLAG_SB, 0x00, tickout, tickstart);
        if (ret != HAL_OK) {
            return (ret);
        }

        hi2c->Instance->DR = I2C_10BIT_HEADER_READ(devAddr);
    }

    ret = I2C_WaitOnFlagWithEscapeCheck(hi2c, I2C_FLAG_ADDR, I2C_FLAG_AF, tickout, tickstart);

    return (ret);
}

static HAL_StatusTypeDef HAL_I2C_Master_OpreateTransmit(I2C_HandleTypeDef *hi2c, const DEV_I2C_Msg_t *msg,
                                                        MDS_Tick_t tickout)
{
    HAL_StatusTypeDef ret = HAL_OK;
    MDS_Tick_t tickstart = DRV_HalGetTick();
    size_t idx = 0;

    if ((msg->flags & DEV_I2C_MSGFLAG_NO_START) == 0U) {
        ret = I2C_MasterRequestWrite(hi2c, hi2c->Init.OwnAddress1, tickout, tickstart);
        __HAL_I2C_CLEAR_ADDRFLAG(hi2c);
    }

    while ((ret == HAL_OK) && (idx < msg->len)) {
        ret = I2C_WaitOnFlagWithEscapeCheck(hi2c, I2C_FLAG_TXE, I2C_FLAG_AF, tickout, tickstart);
        if (ret != HAL_OK) {
            break;
        }

        hi2c->Instance->DR = msg->buff[idx++];

        if ((__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_BTF) == SET) && (idx < msg->len)) {
            hi2c->Instance->DR = msg->buff[idx++];
        }

        ret = I2C_WaitOnFlagWithEscapeCheck(hi2c, I2C_FLAG_BTF, I2C_FLAG_AF, tickout, tickstart);
    }

    if ((ret != HAL_OK) || ((msg->flags & DEV_I2C_MSGFLAG_NO_STOP) == 0U)) {
        SET_BIT(hi2c->Instance->CR1, I2C_CR1_STOP);
    }

    return (ret);
}

static HAL_StatusTypeDef HAL_I2C_Master_OpreateReceive(I2C_HandleTypeDef *hi2c, const DEV_I2C_Msg_t *msg,
                                                       MDS_Tick_t tickout)
{
    HAL_StatusTypeDef ret = HAL_OK;
    MDS_Tick_t tickstart = DRV_HalGetTick();
    size_t idx = 0;

    if ((msg->flags & DEV_I2C_MSGFLAG_NO_START) == 0U) {
        ret = I2C_MasterRequestRead(hi2c, hi2c->Init.OwnAddress1, tickout, tickstart);
        __HAL_I2C_CLEAR_ADDRFLAG(hi2c);
    }

    while ((ret == HAL_OK) && (idx < msg->len)) {
        ret = I2C_WaitOnFlagWithEscapeCheck(hi2c, I2C_FLAG_RXNE, I2C_FLAG_STOPF, tickout, tickstart);
        if (ret != HAL_OK) {
            break;
        }

        if (((msg->flags & DEV_I2C_MSGFLAG_NO_STOP) == 0U) && ((msg->len - idx) <= 2U)) {
            ret = I2C_WaitOnFlagWithEscapeCheck(hi2c, I2C_FLAG_BTF, 0x00, tickout, tickstart);
            if (ret == HAL_OK) {
                ((msg->len - idx) == 2U) ? (CLEAR_BIT(hi2c->Instance->CR1, I2C_CR1_ACK))
                                         : (SET_BIT(hi2c->Instance->CR1, I2C_CR1_STOP));
            }
        }

        msg->buff[idx++] = (uint8_t)(hi2c->Instance->DR);
    }

    if (ret != HAL_OK) {
        CLEAR_BIT(hi2c->Instance->CR1, I2C_CR1_ACK);
        SET_BIT(hi2c->Instance->CR1, I2C_CR1_STOP);
        (void)(hi2c->Instance->DR);
    }

    return (ret);
}

HAL_StatusTypeDef HAL_I2C_Master_Transfer(I2C_HandleTypeDef *hi2c, const DEV_I2C_Msg_t *msg, MDS_Tick_t tickout)
{
    HAL_StatusTypeDef ret;

    if ((hi2c->Instance->CR1 & I2C_CR1_PE) != I2C_CR1_PE) {
        __HAL_I2C_ENABLE(hi2c);
    }

    CLEAR_BIT(hi2c->Instance->CR1, I2C_CR1_POS);

    if ((msg->flags & DEV_I2C_MSGFLAG_RD) != 0U) {
        ret = HAL_I2C_Master_OpreateReceive(hi2c, msg, tickout);
    } else {
        ret = HAL_I2C_Master_OpreateTransmit(hi2c, msg, tickout);
    }

    return (ret);
}

MDS_Err_t DRV_I2C_MasterTransfer(DRV_I2C_Handle_t *hi2c, const DEV_I2C_Msg_t *msg, MDS_Tick_t tickout)
{
    MDS_ASSERT(hi2c != NULL);

    HAL_StatusTypeDef status = HAL_I2C_Master_Transfer(&(hi2c->handle), msg, tickout);

    return (DRV_HalStatusToMdsErr(status));
}

MDS_Err_t DRV_I2C_MasterTransferINT(DRV_I2C_Handle_t *hi2c, const DEV_I2C_Msg_t *msg)
{
    MDS_ASSERT(hi2c != NULL);

    uint32_t xferOptions;
    HAL_StatusTypeDef status;

    if ((msg->flags & DEV_I2C_MSGFLAG_NO_START) != 0U) {
        xferOptions = ((msg->flags & DEV_I2C_MSGFLAG_NO_STOP) != 0U) ? (I2C_LAST_FRAME_NO_STOP) : (I2C_LAST_FRAME);
    } else {
        xferOptions = ((msg->flags & DEV_I2C_MSGFLAG_NO_STOP) != 0U) ? (I2C_FIRST_FRAME) : (I2C_FIRST_AND_LAST_FRAME);
    }

    if ((msg->flags & DEV_I2C_MSGFLAG_RD) != 0U) {
        status = HAL_I2C_Master_Seq_Receive_IT(&(hi2c->handle), hi2c->handle.Init.OwnAddress1, msg->buff, msg->len,
                                               xferOptions);
    } else {
        status = HAL_I2C_Master_Seq_Transmit_IT(&(hi2c->handle), hi2c->handle.Init.OwnAddress1, msg->buff, msg->len,
                                                xferOptions);
    }

    return (DRV_HalStatusToMdsErr(status));
}

MDS_Err_t DRV_I2C_MasterWait(DRV_I2C_Handle_t *hi2c, MDS_Tick_t tickout)
{
    MDS_ASSERT(hi2c != NULL);

    return (MDS_SemaphoreAcquire(&(hi2c->sem), tickout));
}

MDS_Err_t DRV_I2C_MasterAbort(DRV_I2C_Handle_t *hi2c)
{
    MDS_ASSERT(hi2c != NULL);

    HAL_StatusTypeDef status = HAL_I2C_Master_Abort_IT(&(hi2c->handle), hi2c->handle.Init.OwnAddress1);

    return (DRV_HalStatusToMdsErr(status));
}

MDS_Err_t DRV_I2C_SlaveListenINT(DRV_I2C_Handle_t *hi2c)
{
    MDS_ASSERT(hi2c != NULL);

    HAL_StatusTypeDef status = HAL_I2C_EnableListen_IT(&(hi2c->handle));

    return (DRV_HalStatusToMdsErr(status));
}

MDS_Err_t DRV_I2C_SlaveReceiveINT(DRV_I2C_Handle_t *hi2c, uint8_t *buff, size_t size)
{
    MDS_ASSERT(hi2c != NULL);

    HAL_StatusTypeDef status = HAL_I2C_Slave_Receive_IT(&(hi2c->handle), buff, size);

    return (DRV_HalStatusToMdsErr(status));
}

MDS_Err_t DRV_I2C_SlaveTransmitINT(DRV_I2C_Handle_t *hi2c, uint8_t *buff, size_t len)
{
    MDS_ASSERT(hi2c != NULL);

    HAL_StatusTypeDef status = HAL_I2C_Slave_Transmit_IT(&(hi2c->handle), buff, len);

    return (DRV_HalStatusToMdsErr(status));
}

MDS_Err_t DRV_I2C_SlaveWait(DRV_I2C_Handle_t *hi2c, size_t *len, MDS_Tick_t tickout)
{
    MDS_ASSERT(hi2c != NULL);

    MDS_Err_t err = MDS_SemaphoreAcquire(&(hi2c->sem), tickout);

    if (len != NULL) {
        *len = hi2c->handle.XferCount;
    }

    return (err);
}

MDS_Err_t DRV_I2C_SlaveAbort(DRV_I2C_Handle_t *hi2c)
{
    MDS_ASSERT(hi2c != NULL);

    HAL_StatusTypeDef status = HAL_I2C_DisableListen_IT(&(hi2c->handle));

    return (DRV_HalStatusToMdsErr(status));
}

void DRV_I2C_EV_IRQHandler(DRV_I2C_Handle_t *hi2c)
{
    MDS_ASSERT(hi2c != NULL);

    HAL_I2C_EV_IRQHandler(&(hi2c->handle));
}

void DRV_I2C_ER_IRQHandler(DRV_I2C_Handle_t *hi2c)
{
    MDS_ASSERT(hi2c != NULL);

    HAL_I2C_ER_IRQHandler(&(hi2c->handle));
}

/* Driver ------------------------------------------------------------------ */
static MDS_Err_t DDRV_I2C_Control(const DEV_I2C_Adaptr_t *i2c, MDS_Item_t cmd, MDS_Arg_t *arg)
{
    MDS_ASSERT(i2c != NULL);

    DRV_I2C_Handle_t *hi2c = (DRV_I2C_Handle_t *)(i2c->handle);

    switch (cmd) {
        case MDS_DEVICE_CMD_INIT:
            return (DRV_I2C_Init(hi2c, (I2C_TypeDef *)arg));
        case MDS_DEVICE_CMD_DEINIT:
            return (DRV_I2C_DeInit(hi2c));
        case MDS_DEVICE_CMD_HANDLESZ:
            MDS_DEVICE_ARG_HANDLE_SIZE(arg, DRV_I2C_Handle_t);
            return (MDS_EOK);
        case MDS_DEVICE_CMD_OPEN:
            return (DRV_I2C_Open(hi2c, &(((DEV_I2C_Periph_t *)arg)->object)));
        case MDS_DEVICE_CMD_CLOSE:
            return (DRV_I2C_Close(hi2c));
    }

    return (MDS_EACCES);
}

static MDS_Err_t DDRV_I2C_MasterTransfer(const DEV_I2C_Periph_t *periph, const DEV_I2C_Msg_t *msg, MDS_Tick_t tickout)
{
    DRV_I2C_Handle_t *hi2c = (DRV_I2C_Handle_t *)(periph->mount->handle);

    return (DRV_I2C_MasterTransfer(hi2c, msg, tickout));
}

static MDS_Err_t DDRV_I2C_MasterTransferINT(const DEV_I2C_Periph_t *periph, const DEV_I2C_Msg_t *msg,
                                            MDS_Tick_t tickout)
{
    DRV_I2C_Handle_t *hi2c = (DRV_I2C_Handle_t *)(periph->mount->handle);

    MDS_Err_t err = DRV_I2C_MasterTransferINT(hi2c, msg);
    if (err == MDS_EOK) {
        err = DRV_I2C_MasterWait(hi2c, tickout);
    } else {
        DRV_I2C_MasterAbort(hi2c);
    }

    return (err);
}

static MDS_Err_t DDRV_I2C_SlaveTransferINT(const DEV_I2C_Periph_t *periph, DEV_I2C_Msg_t *msg, size_t *len,
                                           MDS_Tick_t tickout)
{
    MDS_Err_t err;
    DRV_I2C_Handle_t *hi2c = (DRV_I2C_Handle_t *)(periph->mount->handle);

    if (msg == NULL) {
        err = DRV_I2C_SlaveListenINT(hi2c);
    } else if ((msg->flags & DEV_I2C_MSGFLAG_RD) != 0U) {
        err = DRV_I2C_SlaveReceiveINT(hi2c, msg->buff, msg->len);
        if (err == MDS_EOK) {
            err = DRV_I2C_SlaveWait(hi2c, len, tickout);
        }
    } else {
        err = DRV_I2C_SlaveTransmitINT(hi2c, msg->buff, msg->len);
        if (err == MDS_EOK) {
            err = DRV_I2C_SlaveWait(hi2c, len, tickout);
        }
    }
    if (err != MDS_EOK) {
        DRV_I2C_SlaveAbort(hi2c);
    }

    return (err);
}

const DEV_I2C_Driver_t G_DRV_STM32F1XX_I2C_MASTER = {
    .control = DDRV_I2C_Control,
    .master = DDRV_I2C_MasterTransfer,
    .slave = NULL,
};

const DEV_I2C_Driver_t G_DRV_STM32F1XX_I2C_INT = {
    .control = DDRV_I2C_Control,
    .master = DDRV_I2C_MasterTransferINT,
    .slave = DDRV_I2C_SlaveTransferINT,
};
