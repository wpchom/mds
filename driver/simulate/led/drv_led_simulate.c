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
#include "drv_led_simulate.h"

/* Variable ----------------------------------------------------------------- */
__attribute__((weak)) const MDS_DevProbeId_t G_DRV_LED_SIMULATE_ID = {
    .number = 0x00,
    .name = "PWM",
};

/* Function ---------------------------------------------------------------- */
static MDS_Err_t LED_PwmColor(DRV_LED_PwmHandle_t *hled, uint32_t nemo, uint32_t deno)
{
    MDS_Err_t err = MDS_EOK;

    for (size_t idx = 0; idx < ARRAY_SIZE(hled->init.oc); idx++) {
        if (hled->init.oc[idx] != NULL) {
            size_t brightOn = (size_t)(hled->curr.color.bright[idx]) * hled->curr.light.levelOn;
            size_t brightOff = (size_t)(hled->curr.color.bright[idx]) * hled->curr.light.levelOff;
            size_t brightMax = (size_t)((DEV_LED_Bright_t)(-1)) * (size_t)((DEV_LED_Bright_t)(-1));
            size_t brightOfs = (deno != 0) ? ((brightOn - brightOff) * nemo / deno) : (0);
            MDS_Err_t ret = DEV_TIMER_OC_ChannelDuty(hled->init.oc[idx], brightOfs + brightOff, brightMax);
            if (ret != MDS_EOK) {
                err = ret;
            }
        }
    }

    return (err);
}

static void LED_PwmLightCallback(const DEV_TIMER_Device_t *timer, MDS_Arg_t *arg)
{
    UNUSED(timer);

    DRV_LED_PwmHandle_t *hled = (DRV_LED_PwmHandle_t *)arg;
    uint32_t ms = hled->count * MDS_TIME_MSEC_OF_SEC / hled->init.freq;

    hled->count += 1;
    if (ms < hled->curr.light.fadeOff) {
        LED_PwmColor(hled, hled->curr.light.fadeOff - ms, hled->curr.light.fadeOff);
        return;
    }

    if (memcmp(&(hled->curr), &(hled->next), sizeof(hled->curr)) != 0) {
        hled->curr = hled->next;
        hled->count = hled->curr.light.fadeOff * hled->init.freq / MDS_TIME_MSEC_OF_SEC;
        ms = 0;
    } else {
        ms -= hled->curr.light.fadeOff;
    }

    ms -= hled->curr.light.fadeOff;
    hled->curr = hled->next;
    if (ms < hled->curr.light.fadeOn) {
        LED_PwmColor(hled, ms, hled->curr.light.fadeOn);
        return;
    }

    if (hled->curr.light.fullOff == 0) {
        LED_PwmColor(hled, 1, 1);
        DEV_TIMER_DeviceCallback(hled->init.timer, NULL, NULL);
        return;
    }
    ms -= hled->curr.light.fadeOn;
    if (ms < hled->curr.light.fullOn) {
        LED_PwmColor(hled, 1, 1);
        return;
    }

    ms -= hled->curr.light.fullOn;
    if (ms < hled->curr.light.fadeOff) {
        LED_PwmColor(hled, hled->curr.light.fadeOff - ms, hled->curr.light.fadeOff);
        return;
    }

    if (hled->curr.light.fullOn == 0) {
        LED_PwmColor(hled, 0, 1);
        DEV_TIMER_DeviceStop(hled->init.timer);
        DEV_TIMER_DeviceCallback(hled->init.timer, NULL, NULL);
        return;
    }
    ms -= hled->curr.light.fadeOff;
    if (ms < hled->curr.light.fullOff) {
        LED_PwmColor(hled, 0, 1);
        return;
    }

    hled->count = hled->curr.light.fadeOff * hled->init.freq / MDS_TIME_MSEC_OF_SEC;
}

MDS_Err_t DRV_LED_PwmLight(DRV_LED_PwmHandle_t *hled, const DEV_LED_Color_t *color, const DEV_LED_Light_t *light)
{
    MDS_ASSERT(hled != NULL);

    DEV_TIMER_DeviceStop(hled->init.timer);

    hled->next.color = *color;
    hled->next.light = *light;

    if (memcmp(&(hled->curr.color), &(hled->next.color), sizeof(hled->curr.color)) != 0) {
        uint32_t ofs;
        uint32_t ms = hled->count * MDS_TIME_MSEC_OF_SEC / hled->init.freq;
        do {
            if (ms < hled->curr.light.fadeOff) {
                ofs = ms;
                break;
            }
            ms -= hled->curr.light.fadeOff;
            if (ms < hled->curr.light.fadeOn) {
                ofs = hled->curr.light.fadeOff - (ms * hled->curr.light.fadeOff / hled->curr.light.fadeOn);
                break;
            }
            ms -= hled->curr.light.fadeOn;
            if (ms < hled->curr.light.fullOn) {
                ofs = 0;
                break;
            }
            ms -= hled->curr.light.fullOn;
            if (ms < hled->curr.light.fadeOff) {
                ofs = ms;
                break;
            }
            ofs = hled->curr.light.fadeOff;
        } while (0);
        hled->count = ofs * hled->init.freq / MDS_TIME_MSEC_OF_SEC;
    }

    DEV_TIMER_DeviceCallback(hled->init.timer, LED_PwmLightCallback, (MDS_Arg_t *)hled);

    return (DEV_TIMER_DeviceStart(hled->init.timer));
}

MDS_Err_t DRV_LED_PwmInit(DRV_LED_PwmHandle_t *hled, const DRV_LED_PwmInitStruct_t *init)
{
    MDS_ASSERT(hled != NULL);

    if (init != NULL) {
        hled->init = *init;
    }

    if (hled->init.timer == NULL) {
        return (MDS_EINVAL);
    }

    static const DEV_TIMER_Config_t config = {
        .type = DEV_TIMER_COUNTERTYPE_HZ,
        .mode = DEV_TIMER_COUNTERMODE_UP,
        .period = DEV_TIMER_PERIOD_ENABLE,
    };

    for (size_t idx = 0; idx < ARRAY_SIZE(hled->init.oc); idx++) {
        if (hled->init.oc[idx] != NULL) {
            DEV_TIMER_OC_ChannelDuty(hled->init.oc[idx], 0, 1);
            DEV_TIMER_OC_ChannelEnable(hled->init.oc[idx], true);
        }
    }

    return (DEV_TIMER_DeviceConfig(hled->init.timer, &config, hled->init.freq));
}

MDS_Err_t DRV_LED_PwmDeInit(DRV_LED_PwmHandle_t *hled)
{
    MDS_ASSERT(hled != NULL);

    return (DEV_TIMER_DeviceStop(hled->init.timer));
}

/* Driver ------------------------------------------------------------------ */
static MDS_Err_t DDRV_LED_Control(const DEV_LED_Device_t *led, MDS_Item_t cmd, MDS_Arg_t *arg)
{
    DRV_LED_PwmHandle_t *hled = (DRV_LED_PwmHandle_t *)(led->handle);

    switch (cmd) {
        case MDS_DEVICE_CMD_INIT:
            return (DRV_LED_PwmInit(hled, (const DRV_LED_PwmInitStruct_t *)arg));

        case MDS_DEVICE_CMD_DEINIT:
            return (DRV_LED_PwmDeInit(hled));

        case MDS_DEVICE_CMD_HANDLESZ:
            MDS_DEVICE_ARG_HANDLE_SIZE(arg, DRV_LED_PwmHandle_t);
            return (MDS_EOK);

        case MDS_DEVICE_CMD_GETID:
            if (arg != NULL) {
                *((const MDS_DevProbeId_t **)arg) = &G_DRV_LED_SIMULATE_ID;
            }
            return (MDS_EOK);

        default:
            break;
    }

    return (MDS_EPERM);
}

static MDS_Err_t DDRV_LED_Light(const DEV_LED_Device_t *led, const DEV_LED_Color_t *color, const DEV_LED_Light_t *light)
{
    return (DRV_LED_PwmLight((DRV_LED_PwmHandle_t *)(led->handle), color, light));
}

const DEV_LED_Driver_t G_DRV_LED_PWM = {
    .control = DDRV_LED_Control,
    .light = DDRV_LED_Light,
};
