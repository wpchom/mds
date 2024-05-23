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
#ifndef __DRV_NTC_SIMULATE_H__
#define __DRV_NTC_SIMULATE_H__

/* Include ----------------------------------------------------------------- */
#include "extend/dev_ntc.h"
#include "dev_adc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Verf    ┳
 *         ▉    resUp
 *         ┣━━━━━━━━━━━━  ADC sample UP
 *         ▉    resHigh
 *         ┣━━━━━━━━━━━━  ADC sample HIGH
 *         ▉    resNTC
 *         ┣━━━━━━━━━━━━  ADC sample LOW
 *         ▉    resLow
 *         ┣━━━━━━━━━━━━  ADC sample DOWN
 *         ▉    resDown
 * GND     ┻
 **/

/* Typedef ----------------------------------------------------------------- */
typedef struct DRV_NTC_MathPara {
    float_t bx;
    float_t r0;
    float_t t0;
} DRV_NTC_MathPara_t;

typedef struct DRV_NTC_TablePara {
    int16_t base;
    uint16_t count;
    uint32_t *const res;
} DRV_NTC_TablePara_t;

typedef enum DRV_NTC_AdcSample {
    DRV_NTC_ADC_SAMPLE_UP,
    DRV_NTC_ADC_SAMPLE_HIGH,
    DRV_NTC_ADC_SAMPLE_LOW,
    DRV_NTC_ADC_SAMPLE_DOWN,
} DRV_NTC_AdcSample_t;

typedef struct DRV_NTC_Handle {
    DEV_ADC_Periph_t *adcPeriph;
    uint32_t refVoltage;  // mV
    uint32_t resUp;       // ohm
    uint32_t resHigh;     // ohm
    uint32_t resLow;      // ohm
    uint32_t resDown;     // ohm
    DRV_NTC_AdcSample_t adcSample;
    int32_t (*ntcCompensation)(int32_t data);
    union {
        const DRV_NTC_MathPara_t *math;
        const DRV_NTC_TablePara_t *table;
    };
} DRV_NTC_Handle_t;

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DRV_NTC_CalcMathValue(const DRV_NTC_Handle_t *handle, DEV_NTC_Value_t *value);
extern MDS_Err_t DRV_NTC_SearchTableValue(const DRV_NTC_Handle_t *handle, DEV_NTC_Value_t *value);

/* Driver ------------------------------------------------------------------ */
extern const DEV_NTC_Driver_t DRV_NTC_MATH;
extern const DEV_NTC_Driver_t DRV_NTC_TABLE;

#ifdef __cplusplus
}
#endif

#endif /* __DRV_NTC_SIMULATE_H__ */
