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
#ifndef __DRV_LED_SIMULATE_H__
#define __DRV_LED_SIMULATE_H__

/* Include ----------------------------------------------------------------- */
#include "dev_led.h"
#include "dev_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct DRV_LED_PwmInitStruct {
    DEV_TIMER_Device_t *timer;
    DEV_TIMER_OC_Channel_t *oc[DEV_LED_COLOR_NUMS];
    uint32_t freq;
} DRV_LED_PwmInitStruct_t;

typedef struct DRV_LED_PwmHandle {
    DRV_LED_PwmInitStruct_t init;
    volatile uint32_t count;
    struct {
        DEV_LED_Color_t color;
        DEV_LED_Light_t light;
    } curr, next;
} DRV_LED_PwmHandle_t;

/* Funtcion ---------------------------------------------------------------- */
extern MDS_Err_t DRV_LED_PwmInit(DRV_LED_PwmHandle_t *hled, const DRV_LED_PwmInitStruct_t *init);
extern MDS_Err_t DRV_LED_PwmDeInit(DRV_LED_PwmHandle_t *hled);
extern MDS_Err_t DRV_LED_PwmLight(DRV_LED_PwmHandle_t *hled, const DEV_LED_Color_t *color,
                                  const DEV_LED_Light_t *light);

/* Driver ------------------------------------------------------------------ */
extern const DEV_LED_Driver_t G_DRV_LED_PWM;

#ifdef __cplusplus
}
#endif

#endif /* __DRV_LED_SIMULATE_H__ */
