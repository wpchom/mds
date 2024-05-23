

#include "drv_sflash.h"

#define SFLASH_CMD_WRITE_ENABLE     0x06
#define SFLASH_CMD_WRITE_DISABLE    0x50
#define SFLASH_CMD_READ_DEVICE_ID   0xAB
#define SFLASH_CMD_READ_MANUFACTURE 0x90
#define SFLASH_CMD_READ_JEDEC_ID    0x9F
#define SFLASH_CMD_READ_UNIQUE_ID   0x4B
#define SFLASH_CMD_READ_DATA        0x03
#define SFLASH_CMD_FAST_READ_DATA   0x0B
#define SFLASH_CMD_PAGE_PROGRAM     0x02
#define SFLASH_CMD_SECTOR_ERASE     0x20
#define SFLASH_CMD_BLOCK_ERASE      0xD8
#define SFLASH_CMD_CHIP_ERASE       0xC7
#define SFLASH_CMD_WRITE_STATUS     0x01
#define SFLASH_CMD_READ_STATUS      0x05

MDS_Err_t DRV_SFLASH_WriteEnable(DRV_SFLASH_QSPIHandle_t *hsflash, bool enabled)
{
    DEV_QSPI_Command_t cmd = {
        .instruction = (enabled) ? (SFLASH_CMD_WRITE_ENABLE) : (SFLASH_CMD_WRITE_DISABLE),
        .instructionLine = DEV_QSPI_CMDLINE_1,
    };

    MDS_Err_t err = DEV_QSPI_PeriphOpen(hsflash->periph, hsflash->timeout);
    if (err == MDS_EOK) {
        err = DEV_QSPI_PeriphCommand(hsflash->periph, &cmd);
        DEV_QSPI_PeriphClose(hsflash->periph);
    }

    return (err);
}

const DEV_STORAGE_Driver_t G_DRV_SFLASH_QSPI = {

};

const DEV_STORAGE_Driver_t G_DRV_SFLASH_DSPI = {

};

const DEV_STORAGE_Driver_t G_DRV_SFLASH_SPI = {

};
