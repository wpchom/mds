/**
 * @copyright   Copyright (c) 2024 Pchom & licensed under Mulan PSL v2
 * @file        drv_chip.h
 * @brief       stm32f1xx chip driver for mds device
 * @date        2024-05-30
 */
#ifndef __DRV_CHIP_H__
#define __DRV_CHIP_H__

/* Include ----------------------------------------------------------------- */
#include "mds_sys.h"
#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Function ---------------------------------------------------------------- */
static inline MDS_Err_t DRV_HalStatusToMdsErr(HAL_StatusTypeDef status)
{
    if (status == HAL_OK) {
        return (MDS_EOK);
    } else if (status == HAL_BUSY) {
        return (MDS_EBUSY);
    } else if (status == HAL_TIMEOUT) {
        return (MDS_ETIME);
    } else {
        return (MDS_EIO);
    }
}

static inline MDS_Tick_t DRV_HalGetTick(void)
{
    uint32_t count = 0;

    if ((DCB->DEMCR & DCB_DEMCR_TRCENA_Msk) == 0U) {
        MDS_Item_t lock = MDS_CoreInterruptLock();
        DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        MDS_CoreInterruptRestore(lock);
    } else {
        count = DWT->CYCCNT / (SystemCoreClock / MDS_SYSTICK_FREQ_HZ);
    }

    return (count);
}

#ifdef __cplusplus
}
#endif

#endif /* __DRV_CHIP_H__ */
