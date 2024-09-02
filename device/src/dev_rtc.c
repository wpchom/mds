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
#include "dev_rtc.h"

/* RTC device -------------------------------------------------------------- */
MDS_Err_t DEV_RTC_DeviceInit(DEV_RTC_Device_t *rtc, const char *name, const DEV_RTC_Driver_t *driver,
                             MDS_DevHandle_t *handle, const MDS_Arg_t *init)
{
    return (MDS_DevModuleInit((MDS_DevModule_t *)rtc, name, (const MDS_DevDriver_t *)driver, handle, init));
}

MDS_Err_t DEV_RTC_DeviceDeInit(DEV_RTC_Device_t *rtc)
{
    return (MDS_DevModuleDeInit((MDS_DevModule_t *)rtc));
}

DEV_RTC_Device_t *DEV_RTC_DeviceCreate(const char *name, const DEV_RTC_Driver_t *driver, const MDS_Arg_t *init)
{
    return ((DEV_RTC_Device_t *)MDS_DevModuleCreate(sizeof(DEV_RTC_Device_t), name, (const MDS_DevDriver_t *)driver,
                                                    init));
}

MDS_Err_t DEV_RTC_DeviceDestroy(DEV_RTC_Device_t *rtc)
{
    return (MDS_DevModuleDestroy((MDS_DevModule_t *)rtc));
}

MDS_Err_t DEV_RTC_GetTimeDate(DEV_RTC_Device_t *rtc, MDS_TimeDate_t *tm)
{
    MDS_ASSERT(rtc != NULL);
    MDS_ASSERT(rtc->driver != NULL);
    MDS_ASSERT(rtc->driver->control != NULL);

    return (rtc->driver->control(rtc, DEV_RTC_CMD_TIMEDATE_GET, (MDS_Arg_t *)tm));
}

MDS_Err_t DEV_RTC_SetTimeDate(DEV_RTC_Device_t *rtc, const MDS_TimeDate_t *tm)
{
    MDS_ASSERT(rtc != NULL);
    MDS_ASSERT(rtc->driver != NULL);
    MDS_ASSERT(rtc->driver->control != NULL);

    return (rtc->driver->control(rtc, DEV_RTC_CMD_TIMEDATE_SET, (MDS_Arg_t *)tm));
}

MDS_Err_t DEV_RTC_GetTimeStamp(DEV_RTC_Device_t *rtc, MDS_Time_t *timestamp)
{
    MDS_ASSERT(rtc != NULL);
    MDS_ASSERT(rtc->driver != NULL);
    MDS_ASSERT(rtc->driver->control != NULL);

    return (rtc->driver->control(rtc, DEV_RTC_CMD_TIMESTAMP_GET, (MDS_Arg_t *)(&timestamp)));
}

MDS_Err_t DEV_RTC_SetTimeStamp(DEV_RTC_Device_t *rtc, MDS_Time_t timestamp)
{
    MDS_ASSERT(rtc != NULL);
    MDS_ASSERT(rtc->driver != NULL);
    MDS_ASSERT(rtc->driver->control != NULL);

    return (rtc->driver->control(rtc, DEV_RTC_CMD_TIMESTAMP_SET, (MDS_Arg_t *)(&timestamp)));
}

/* RTC Timer --------------------------------------------------------------- */
MDS_Err_t DEV_RTC_TimerInit(DEV_RTC_Timer_t *timer, const char *name, DEV_RTC_Device_t *rtc)
{
    return (MDS_DevPeriphInit((MDS_DevPeriph_t *)timer, name, (MDS_DevAdaptr_t *)rtc));
}

MDS_Err_t DEV_RTC_TimerDeInit(DEV_RTC_Timer_t *timer)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)timer));
}

DEV_RTC_Timer_t *DEV_RTC_TimerCreate(const char *name, DEV_RTC_Device_t *rtc)
{
    return ((DEV_RTC_Timer_t *)MDS_DevPeriphCreate(sizeof(DEV_RTC_Timer_t), name, (MDS_DevAdaptr_t *)rtc));
}

MDS_Err_t DEV_RTC_TimerDestroy(DEV_RTC_Timer_t *timer)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)timer));
}

void DEV_RTC_TimerCallback(DEV_RTC_Timer_t *timer, void (*callback)(DEV_RTC_Timer_t *, MDS_Arg_t *), MDS_Arg_t *arg)
{
    MDS_ASSERT(timer != NULL);

    timer->callback = callback;
    timer->arg = arg;
}

MDS_Err_t DEV_RTC_TimerStart(DEV_RTC_Timer_t *timer, MDS_Tick_t timeout)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(timer->mount != NULL);
    MDS_ASSERT(timer->mount->driver != NULL);
    MDS_ASSERT(timer->mount->driver->timer != NULL);

    return (timer->mount->driver->timer(timer, timeout));
}

MDS_Err_t DEV_RTC_TimerStop(DEV_RTC_Timer_t *timer)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(timer->mount != NULL);
    MDS_ASSERT(timer->mount->driver != NULL);
    MDS_ASSERT(timer->mount->driver->timer != NULL);

    return (timer->mount->driver->timer(timer, 0));
}

MDS_Tick_t DEV_RTC_TimerGetCount(DEV_RTC_Timer_t *timer)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(timer->mount != NULL);
    MDS_ASSERT(timer->mount->driver != NULL);
    MDS_ASSERT(timer->mount->driver->count != NULL);

    return (timer->mount->driver->count(timer));
}

/* RTC Alarm --------------------------------------------------------------- */
MDS_Err_t DEV_RTC_AlarmInit(DEV_RTC_Alarm_t *alarm, const char *name, DEV_RTC_Device_t *rtc)
{
    return (MDS_DevPeriphInit((MDS_DevPeriph_t *)alarm, name, (MDS_DevAdaptr_t *)rtc));
}

MDS_Err_t DEV_RTC_AlarmDeInit(DEV_RTC_Alarm_t *alarm)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)alarm));
}

DEV_RTC_Alarm_t *DEV_RTC_AlarmCreate(const char *name, DEV_RTC_Device_t *rtc)
{
    return ((DEV_RTC_Alarm_t *)MDS_DevPeriphCreate(sizeof(DEV_RTC_Alarm_t), name, (MDS_DevAdaptr_t *)rtc));
}

MDS_Err_t DEV_RTC_AlarmDestroy(DEV_RTC_Alarm_t *alarm)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)alarm));
}

void DEV_RTC_AlarmCallback(DEV_RTC_Alarm_t *alarm, void (*callback)(DEV_RTC_Alarm_t *, MDS_Arg_t *), MDS_Arg_t *arg)
{
    MDS_ASSERT(alarm != NULL);

    alarm->callback = callback;
    alarm->arg = arg;
}

MDS_Err_t DEV_RTC_AlarmEnable(DEV_RTC_Alarm_t *alarm, const MDS_TimeDate_t *tm)
{
    MDS_ASSERT(alarm != NULL);
    MDS_ASSERT(alarm->mount != NULL);
    MDS_ASSERT(alarm->mount->driver != NULL);
    MDS_ASSERT(alarm->mount->driver->alarm != NULL);

    return (alarm->mount->driver->alarm(alarm, tm));
}

MDS_Err_t DEV_RTC_AlarmDisable(DEV_RTC_Alarm_t *alarm)
{
    MDS_ASSERT(alarm != NULL);
    MDS_ASSERT(alarm->mount != NULL);
    MDS_ASSERT(alarm->mount->driver != NULL);
    MDS_ASSERT(alarm->mount->driver->alarm != NULL);

    return (alarm->mount->driver->alarm(alarm, NULL));
}
