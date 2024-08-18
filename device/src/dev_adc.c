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
#include "dev_adc.h"

/* ADC adaptr -------------------------------------------------------------- */
MDS_Err_t DEV_ADC_AdaptrInit(DEV_ADC_Adaptr_t *adc, const char *name, const DEV_ADC_Driver_t *driver,
                             MDS_DevHandle_t *handle, const MDS_Arg_t *init)
{
    return (MDS_DevAdaptrInit((MDS_DevAdaptr_t *)adc, name, (const MDS_DevDriver_t *)driver, handle, init));
}

MDS_Err_t DEV_ADC_AdaptrDeInit(DEV_ADC_Adaptr_t *adc)
{
    return (MDS_DevAdaptrDeInit((MDS_DevAdaptr_t *)adc));
}

DEV_ADC_Adaptr_t *DEV_ADC_AdaptrCreate(const char *name, const DEV_ADC_Driver_t *driver, const MDS_Arg_t *init)
{
    return ((DEV_ADC_Adaptr_t *)MDS_DevAdaptrCreate(sizeof(DEV_ADC_Adaptr_t), name, (const MDS_DevDriver_t *)driver,
                                                    init));
}

MDS_Err_t DEV_ADC_AdaptrDestroy(DEV_ADC_Adaptr_t *adc)
{
    return (MDS_DevAdaptrDestroy((MDS_DevAdaptr_t *)adc));
}

/* ADC periph -------------------------------------------------------------- */
MDS_Err_t DEV_ADC_PeriphInit(DEV_ADC_Periph_t *periph, const char *name, DEV_ADC_Adaptr_t *adc)
{
    MDS_Err_t err = MDS_DevPeriphInit((MDS_DevPeriph_t *)periph, name, (MDS_DevAdaptr_t *)adc);
    if (err == MDS_EOK) {
        periph->object.optick = MDS_DEVICE_PERIPH_TIMEOUT;
    }

    return (err);
}

MDS_Err_t DEV_ADC_PeriphDeInit(DEV_ADC_Periph_t *periph)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)periph));
}

DEV_ADC_Periph_t *DEV_ADC_PeriphCreate(const char *name, DEV_ADC_Adaptr_t *adc)
{
    DEV_ADC_Periph_t *periph = (DEV_ADC_Periph_t *)MDS_DevPeriphCreate(sizeof(DEV_ADC_Periph_t), name,
                                                                       (MDS_DevAdaptr_t *)adc);
    if (periph != NULL) {
        periph->object.optick = MDS_DEVICE_PERIPH_TIMEOUT;
    }

    return (periph);
}

MDS_Err_t DEV_ADC_PeriphDestroy(DEV_ADC_Periph_t *periph)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)periph));
}

MDS_Err_t DEV_ADC_PeriphOpen(DEV_ADC_Periph_t *periph, MDS_Tick_t timeout)
{
    return (MDS_DevPeriphOpen((MDS_DevPeriph_t *)periph, timeout));
}

MDS_Err_t DEV_ADC_PeriphClose(DEV_ADC_Periph_t *periph)
{
    return (MDS_DevPeriphClose((MDS_DevPeriph_t *)periph));
}

MDS_Err_t DEV_ADC_PeriphConvert(DEV_ADC_Periph_t *periph, uint32_t *value, uint32_t *voltage)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->convert != NULL);

    const DEV_ADC_Adaptr_t *adc = periph->mount;

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    uint32_t times = 1;
    uint32_t sum, max, min, val;
    MDS_Err_t err = adc->driver->convert(periph, &sum);
    if (err == MDS_EOK) {
        for (max = min = sum; times < periph->object.averages; times++) {
            err = adc->driver->convert(periph, &val);
            if (err != MDS_EOK) {
                break;
            }
            if (val > max) {
                max = val;
            } else if (val < min) {
                min = val;
            }
            sum += val;
        }
    }

    val = (times > 0x2U) ? ((sum - max - min) / (times - 0x2U)) : (sum / times);
    if (value != NULL) {
        *value = val;
    }
    if (voltage != NULL) {
        *voltage = ((uint64_t)(val + 1) * periph->mount->refVoltage) >> periph->object.resolution;
    }

    return (err);
}
