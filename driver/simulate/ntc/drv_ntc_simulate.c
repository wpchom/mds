/**
 * Copyright (c) [2022] [pchom]
 * [MDS_FS] is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 **/
/* Include ----------------------------------------------------------------- */
#include "drv_ntc_simulate.h"
#include <math.h>

/* Define ------------------------------------------------------------------ */
#define DRV_NTC_TEMPERATURE_K 273.15f
#define DRV_NTC_CALC_LOG(x)   _Generic((x), float: logf(x), default: log(x))
#define DRV_NTC_CALC_ROUND(x) _Generic((x), float: roundf(x), default: round(x))

/* Function ---------------------------------------------------------------- */
// Rt = R0 * exp(B * (1/T - 1/T0))
MDS_Err_t DRV_NTC_CalcMathValue(const DRV_NTC_Handle_t *hntc, DEV_NTC_Value_t *value)
{
    MDS_ASSERT(hntc != NULL);
    MDS_ASSERT(hntc->math != NULL);
    MDS_ASSERT(value != NULL);

    uint32_t adcVoltage = 0;
    float_t current, voltage, resistance, temperature;
    MDS_Err_t err = DEV_ADC_PeriphOpen(hntc->adcPeriph, MDS_TICK_FOREVER);
    if (err == MDS_EOK) {
        err = DEV_ADC_PeriphConvert(hntc->adcPeriph, NULL, &adcVoltage);
        DEV_ADC_PeriphClose(hntc->adcPeriph);
    }

    adcVoltage = VALUE_RANGE(adcVoltage, 0, hntc->refVoltage);

    if (hntc->adcSample == DRV_NTC_ADC_SAMPLE_UP) {
        current = (float_t)(hntc->refVoltage - adcVoltage) / (hntc->resUp);
    } else if (hntc->adcSample == DRV_NTC_ADC_SAMPLE_HIGH) {
        current = (float_t)(hntc->refVoltage - adcVoltage) / (hntc->resUp + hntc->resHigh);
    } else if (hntc->adcSample == DRV_NTC_ADC_SAMPLE_LOW) {
        current = (float_t)(adcVoltage) / (hntc->resLow + hntc->resDown);
    } else if (hntc->adcSample == DRV_NTC_ADC_SAMPLE_DOWN) {
        current = (float_t)(adcVoltage) / (hntc->resDown);
    } else {
        return (MDS_EINVAL);
    }

    voltage = (float_t)(hntc->refVoltage) - current * (hntc->resUp + hntc->resHigh + hntc->resLow + hntc->resDown);
    resistance = voltage / ((current == 0) ? (1) : (current));
    temperature = (1.0f / (DRV_NTC_CALC_LOG(resistance / hntc->math->r0) / hntc->math->bx + (1.0f / hntc->math->t0))) -
                  DRV_NTC_TEMPERATURE_K;

    value->resistance = DRV_NTC_CALC_ROUND(resistance);
    value->temperature = DRV_NTC_CALC_ROUND(temperature);

    return (err);
}

static MDS_Err_t DDRV_NTC_ControlMath(const DEV_NTC_Device_t *ntc, MDS_Item_t cmd, MDS_Arg_t *arg)
{
    switch (cmd) {
        case MDS_DEVICE_CMD_INIT:
        case MDS_DEVICE_CMD_DEINIT:
            return (MDS_EOK);
        case MDS_DEVICE_CMD_HANDLESZ:
            MDS_DEVICE_ARG_HANDLE_SIZE(arg, DRV_NTC_Handle_t);
            return (MDS_EOK);
        default:
            break;
    }

    switch (cmd) {
        case DEV_NTC_CMD_GET_VALUE:
            return (DRV_NTC_CalcMathValue((DRV_NTC_Handle_t *)(ntc->handle), (DEV_NTC_Value_t *)arg));
        default:
            break;
    }

    return (MDS_EPERM);
}

MDS_Err_t DRV_NTC_SearchTableValue(const DRV_NTC_Handle_t *hntc, DEV_NTC_Value_t *value)
{
    MDS_ASSERT(hntc != NULL);
    MDS_ASSERT(hntc->math != NULL);
    MDS_ASSERT(value != NULL);

    uint32_t adcVoltage = 0;
    uint16_t tIdx = 0;
    uint32_t current, voltage, resistance;
    MDS_Err_t err = DEV_ADC_PeriphOpen(hntc->adcPeriph, MDS_TICK_FOREVER);
    if (err == MDS_EOK) {
        err = DEV_ADC_PeriphConvert(hntc->adcPeriph, NULL, &adcVoltage);
        DEV_ADC_PeriphClose(hntc->adcPeriph);
    }

    adcVoltage = VALUE_RANGE(adcVoltage, 0, hntc->refVoltage);

    if (hntc->adcSample == DRV_NTC_ADC_SAMPLE_UP) {
        current = (uint32_t)(hntc->refVoltage - adcVoltage) / (hntc->resUp);
    } else if (hntc->adcSample == DRV_NTC_ADC_SAMPLE_HIGH) {
        current = (uint32_t)(hntc->refVoltage - adcVoltage) / (hntc->resUp + hntc->resHigh);
    } else if (hntc->adcSample == DRV_NTC_ADC_SAMPLE_LOW) {
        current = (uint32_t)(adcVoltage) / (hntc->resLow + hntc->resDown);
    } else if (hntc->adcSample == DRV_NTC_ADC_SAMPLE_DOWN) {
        current = (uint32_t)(adcVoltage) / (hntc->resDown);
    } else {
        return (MDS_EINVAL);
    }

    voltage = (uint32_t)(hntc->refVoltage) - current * (hntc->resUp + hntc->resHigh + hntc->resLow + hntc->resDown);
    resistance = voltage / ((current == 0) ? (1) : (current));
    while ((tIdx < (hntc->table->count - 1)) && ((uint32_t)(hntc->table->res[tIdx + 1]) >= resistance)) {
        tIdx += 1;
    }

    value->resistance = resistance;
    value->temperature = tIdx + hntc->table->base;

    return (err);
}

static MDS_Err_t DDRV_NTC_ControlTable(const DEV_NTC_Device_t *ntc, MDS_Item_t cmd, MDS_Arg_t *arg)
{
    switch (cmd) {
        case MDS_DEVICE_CMD_INIT:
        case MDS_DEVICE_CMD_DEINIT:
            return (MDS_EOK);
        case MDS_DEVICE_CMD_HANDLESZ:
            MDS_DEVICE_ARG_HANDLE_SIZE(arg, DRV_NTC_Handle_t);
            return (MDS_EOK);
        default:
            break;
    }

    switch (cmd) {
        case DEV_NTC_CMD_GET_VALUE:
            return (DRV_NTC_SearchTableValue((DRV_NTC_Handle_t *)(ntc->handle), (DEV_NTC_Value_t *)arg));
        default:
            break;
    }

    return (MDS_EPERM);
}

const DEV_NTC_Driver_t DRV_NTC_MATH = {
    .control = DDRV_NTC_ControlMath,
};

const DEV_NTC_Driver_t DRV_NTC_TABLE = {
    .control = DDRV_NTC_ControlTable,
};
