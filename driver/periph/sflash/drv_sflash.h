#ifndef __DRV_SFLASH_H__
#define __DRV_SFLASH_H__

#include "dev_storage.h"
#include "dev_qspi.h"
#include "dev_spi.h"

typedef struct DRV_SFLASH_QSPIHandle {
    DEV_QSPI_Periph_t *periph;
    MDS_Tick_t timeout;

    int8_t addrWord;
} DRV_SFLASH_QSPIHandle_t;

#endif /* __DRV_SFLASH_H__ */
