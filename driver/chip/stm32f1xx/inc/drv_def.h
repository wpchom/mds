#include "mds_def.h"
#include "stm32f1xx_hal_def.h"

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
