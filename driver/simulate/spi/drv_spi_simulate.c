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
#include "drv_spi_simulate.h"

/* Define ------------------------------------------------------------------ */
#define SPI_DATA_BYTE_LSB 0x01U

/* Typedef ----------------------------------------------------------------- */
typedef struct SPI_bitSwap {
    uint32_t bitCnt;
    uint32_t bitWr, bitRd;
    uint32_t (*swap)(DRV_SPI_SimulateHandle_t *hspi, struct SPI_bitSwap *bitSwap, uint32_t dat);
} SPI_BitSwap_t;

/* Function ---------------------------------------------------------------- */
static void SPI_SimulateDirect(DEV_GPIO_Pin_t *sio, DEV_GPIO_Mode_t mode)
{
    DEV_GPIO_Config_t sioConfig = {
        .mode = mode,
        .type = DEV_GPIO_TYPE_PP_UP,
        .speed = DEV_GPIO_SPEED_HIGH,
        .alternate = 0,
    };

    DEV_GPIO_PinConfig(sio, &sioConfig);
}

/* CPOL = 0, CPHA = 0, MSB first */
static uint32_t SPI_SimulateSwap_Mode0(DRV_SPI_SimulateHandle_t *hspi, SPI_BitSwap_t *bitSwap, uint32_t dat)
{
    uint32_t value = 0x00;
    MDS_Item_t lock = MDS_CoreInterruptLock();

    uint32_t cnt;
    for (cnt = 0; cnt < bitSwap->bitCnt; cnt++) {
        if ((dat & bitSwap->bitWr) != 0U) {
            DEV_GPIO_PinHigh(hspi->mosi);
        } else {
            DEV_GPIO_PinLow(hspi->mosi);
        }
        MDS_SysCountDelay(hspi->delay);
        DEV_GPIO_PinHigh(hspi->sclk);
        if (bitSwap->bitWr >= bitSwap->bitRd) {
            dat <<= 1;
            value <<= 1;
        } else {
            dat >>= 1;
            value >>= 1;
        }
        if (DEV_GPIO_PinRead(hspi->miso) == DEV_GPIO_LEVEL_HIGH) {
            value |= bitSwap->bitRd;
        }
        MDS_SysCountDelay(hspi->delay);
        DEV_GPIO_PinLow(hspi->sclk);
        MDS_SysCountDelay(hspi->delay);
    }
    MDS_CoreInterruptRestore(lock);

    return (value);
}

/* CPOL=0，CPHA=1, MSB first */
static uint32_t SPI_SimulateSwap_Mode1(DRV_SPI_SimulateHandle_t *hspi, SPI_BitSwap_t *bitSwap, uint32_t dat)
{
    uint32_t value = 0x00;
    MDS_Item_t lock = MDS_CoreInterruptLock();

    uint32_t cnt;
    for (cnt = 0; cnt < bitSwap->bitCnt; cnt++) {
        DEV_GPIO_PinHigh(hspi->sclk);
        if ((dat & bitSwap->bitWr) != 0U) {
            DEV_GPIO_PinHigh(hspi->mosi);
        } else {
            DEV_GPIO_PinLow(hspi->mosi);
        }
        MDS_SysCountDelay(hspi->delay);
        DEV_GPIO_PinLow(hspi->sclk);
        if (bitSwap->bitWr >= bitSwap->bitRd) {
            dat <<= 1;
            value <<= 1;
        } else {
            dat >>= 1;
            value >>= 1;
        }
        if (DEV_GPIO_PinRead(hspi->miso) == DEV_GPIO_LEVEL_HIGH) {
            value |= bitSwap->bitRd;
        }
        MDS_SysCountDelay(hspi->delay);
    }
    MDS_CoreInterruptRestore(lock);

    return (value);
}

/* CPOL=1，CPHA=0, MSB first */
static uint32_t SPI_SimulateSwap_Mode2(DRV_SPI_SimulateHandle_t *hspi, SPI_BitSwap_t *bitSwap, uint32_t dat)
{
    uint32_t value = 0x00;
    MDS_Item_t lock = MDS_CoreInterruptLock();

    uint32_t cnt;
    for (cnt = 0; cnt < bitSwap->bitCnt; cnt++) {
        if ((dat & bitSwap->bitWr) != 0U) {
            DEV_GPIO_PinHigh(hspi->mosi);
        } else {
            DEV_GPIO_PinLow(hspi->mosi);
        }
        MDS_SysCountDelay(hspi->delay);
        DEV_GPIO_PinLow(hspi->sclk);
        if (bitSwap->bitWr >= bitSwap->bitRd) {
            dat <<= 1;
            value <<= 1;
        } else {
            dat >>= 1;
            value >>= 1;
        }
        if (DEV_GPIO_PinRead(hspi->miso) == DEV_GPIO_LEVEL_HIGH) {
            value |= bitSwap->bitRd;
        }
        MDS_SysCountDelay(hspi->delay);
        DEV_GPIO_PinHigh(hspi->sclk);
    }
    MDS_CoreInterruptRestore(lock);

    return (value);
}

/* CPOL = 1, CPHA = 1, MSB first */
static uint32_t SPI_SimulateSwap_Mode3(DRV_SPI_SimulateHandle_t *hspi, SPI_BitSwap_t *bitSwap, uint32_t dat)
{
    uint32_t value = 0x00;
    MDS_Item_t lock = MDS_CoreInterruptLock();

    uint32_t cnt;
    for (cnt = 0; cnt < bitSwap->bitCnt; cnt++) {
        DEV_GPIO_PinLow(hspi->sclk);
        if ((dat & bitSwap->bitWr) != 0U) {
            DEV_GPIO_PinHigh(hspi->mosi);
        } else {
            DEV_GPIO_PinLow(hspi->mosi);
        }
        MDS_SysCountDelay(hspi->delay);
        DEV_GPIO_PinHigh(hspi->sclk);
        if (bitSwap->bitWr >= bitSwap->bitRd) {
            dat <<= 1;
            value <<= 1;
        } else {
            dat >>= 1;
            value >>= 1;
        }
        if (DEV_GPIO_PinRead(hspi->miso) == DEV_GPIO_LEVEL_HIGH) {
            value |= bitSwap->bitRd;
        }
        MDS_SysCountDelay(hspi->delay);
    }
    MDS_CoreInterruptRestore(lock);

    return (value);
}

static void SPI_SwapData_8B(DRV_SPI_SimulateHandle_t *hspi, SPI_BitSwap_t *bitSwap, const void *txbuff, void *rxbuff,
                            size_t idx)
{
    uint32_t val = 0xFF;

    if (txbuff != NULL) {
        const uint8_t *datWr = txbuff;
        val = datWr[idx];
    }
    if (bitSwap->swap != NULL) {
        val = bitSwap->swap(hspi, bitSwap, val);
    }
    if (rxbuff != NULL) {
        uint8_t *datRd = rxbuff;
        datRd[idx] = val;
    }
}

static void SPI_SwapData_16B(DRV_SPI_SimulateHandle_t *hspi, SPI_BitSwap_t *bitSwap, const void *txbuff, void *rxbuff,
                             size_t idx)
{
    uint16_t val = 0xFFFF;

    if (txbuff != NULL) {
        const uint16_t *datWr = txbuff;
        val = datWr[idx];
    }
    if (bitSwap->swap != NULL) {
        val = bitSwap->swap(hspi, bitSwap, val);
    }
    if (rxbuff != NULL) {
        uint16_t *datRd = rxbuff;
        datRd[idx] = val;
    }
}

static void SPI_SwapData_32B(DRV_SPI_SimulateHandle_t *hspi, SPI_BitSwap_t *bitSwap, const void *txbuff, void *rxbuff,
                             size_t idx)
{
    uint32_t val = 0xFFFFFFFF;

    if (txbuff != NULL) {
        const uint32_t *datWr = txbuff;
        val = datWr[idx];
    }
    if (bitSwap->swap != NULL) {
        val = bitSwap->swap(hspi, bitSwap, val);
    }
    if (rxbuff != NULL) {
        uint32_t *datRd = rxbuff;
        datRd[idx] = val;
    }
}

static void SPI_SimulateSwapConfig(const DEV_SPI_Config_t *config, SPI_BitSwap_t *bitSwap)
{
    bitSwap->bitCnt = MDS_BITS_OF_BYTE;
    if (config->dataBits == DEV_SPI_DATABITS_16) {
        bitSwap->bitCnt *= sizeof(uint16_t);
    } else if (config->dataBits == DEV_SPI_DATABITS_32) {
        bitSwap->bitCnt *= sizeof(uint32_t);
    }

    if (config->firstBit == DEV_SPI_FIRSTBIT_MSB) {
        bitSwap->bitWr = SPI_DATA_BYTE_LSB << (bitSwap->bitCnt - 1);
        bitSwap->bitRd = SPI_DATA_BYTE_LSB;
    } else {
        bitSwap->bitWr = SPI_DATA_BYTE_LSB;
        bitSwap->bitRd = SPI_DATA_BYTE_LSB << (bitSwap->bitCnt - 1);
    }

    switch (config->clkMode) {
        case DEV_SPI_CLKMODE_3:
            bitSwap->swap = SPI_SimulateSwap_Mode3;
            break;
        case DEV_SPI_CLKMODE_2:
            bitSwap->swap = SPI_SimulateSwap_Mode2;
            break;
        case DEV_SPI_CLKMODE_1:
            bitSwap->swap = SPI_SimulateSwap_Mode1;
            break;
        default:
            bitSwap->swap = SPI_SimulateSwap_Mode0;
            break;
    }
}

MDS_Err_t DRV_SPI_SimulateInit(DRV_SPI_SimulateHandle_t *hspi)
{
    MDS_ASSERT(hspi != NULL);

    static const DEV_GPIO_Config_t gpioConfig = {
        .mode = DEV_GPIO_MODE_OUTPUT,
        .type = DEV_GPIO_TYPE_PP_UP,
        .speed = DEV_GPIO_SPEED_HIGH,
        .alternate = 0,
    };

    DEV_GPIO_PinConfig(hspi->sclk, &gpioConfig);

    return (MDS_EOK);
}

MDS_Err_t DRV_SPI_SimulateDeInit(DRV_SPI_SimulateHandle_t *hspi)
{
    MDS_ASSERT(hspi != NULL);

    DEV_GPIO_PinDeInit(hspi->sclk);
    DEV_GPIO_PinDeInit(hspi->mosi);
    if (hspi->miso != NULL) {
        DEV_GPIO_PinDeInit(hspi->miso);
    }

    return (MDS_EOK);
}

MDS_Err_t DRV_SPI_SimulateTransfer(DRV_SPI_SimulateHandle_t *hspi, const DEV_SPI_Config_t *config, const void *txbuff,
                                   void *rxbuff, size_t cnt)
{
    SPI_BitSwap_t bitSwap;

    SPI_SimulateSwapConfig(config, &bitSwap);

    if (config->busMode != DEV_SPI_BUSMODE_MASTER_HALF) {
        SPI_SimulateDirect(hspi->mosi, DEV_GPIO_MODE_OUTPUT);
        SPI_SimulateDirect(hspi->miso, DEV_GPIO_MODE_INPUT);
    } else if (rxbuff != NULL) {
        SPI_SimulateDirect(hspi->mosi, DEV_GPIO_MODE_INPUT);
    } else {
        SPI_SimulateDirect(hspi->mosi, DEV_GPIO_MODE_OUTPUT);
    }

    void (*swapData)(DRV_SPI_SimulateHandle_t *, SPI_BitSwap_t *, const void *, void *, size_t) = NULL;
    switch (config->dataBits) {
        case DEV_SPI_DATABITS_16:
            swapData = SPI_SwapData_16B;
            break;
        case DEV_SPI_DATABITS_32:
            swapData = SPI_SwapData_32B;
            break;
        default:
            swapData = SPI_SwapData_8B;
            break;
    }

    uint32_t idx;
    for (idx = 0; idx < cnt; idx++) {
        swapData(hspi, &bitSwap, txbuff, rxbuff, idx);
    }

    return (MDS_EOK);
}

/* Driver ------------------------------------------------------------------ */
static MDS_Err_t DDRV_SPI_Control(const DEV_SPI_Adaptr_t *spi, MDS_Item_t cmd, void *arg)
{
    DRV_SPI_SimulateHandle_t *hspi = (DRV_SPI_SimulateHandle_t *)(spi->handle);

    switch (cmd) {
        case MDS_DEVICE_CMD_INIT:
            return (DRV_SPI_SimulateInit(hspi));
        case MDS_DEVICE_CMD_DEINIT:
            return (DRV_SPI_SimulateDeInit(hspi));
        case MDS_DEVICE_CMD_HANDLESZ:
            MDS_DEVICE_ARG_HANDLE_SIZE(arg, DRV_SPI_SimulateHandle_t);
            return (MDS_EOK);
        case MDS_DEVICE_CMD_OPEN:
        case MDS_DEVICE_CMD_CLOSE:
            return (MDS_EOK);
        default:
            break;
    }

    return (MDS_EPERM);
}

static MDS_Err_t DDRV_SPI_Transfer(const DEV_SPI_Periph_t *periph, const void *txbuff, void *rxbuff, size_t cnt)
{
    DRV_SPI_SimulateHandle_t *hspi = (DRV_SPI_SimulateHandle_t *)(periph->mount->handle);

    return (DRV_SPI_SimulateTransfer(hspi, &(periph->config), txbuff, rxbuff, cnt));
}

const DEV_SPI_Driver_t G_DRV_SPI_SIMULATE = {
    .control = DDRV_SPI_Control,
    .transfer = DDRV_SPI_Transfer,
};
