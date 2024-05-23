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
#ifndef __DRV_SPI_SIMULATE_H__
#define __DRV_SPI_SIMULATE_H__

/* Include ----------------------------------------------------------------- */
#include "dev_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct DRV_SPI_SimulateHandle {
    DEV_GPIO_Pin_t *sclk;
    DEV_GPIO_Pin_t *miso;
    DEV_GPIO_Pin_t *mosi;
    MDS_Tick_t delay;
} DRV_SPI_SimulateHandle_t;

/* Funtcion ---------------------------------------------------------------- */
extern MDS_Err_t DRV_SPI_SimulateInit(DRV_SPI_SimulateHandle_t *hspi);
extern MDS_Err_t DRV_SPI_SimulateDeInit(DRV_SPI_SimulateHandle_t *hspi);
extern MDS_Err_t DRV_SPI_SimulateTransfer(DRV_SPI_SimulateHandle_t *hspi, const DEV_SPI_Config_t *config,
                                          const void *txbuff, void *rxbuff, size_t cnt);

/* Driver ------------------------------------------------------------------ */
extern const DEV_SPI_Driver_t G_DRV_SPI_SIMULATE;

#ifdef __cplusplus
}
#endif

#endif /* __DRV_SPI_SIMULATE_H__ */
