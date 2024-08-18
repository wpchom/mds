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
#include "drv_fpga_simulate.h"

/* Define ------------------------------------------------------------------ */
#define FPGA_DATA_BYTE_MSB 0x80U

/* Function ---------------------------------------------------------------- */
static void FPGA_SimulateSendByte(DRV_FPGA_SimulateHandle_t *hfpga, uint8_t data)
{
    MDS_Item_t lock = MDS_CoreInterruptLock();

    size_t cnt;
    for (cnt = 0; cnt < MDS_BITS_OF_BYTE; cnt++) {
        DEV_GPIO_PinLow(hfpga->clk);
        if ((data & FPGA_DATA_BYTE_MSB) != 0U) {
            DEV_GPIO_PinHigh(hfpga->dat);
        } else {
            DEV_GPIO_PinLow(hfpga->dat);
        }
        MDS_SysCountDelay(hfpga->delay);
        DEV_GPIO_PinHigh(hfpga->clk);
        MDS_SysCountDelay(hfpga->delay);
        data <<= 1;
    }

    MDS_CoreInterruptRestore(lock);
}

MDS_Err_t DRV_FPGA_SimulateInit(DRV_FPGA_SimulateHandle_t *hfpga)
{
    MDS_ASSERT(hfpga != NULL);

    static const DEV_GPIO_Config_t gpioConfig = {
        .mode = DEV_GPIO_MODE_OUTPUT,
        .type = DEV_GPIO_TYPE_PP_UP,
        .alternate = 0,
    };

    DEV_GPIO_PinConfig(hfpga->clk, &gpioConfig);
    DEV_GPIO_PinConfig(hfpga->dat, &gpioConfig);

    return (MDS_EOK);
}

MDS_Err_t DRV_FPGA_SimulateDeInit(DRV_FPGA_SimulateHandle_t *hfpga)
{
    MDS_ASSERT(hfpga != NULL);

    DEV_GPIO_PinDeInit(hfpga->clk);
    DEV_GPIO_PinDeInit(hfpga->dat);

    return (MDS_EOK);
}

MDS_Err_t DRV_FPGA_SimulateTransmit(DRV_FPGA_SimulateHandle_t *hfpga, const uint8_t *buff, size_t len)
{
    MDS_ASSERT(hfpga != NULL);
    MDS_ASSERT(buff != NULL);

    size_t cnt;
    for (cnt = 0; cnt < len; cnt++) {
        FPGA_SimulateSendByte(hfpga, buff[cnt]);
    }

    return (MDS_EOK);
}

MDS_Err_t DRV_FPGA_SimulateStart(DRV_FPGA_SimulateHandle_t *hfpga, const DEV_FPGA_Object_t *object)
{
    MDS_ASSERT(hfpga != NULL);
    MDS_ASSERT(object != NULL);

    DEV_GPIO_PinHigh(object->prog_b);
    MDS_SysCountDelay(hfpga->delay);
    DEV_GPIO_PinLow(object->prog_b);

    MDS_Tick_t tickstart = MDS_SysTickGetCount();
    while (DEV_GPIO_PinRead(object->init_b) != DEV_GPIO_LEVEL_LOW) {
        if ((MDS_SysTickGetCount() - tickstart) > object->timeout) {
            DEV_GPIO_PinHigh(object->prog_b);
            return (MDS_ETIME);
        }
    }

    MDS_SysCountDelay(hfpga->delay);
    DEV_GPIO_PinHigh(object->prog_b);

    tickstart = MDS_SysTickGetCount();
    while (DEV_GPIO_PinRead(object->init_b) != DEV_GPIO_LEVEL_HIGH) {
        if ((MDS_SysTickGetCount() - tickstart) > object->timeout) {
            return (MDS_ETIME);
        }
    }

    return (MDS_EOK);
}

MDS_Err_t DRV_FPGA_SimulateFinish(DRV_FPGA_SimulateHandle_t *hfpga, const DEV_FPGA_Object_t *object)
{
    MDS_ASSERT(hfpga != NULL);
    MDS_ASSERT(object != NULL);

    MDS_Tick_t tickstart = MDS_SysTickGetCount();

    while (DEV_GPIO_PinRead(object->done) != DEV_GPIO_LEVEL_HIGH) {
        if ((MDS_SysTickGetCount() - tickstart) > object->timeout) {
            return (MDS_ETIME);
        }
    }

    return (MDS_EOK);
}

/* Driver ------------------------------------------------------------------ */
static MDS_Err_t DDRV_FPGA_Control(const DEV_FPGA_Adaptr_t *fpga, MDS_Item_t cmd, void *arg)
{
    DRV_FPGA_SimulateHandle_t *hfpga = (DRV_FPGA_SimulateHandle_t *)(fpga->handle);

    switch (cmd) {
        case MDS_DEVICE_CMD_INIT:
            return (DRV_FPGA_SimulateInit(hfpga));
        case MDS_DEVICE_CMD_DEINIT:
            return (DRV_FPGA_SimulateDeInit(hfpga));
        case MDS_DEVICE_CMD_HANDLESZ:
            MDS_DEVICE_ARG_HANDLE_SIZE(arg, DRV_FPGA_SimulateHandle_t);
            return (MDS_EOK);
        case MDS_DEVICE_CMD_OPEN:
        case MDS_DEVICE_CMD_CLOSE:
            return (MDS_EOK);
        default:
            break;
    }

    return (MDS_EPERM);
}

static MDS_Err_t DDRV_FPGA_Start(const DEV_FPGA_Periph_t *periph)
{
    DRV_FPGA_SimulateHandle_t *hfpga = (DRV_FPGA_SimulateHandle_t *)(periph->mount->handle);

    return (DRV_FPGA_SimulateStart(hfpga, &(periph->object)));
}

static MDS_Err_t DDRV_FPGA_Transmit(const DEV_FPGA_Periph_t *periph, const uint8_t *buff, size_t len)
{
    DRV_FPGA_SimulateHandle_t *hfpga = (DRV_FPGA_SimulateHandle_t *)(periph->mount->handle);

    return (DRV_FPGA_SimulateTransmit(hfpga, buff, len));
}

static MDS_Err_t DDRV_FPGA_Finish(const DEV_FPGA_Periph_t *periph)
{
    DRV_FPGA_SimulateHandle_t *hfpga = (DRV_FPGA_SimulateHandle_t *)(periph->mount->handle);

    return (DRV_FPGA_SimulateFinish(hfpga, &(periph->object)));
}

const DEV_FPGA_Driver_t G_DRV_FPGA_SIMULATE = {
    .control = DDRV_FPGA_Control,
    .start = DDRV_FPGA_Start,
    .transmit = DDRV_FPGA_Transmit,
    .finish = DDRV_FPGA_Finish,
};
