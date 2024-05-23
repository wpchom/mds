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
#ifndef __DRV_FPGA_SIMULATE_H__
#define __DRV_FPGA_SIMULATE_H__

/* Include ----------------------------------------------------------------- */
#include "extend/dev_fpga.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct DRV_FPGA_SimulateHandle {
    DEV_GPIO_Pin_t *clk;
    DEV_GPIO_Pin_t *dat;
    MDS_Tick_t delay;
} DRV_FPGA_SimulateHandle_t;

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DRV_FPGA_SimulateInit(DRV_FPGA_SimulateHandle_t *hfpga);
extern MDS_Err_t DRV_FPGA_SimulateDeInit(DRV_FPGA_SimulateHandle_t *hfpga);
extern MDS_Err_t DRV_FPGA_SimulateTransmit(DRV_FPGA_SimulateHandle_t *hfpga, const uint8_t *buff, size_t len);
extern MDS_Err_t DRV_FPGA_SimulateStart(DRV_FPGA_SimulateHandle_t *hfpga, const DEV_FPGA_Object_t *object);
extern MDS_Err_t DRV_FPGA_SimulateFinish(DRV_FPGA_SimulateHandle_t *hfpga, const DEV_FPGA_Object_t *object);

/* Driver ------------------------------------------------------------------ */
extern const DEV_FPGA_Driver_t G_DRV_FPGA_SIMULATE;

#ifdef __cplusplus
}
#endif

#endif /* __DRV_FPGA_SIMULATE_H__ */
