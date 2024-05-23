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
#ifndef __DEV_RTC_H__
#define __DEV_RTC_H__

/* Include ----------------------------------------------------------------- */
#include "mds_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct DEV_RTC_Device DEV_RTC_Device_t;
typedef struct DEV_RTC_Timer DEV_RTC_Timer_t;
typedef struct DEV_RTC_Alarm DEV_RTC_Alarm_t;

enum DEV_RTC_Cmd {
    DEV_RTC_CMD_TIMEDATE_GET = MDS_DEVICE_CMD_DRIVER,
    DEV_RTC_CMD_TIMEDATE_SET,
    DEV_RTC_CMD_TIMESTAMP_GET,
    DEV_RTC_CMD_TIMESTAMP_SET,
};

typedef struct DEV_RTC_Driver {
    MDS_Err_t (*control)(const DEV_RTC_Device_t *rtc, MDS_Item_t cmd, MDS_Arg_t *arg);
    MDS_Err_t (*timer)(const DEV_RTC_Timer_t *timer, MDS_Tick_t timeout);
    MDS_Tick_t (*count)(const DEV_RTC_Timer_t *timer);
    MDS_Err_t (*alarm)(const DEV_RTC_Alarm_t *alarm, const MDS_TimeDate_t *tm);
} DEV_RTC_Driver_t;

struct DEV_RTC_Device {
    const MDS_Device_t device;
    const DEV_RTC_Driver_t *driver;
    const MDS_DevHandle_t *handle;
};

struct DEV_RTC_Timer {
    const MDS_Device_t device;
    const DEV_RTC_Device_t *mount;

    void (*callback)(const DEV_RTC_Timer_t *timer, MDS_Arg_t *arg);
    MDS_Arg_t *arg;
};

struct DEV_RTC_Alarm {
    const MDS_Device_t device;
    const DEV_RTC_Device_t *mount;

    void (*callback)(const DEV_RTC_Alarm_t *alarm, MDS_Arg_t *arg);
    MDS_Arg_t *arg;
};

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DEV_RTC_DeviceInit(DEV_RTC_Device_t *rtc, const char *name, const DEV_RTC_Driver_t *driver,
                                    MDS_DevHandle_t *handle, const MDS_Arg_t *init);
extern MDS_Err_t DEV_RTC_DeviceDeInit(DEV_RTC_Device_t *rtc);
extern DEV_RTC_Device_t *DEV_RTC_DeviceCreate(const char *name, const DEV_RTC_Driver_t *driver, const MDS_Arg_t *init);
extern MDS_Err_t DEV_RTC_DeviceDestroy(DEV_RTC_Device_t *rtc);

extern MDS_Err_t DEV_RTC_GetTimeDate(DEV_RTC_Device_t *rtc, MDS_TimeDate_t *tm);
extern MDS_Err_t DEV_RTC_SetTimeDate(DEV_RTC_Device_t *rtc, const MDS_TimeDate_t *tm);
extern MDS_Err_t DEV_RTC_GetTimeStamp(DEV_RTC_Device_t *rtc, MDS_Time_t *timestamp);
extern MDS_Err_t DEV_RTC_SetTimeStamp(DEV_RTC_Device_t *rtc, MDS_Time_t timestamp);

extern MDS_Err_t DEV_RTC_TimerInit(DEV_RTC_Timer_t *timer, const char *name, DEV_RTC_Device_t *rtc);
extern MDS_Err_t DEV_RTC_TimerDeInit(DEV_RTC_Timer_t *timer);
extern DEV_RTC_Timer_t *DEV_RTC_TimerCreate(const char *name, DEV_RTC_Device_t *rtc);
extern MDS_Err_t DEV_RTC_TimerDestroy(DEV_RTC_Timer_t *timer);

extern void DEV_RTC_TimerCallback(DEV_RTC_Timer_t *timer, void (*callback)(const DEV_RTC_Timer_t *, MDS_Arg_t *),
                                  MDS_Arg_t *arg);
extern MDS_Err_t DEV_RTC_TimerStart(DEV_RTC_Timer_t *timer, MDS_Tick_t timeout);
extern MDS_Err_t DEV_RTC_TimerStop(DEV_RTC_Timer_t *timer);
extern MDS_Tick_t DEV_RTC_TimerGetCount(DEV_RTC_Timer_t *timer);

extern MDS_Err_t DEV_RTC_AlarmInit(DEV_RTC_Alarm_t *alarm, const char *name, DEV_RTC_Device_t *rtc);
extern MDS_Err_t DEV_RTC_AlarmDeInit(DEV_RTC_Alarm_t *alarm);
extern DEV_RTC_Alarm_t *DEV_RTC_AlarmCreate(const char *name, DEV_RTC_Device_t *rtc);
extern MDS_Err_t DEV_RTC_AlarmDestroy(DEV_RTC_Alarm_t *alarm);

extern void DEV_RTC_AlarmCallback(DEV_RTC_Alarm_t *alarm, void (*callback)(const DEV_RTC_Alarm_t *, MDS_Arg_t *),
                                  MDS_Arg_t *arg);
extern MDS_Err_t DEV_RTC_AlarmEnable(DEV_RTC_Alarm_t *alarm, const MDS_TimeDate_t *tm);
extern MDS_Err_t DEV_RTC_AlarmDisable(DEV_RTC_Alarm_t *alarm);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_RTC_H__ */
