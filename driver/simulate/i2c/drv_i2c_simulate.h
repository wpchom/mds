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
#ifndef __DRV_I2C_SIMULATE_H__
#define __DRV_I2C_SIMULATE_H__

/* Include ----------------------------------------------------------------- */
#include "dev_i2c.h"
#include "dev_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct DRV_I2C_SimulateHandle {
    DEV_GPIO_Pin_t *scl;
    DEV_GPIO_Pin_t *sda;
    uint32_t delay;
} DRV_I2C_SimulateHandle_t;

/* Funtcion ---------------------------------------------------------------- */
extern MDS_Err_t DRV_I2C_SimulateInit(DRV_I2C_SimulateHandle_t *hi2c);
extern MDS_Err_t DRV_I2C_SimulateDeInit(DRV_I2C_SimulateHandle_t *hi2c);
extern MDS_Err_t DRV_I2C_SimulateTransfer(DRV_I2C_SimulateHandle_t *hi2c, const DEV_I2C_Object_t *obj,
                                          DEV_I2C_Msg_t *msg);
extern MDS_Err_t DRV_I2C_SimulateTransferAbort(DRV_I2C_SimulateHandle_t *hi2c);

/* Driver ------------------------------------------------------------------ */
extern const DEV_I2C_Driver_t G_DRV_I2C_SIMULATE;

#ifdef __cplusplus
}
#endif

#endif /* __DRV_I2C_SIMULATE_H__ */
