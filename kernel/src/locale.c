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
/* Include ---------------------------------------------------------------- */
#include "mds_sys.h"

/* Define ------------------------------------------------------------------ */
#ifndef MDS_TIME_DEFAULT_TZ
#define MDS_TIME_DEFAULT_TZ (+8)
#endif

#ifndef MDS_TIME_DEFAULT_TS
#define MDS_TIME_DEFAULT_TS 1577836800000LL
#endif

#define TIME_UNIX_YEAR_BEGIN    1970
#define TIME_UNIX_WEEKDAY_BEGIN 4

#define TIME_IS_LEAPYEAR(year)                                                                                         \
    (((((year) % 400) == 0) || ((((year) % 4) == 0) && (((year) % 100) != 0))) ? (true) : (false))

#define TIME_GET_YEARDAYS(year)                                                                                        \
    ((((year) - (TIME_UNIX_YEAR_BEGIN) + (1)) / 4) + (((year) - (TIME_UNIX_YEAR_BEGIN) - (1)) / 100) +                 \
     (((year) - (TIME_UNIX_YEAR_BEGIN) + (299)) / 400))

/* Variable ---------------------------------------------------------------- */
static const int16_t G_YDAYS_OF_MONTH[] = {-1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364};

static MDS_Time_t g_unixTimeBase = MDS_TIME_DEFAULT_TS;
static int8_t g_localTimeZone = MDS_TIME_DEFAULT_TZ;

/* Function ---------------------------------------------------------------- */
MDS_Time_t MDS_TIME_ChangeTimeStamp(MDS_TimeDate_t *tm, int8_t tz)
{
    MDS_Time_t tmp = ((tm->year - TIME_UNIX_YEAR_BEGIN) * MDS_TIME_DAY_OF_YEAR) + TIME_GET_YEARDAYS(tm->year);

    tm->yday = tm->mday + G_YDAYS_OF_MONTH[tm->month - 1];
    if ((tm->month > MDS_TIME_MONTH_FEB) && (TIME_IS_LEAPYEAR(tm->year))) {
        tm->yday += 1;
    }

    tmp = (tmp + tm->yday) * MDS_TIME_SEC_OF_DAY + MDS_TIME_SEC_OF_HOUR * (tz + tm->hour) +
          MDS_TIME_SEC_OF_MIN * tm->minute + tm->second;

    return (tmp * MDS_TIME_MSEC_OF_SEC + tm->msec);
}

void MDS_TIME_ChangeTimeDate(MDS_TimeDate_t *tm, MDS_Time_t timestamp, int8_t tz)
{
    MDS_Time_t tmp = timestamp + (MDS_Time_t)tz * MDS_TIME_MSEC_OF_SEC * MDS_TIME_SEC_OF_HOUR;

    tm->msec = tmp % MDS_TIME_MSEC_OF_SEC;
    tmp /= MDS_TIME_MSEC_OF_SEC;
    tm->second = tmp % MDS_TIME_SEC_OF_MIN;
    tmp /= MDS_TIME_SEC_OF_MIN;
    tm->minute = tmp % MDS_TIME_MIN_OF_HOUR;
    tmp /= MDS_TIME_MIN_OF_HOUR;
    tm->hour = tmp % MDS_TIME_HOUR_OF_DAY;
    tmp /= MDS_TIME_HOUR_OF_DAY;
    tm->wday = (TIME_UNIX_WEEKDAY_BEGIN + tmp) % MDS_TIME_DAY_OF_WEEK;
    for (tm->year = TIME_UNIX_YEAR_BEGIN; tmp > 0; (tm->year)++) {
        int16_t yday = (TIME_IS_LEAPYEAR(tm->year)) ? (MDS_TIME_DAY_OF_YEAR + 1) : (MDS_TIME_DAY_OF_YEAR);
        if (tmp >= yday) {
            tmp -= yday;
        } else {
            break;
        }
    }
    tm->yday = tmp + 1;
    for (tm->month = MDS_TIME_MONTH_DEC; tm->month >= MDS_TIME_MONTH_JAN; tm->month -= 1) {
        int16_t yday = G_YDAYS_OF_MONTH[tm->month - 1];
        if ((tm->month > MDS_TIME_MONTH_FEB) && TIME_IS_LEAPYEAR(tm->year)) {
            yday += 1;
        }
        if (yday < tmp) {
            tmp -= yday;
            break;
        }
    }
    tm->mday = tmp;
}

MDS_Time_t MDS_TIME_DiffTimeMs(MDS_TimeDate_t *tm1, MDS_TimeDate_t *tm2)
{
    MDS_Time_t ts1 = MDS_TIME_ChangeTimeStamp(tm1, 0);
    MDS_Time_t ts2 = MDS_TIME_ChangeTimeStamp(tm2, 0);

    return (ts1 - ts2);
}

__attribute__((weak)) MDS_Time_t MDS_TIME_GetTimeStamp(int8_t *tz)
{
    MDS_Tick_t ticks = MDS_SysTickGetCount();
    MDS_Time_t ts = g_unixTimeBase + MDS_SysTickToMs(ticks);

    if (tz != NULL) {
        *tz = g_localTimeZone;
    }

    return (ts);
}

__attribute__((weak)) void MDS_TIME_SetTimeStamp(MDS_Time_t ts, int8_t tz)
{
    MDS_Tick_t ticks = MDS_SysTickGetCount();

    g_unixTimeBase = ts - MDS_SysTickToMs(ticks);
    g_localTimeZone = tz;
}
