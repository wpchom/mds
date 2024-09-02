/**
 * @copyright   Copyright (c) 2024 Pchom & licensed under Mulan PSL v2
 * @file        drv_i2c.h
 * @brief       stm32f1xx i2c driver for mds device
 * @date        2024-05-31
 */
#ifndef __DRV_I2C_H__
#define __DRV_I2C_H__

/* Include ----------------------------------------------------------------- */
#include "dev_i2c.h"
#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct DRV_I2C_Handle {
    I2C_HandleTypeDef handle;

    MDS_Semaphore_t sem;
} DRV_I2C_Handle_t;

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DRV_I2C_Init(DRV_I2C_Handle_t *hi2c, I2C_TypeDef *I2Cx);
extern MDS_Err_t DRV_I2C_DeInit(DRV_I2C_Handle_t *hi2c);
extern MDS_Err_t DRV_I2C_Open(DRV_I2C_Handle_t *hi2c, const DEV_I2C_Object_t *object);
extern MDS_Err_t DRV_I2C_Close(DRV_I2C_Handle_t *hi2c);

extern MDS_Err_t DRV_I2C_MasterTransfer(DRV_I2C_Handle_t *hi2c, const DEV_I2C_Msg_t *msg, MDS_Tick_t timeout);
extern MDS_Err_t DRV_I2C_MasterTransferINT(DRV_I2C_Handle_t *hi2c, const DEV_I2C_Msg_t *msg);
extern MDS_Err_t DRV_I2C_MasterWait(DRV_I2C_Handle_t *hi2c, MDS_Tick_t tickout);
extern MDS_Err_t DRV_I2C_MasterAbort(DRV_I2C_Handle_t *hi2c);

extern MDS_Err_t DRV_I2C_SlaveListenINT(DRV_I2C_Handle_t *hi2c);
extern MDS_Err_t DRV_I2C_SlaveReceiveINT(DRV_I2C_Handle_t *hi2c, uint8_t *buff, size_t size);
extern MDS_Err_t DRV_I2C_SlaveTransmitINT(DRV_I2C_Handle_t *hi2c, uint8_t *buff, size_t len);
extern MDS_Err_t DRV_I2C_SlaveWait(DRV_I2C_Handle_t *hi2c, size_t *len, MDS_Tick_t tickout);
extern MDS_Err_t DRV_I2C_SlaveAbort(DRV_I2C_Handle_t *hi2c);

extern void DRV_I2C_EV_IRQHandler(DRV_I2C_Handle_t *hi2c);
extern void DRV_I2C_ER_IRQHandler(DRV_I2C_Handle_t *hi2c);

/* Driver ------------------------------------------------------------------ */
extern const DEV_I2C_Driver_t G_DRV_STM32F1XX_I2C_MASTER;
extern const DEV_I2C_Driver_t G_DRV_STM32F1XX_I2C_INT;

#ifdef __cplusplus
}
#endif

#endif /* __DRV_I2C_H__ */
