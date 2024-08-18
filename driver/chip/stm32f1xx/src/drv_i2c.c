/**
 * @copyright   Copyright (c) 2024 Pchom & licensed under Mulan PSL v2
 * @file        drv_i2c.c
 * @brief       stm32f1xx i2c driver for mds device
 * @date        2024-05-31
 */
/* Include ----------------------------------------------------------------- */
#include "drv_i2c.h"
#include "drv_def.h"

/* Function ---------------------------------------------------------------- */
MDS_Err_t DRV_I2C_Init(DRV_I2C_Handle_t *hi2c, I2C_TypeDef *I2Cx)
{
    MDS_ASSERT(hi2c != NULL);

    hi2c->handle.Instance = I2Cx;

    return (MDS_EOK);
}

MDS_Err_t DRV_I2C_DeInit(DRV_I2C_Handle_t *hi2c)
{
    MDS_ASSERT(hi2c != NULL);

    return (MDS_EOK);
}

MDS_Err_t DRV_I2C_Open(DRV_I2C_Handle_t *hi2c, const DEV_I2C_Object_t *object)
{
    MDS_ASSERT(hi2c != NULL);
    MDS_ASSERT(object != NULL);

    hi2c->handle.Init.ClockSpeed = object->clock;
    hi2c->handle.Init.OwnAddress1 = object->devAddress;
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


static HAL_StatusTypeDef I2C_WaitOnMasterAddressFlagUntilTimeout(I2C_HandleTypeDef *hi2c, uint32_t Flag, uint32_t Timeout, uint32_t Tickstart)
{
  while (__HAL_I2C_GET_FLAG(hi2c, Flag) == RESET)
  {
    if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_AF) == SET)
    {
      /* Generate Stop */
      SET_BIT(hi2c->Instance->CR1, I2C_CR1_STOP);

      /* Clear AF Flag */
      __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_AF);

      hi2c->PreviousState       = I2C_STATE_NONE;
      hi2c->State               = HAL_I2C_STATE_READY;
      hi2c->Mode                = HAL_I2C_MODE_NONE;
      hi2c->ErrorCode           |= HAL_I2C_ERROR_AF;

      /* Process Unlocked */
      __HAL_UNLOCK(hi2c);

      return HAL_ERROR;
    }

    /* Check for the Timeout */
    if (Timeout != HAL_MAX_DELAY)
    {
      if (((HAL_GetTick() - Tickstart) > Timeout) || (Timeout == 0U))
      {
        if ((__HAL_I2C_GET_FLAG(hi2c, Flag) == RESET))
        {
          hi2c->PreviousState       = I2C_STATE_NONE;
          hi2c->State               = HAL_I2C_STATE_READY;
          hi2c->Mode                = HAL_I2C_MODE_NONE;
          hi2c->ErrorCode           |= HAL_I2C_ERROR_TIMEOUT;

          /* Process Unlocked */
          __HAL_UNLOCK(hi2c);

          return HAL_ERROR;
        }
      }
    }
  }
  return HAL_OK;
}

static HAL_StatusTypeDef I2C_MasterRequestRead(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint32_t Timeout,
                                               uint32_t Tickstart)
{
    /* Declaration of temporary variable to prevent undefined behavior of volatile usage */
    uint32_t CurrentXferOptions = hi2c->XferOptions;

    /* Enable Acknowledge */
    SET_BIT(hi2c->Instance->CR1, I2C_CR1_ACK);

    /* Generate Start condition if first transfer */
    if ((CurrentXferOptions == I2C_FIRST_AND_LAST_FRAME) || (CurrentXferOptions == I2C_FIRST_FRAME) ||
        (CurrentXferOptions == I2C_NO_OPTION_FRAME)) {
        /* Generate Start */
        SET_BIT(hi2c->Instance->CR1, I2C_CR1_START);
    } else if (hi2c->PreviousState == I2C_STATE_MASTER_BUSY_TX) {
        /* Generate ReStart */
        SET_BIT(hi2c->Instance->CR1, I2C_CR1_START);
    } else {
        /* Do nothing */
    }

    /* Wait until SB flag is set */
    if (I2C_WaitOnFlagUntilTimeout(hi2c, I2C_FLAG_SB, RESET, Timeout, Tickstart) != HAL_OK) {
        if (READ_BIT(hi2c->Instance->CR1, I2C_CR1_START) == I2C_CR1_START) {
            hi2c->ErrorCode = HAL_I2C_WRONG_START;
        }
        return HAL_TIMEOUT;
    }

    if (hi2c->Init.AddressingMode == I2C_ADDRESSINGMODE_7BIT) {
        /* Send slave address */
        hi2c->Instance->DR = I2C_7BIT_ADD_READ(DevAddress);
    } else {
        /* Send header of slave address */
        hi2c->Instance->DR = I2C_10BIT_HEADER_WRITE(DevAddress);

        /* Wait until ADD10 flag is set */
        if (I2C_WaitOnMasterAddressFlagUntilTimeout(hi2c, I2C_FLAG_ADD10, Timeout, Tickstart) != HAL_OK) {
            return HAL_ERROR;
        }

        /* Send slave address */
        hi2c->Instance->DR = I2C_10BIT_ADDRESS(DevAddress);

        /* Wait until ADDR flag is set */
        if (I2C_WaitOnMasterAddressFlagUntilTimeout(hi2c, I2C_FLAG_ADDR, Timeout, Tickstart) != HAL_OK) {
            return HAL_ERROR;
        }

        /* Clear ADDR flag */
        __HAL_I2C_CLEAR_ADDRFLAG(hi2c);

        /* Generate Restart */
        SET_BIT(hi2c->Instance->CR1, I2C_CR1_START);

        /* Wait until SB flag is set */
        if (I2C_WaitOnFlagUntilTimeout(hi2c, I2C_FLAG_SB, RESET, Timeout, Tickstart) != HAL_OK) {
            if (READ_BIT(hi2c->Instance->CR1, I2C_CR1_START) == I2C_CR1_START) {
                hi2c->ErrorCode = HAL_I2C_WRONG_START;
            }
            return HAL_TIMEOUT;
        }

        /* Send header of slave address */
        hi2c->Instance->DR = I2C_10BIT_HEADER_READ(DevAddress);
    }

    /* Wait until ADDR flag is set */
    if (I2C_WaitOnMasterAddressFlagUntilTimeout(hi2c, I2C_FLAG_ADDR, Timeout, Tickstart) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

static HAL_StatusTypeDef I2C_MasterRequestWrite(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint32_t Timeout,
                                                uint32_t Tickstart)
{
    /* Declaration of temporary variable to prevent undefined behavior of volatile usage */
    uint32_t CurrentXferOptions = hi2c->XferOptions;

    /* Generate Start condition if first transfer */
    if ((CurrentXferOptions == I2C_FIRST_AND_LAST_FRAME) || (CurrentXferOptions == I2C_FIRST_FRAME) ||
        (CurrentXferOptions == I2C_NO_OPTION_FRAME)) {
        /* Generate Start */
        SET_BIT(hi2c->Instance->CR1, I2C_CR1_START);
    } else if (hi2c->PreviousState == I2C_STATE_MASTER_BUSY_RX) {
        /* Generate ReStart */
        SET_BIT(hi2c->Instance->CR1, I2C_CR1_START);
    } else {
        /* Do nothing */
    }

    /* Wait until SB flag is set */
    if (I2C_WaitOnFlagUntilTimeout(hi2c, I2C_FLAG_SB, RESET, Timeout, Tickstart) != HAL_OK) {
        if (READ_BIT(hi2c->Instance->CR1, I2C_CR1_START) == I2C_CR1_START) {
            hi2c->ErrorCode = HAL_I2C_WRONG_START;
        }
        return HAL_TIMEOUT;
    }

    if (hi2c->Init.AddressingMode == I2C_ADDRESSINGMODE_7BIT) {
        /* Send slave address */
        hi2c->Instance->DR = I2C_7BIT_ADD_WRITE(DevAddress);
    } else {
        /* Send header of slave address */
        hi2c->Instance->DR = I2C_10BIT_HEADER_WRITE(DevAddress);

        /* Wait until ADD10 flag is set */
        if (I2C_WaitOnMasterAddressFlagUntilTimeout(hi2c, I2C_FLAG_ADD10, Timeout, Tickstart) != HAL_OK) {
            return HAL_ERROR;
        }

        /* Send slave address */
        hi2c->Instance->DR = I2C_10BIT_ADDRESS(DevAddress);
    }

    /* Wait until ADDR flag is set */
    if (I2C_WaitOnMasterAddressFlagUntilTimeout(hi2c, I2C_FLAG_ADDR, Timeout, Tickstart) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

MDS_Err_t DRV_I2C_MasterTransfer(DRV_I2C_Handle_t *hi2c, uint16_t devAddress, DEV_I2C_DevAddrBits_t devAddrBit,
                                 const DEV_I2C_Msg_t *msg, MDS_Tick_t timeout)
{
    MDS_Tick_t tickstart = MDS_SysTickGetCount();

    if ((msg->flags & DEV_I2C_MSGFLAG_NO_START) != 0U) {
        if (msg->flags & DEV_I2C_MSGFLAG_RD) {
        } else {
        }
    }

    if ((msg->flags & DEV_I2C_MSGFLAG_NO_STOP) == 0U) {
    }
}

MDS_Err_t DRV_I2C_SlaveReceive(DRV_I2C_Handle_t *hi2c, uint8_t *buff, size_t size, MDS_Tick_t timeout)
{
    MDS_ASSERT(hi2c != NULL);

    HAL_StatusTypeDef status = HAL_I2C_Slave_Receive(&(hi2c->handle), buff, size, timeout);

    return (DRV_HalStatusToMdsErr(status));
}

MDS_Err_t DRV_I2C_SlaveTransmit(DRV_I2C_Handle_t *hi2c, uint8_t *buff, size_t len, MDS_Tick_t timeout)
{
    MDS_ASSERT(hi2c != NULL);

    HAL_StatusTypeDef status = HAL_I2C_Slave_Transmit(&(hi2c->handle), buff, len, timeout);

    return (DRV_HalStatusToMdsErr(status));
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
            return (DRV_I2C_Open(hi2c, &(((DEV_I2C_Periph_t *)arg)->config)));
        case MDS_DEVICE_CMD_CLOSE:
            return (DRV_I2C_Close(hi2c));
    }

    return (MDS_EACCES);
}

static MDS_Err_t DDRV_I2C_MasterTransfer(const DEV_I2C_Periph_t *periph, const DEV_I2C_Msg_t *msg)
{
    DRV_I2C_Handle_t *hi2c = (DRV_I2C_Handle_t *)(periph->mount->handle);

    return (DRV_I2C_MasterTransfer(hi2c, periph->config.devAddress, periph->config.devAddrBit, msg,
                                   periph->config.optick));
}

static MDS_Err_t DDRV_I2C_SlaveTransfer(const DEV_I2C_Periph_t *periph, bool read, uint8_t *buff, size_t size,
                                        MDS_Tick_t timeout)
{
    MDS_Err_t err;
    DRV_I2C_Handle_t *hi2c = (DRV_I2C_Handle_t *)(periph->mount->handle);

    if (read) {
        err = DRV_I2C_SlaveReceive(hi2c, buff, size, timeout);
    } else {
        err = DRV_I2C_SlaveTransmit(hi2c, buff, size, timeout);
    }

    return (err);
}

const DEV_I2C_Driver_t G_DRV_STM32F1XX_I2C_MASTER = {
    .control = DDRV_I2C_Control,
    .master = DDRV_I2C_MasterTransfer,
    .slave = NULL,
};

const DEV_I2C_Driver_t G_DRV_STM32F1XX_I2C = {
    .control = DDRV_I2C_Control,
    .master = DDRV_I2C_MasterTransfer,
    .slave = DDRV_I2C_SlaveTransfer,
};
