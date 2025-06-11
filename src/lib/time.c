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
#include "mds_def.h"
#include "mds_log.h"

/* Time -------------------------------------------------------------------- */
#define TIME_UNIX_YEAR_BEGIN    1970
#define TIME_UNIX_WEEKDAY_BEGIN 4

#define TIME_IS_LEAPYEAR(year)                                                                    \
    (((((year) % 400) == 0) || ((((year) % 4) == 0) && (((year) % 100) != 0))) ? (true) : (false))

#define TIME_GET_YEARDAYS(year)                                                                   \
    ((((year) - (TIME_UNIX_YEAR_BEGIN) + (1)) / 4) +                                              \
     (((year) - (TIME_UNIX_YEAR_BEGIN) - (1)) / 100) +                                            \
     (((year) - (TIME_UNIX_YEAR_BEGIN) + (299)) / 400))

static const int16_t G_YDAYS_OF_MONTH[] = {
    -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364};

MDS_TimeStamp_t MDS_TIME_ChangeTimeStamp(MDS_TimeDate_t *tm, int8_t tz)
{
    MDS_TimeStamp_t tmp = {.ts = ((tm->year - TIME_UNIX_YEAR_BEGIN) * MDS_TIME_DAY_OF_YEAR) +
                                 TIME_GET_YEARDAYS(tm->year)};

    tm->yday = tm->mday + G_YDAYS_OF_MONTH[tm->month - 1];
    if ((tm->month > MDS_TIME_MONTH_FEB) && (TIME_IS_LEAPYEAR(tm->year))) {
        tm->yday += 1;
    }

    tmp.ts = (tmp.ts + tm->yday) * MDS_TIME_SEC_OF_DAY + MDS_TIME_SEC_OF_HOUR * (tz + tm->hour) +
             MDS_TIME_SEC_OF_MIN * tm->minute + tm->second;

    tmp.ts = (tmp.ts * MDS_TIME_MSEC_OF_SEC) + tm->msec;

    return (tmp);
}

void MDS_TIME_ChangeTimeDate(MDS_TimeDate_t *tm, MDS_TimeStamp_t ts, int8_t tz)
{
    MDS_TimeStamp_t tmp = {.ts = ts.ts + tz * MDS_TIME_MSEC_OF_SEC * MDS_TIME_SEC_OF_HOUR * tz};

    tm->msec = tmp.ts % MDS_TIME_MSEC_OF_SEC;
    tmp.ts /= MDS_TIME_MSEC_OF_SEC;
    tm->second = tmp.ts % MDS_TIME_SEC_OF_MIN;
    tmp.ts /= MDS_TIME_SEC_OF_MIN;
    tm->minute = tmp.ts % MDS_TIME_MIN_OF_HOUR;
    tmp.ts /= MDS_TIME_MIN_OF_HOUR;
    tm->hour = tmp.ts % MDS_TIME_HOUR_OF_DAY;
    tmp.ts /= MDS_TIME_HOUR_OF_DAY;
    tm->wday = (TIME_UNIX_WEEKDAY_BEGIN + tmp.ts) % MDS_TIME_DAY_OF_WEEK;
    for (tm->year = TIME_UNIX_YEAR_BEGIN; tmp.ts > 0; (tm->year)++) {
        int16_t yday = (TIME_IS_LEAPYEAR(tm->year)) ? (MDS_TIME_DAY_OF_YEAR + 1)
                                                    : (MDS_TIME_DAY_OF_YEAR);
        if (tmp.ts >= yday) {
            tmp.ts -= yday;
        } else {
            break;
        }
    }
    tm->yday = tmp.ts + 1;
    for (tm->month = MDS_TIME_MONTH_DEC; tm->month >= MDS_TIME_MONTH_JAN; tm->month -= 1) {
        int16_t yday = G_YDAYS_OF_MONTH[tm->month - 1];
        if ((tm->month > MDS_TIME_MONTH_FEB) && TIME_IS_LEAPYEAR(tm->year)) {
            yday += 1;
        }
        if (yday < tmp.ts) {
            tmp.ts -= yday;
            break;
        }
    }
    tm->mday = tmp.ts;
}

MDS_TimeStamp_t MDS_TIME_DiffTimeMs(MDS_TimeDate_t *tm1, MDS_TimeDate_t *tm2)
{
    MDS_TimeStamp_t ts1 = MDS_TIME_ChangeTimeStamp(tm1, 0);
    MDS_TimeStamp_t ts2 = MDS_TIME_ChangeTimeStamp(tm2, 0);

    return ((MDS_TimeStamp_t) {.ts = ts1.ts - ts2.ts});
}
