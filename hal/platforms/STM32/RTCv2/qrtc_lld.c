/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    STM32/RTCv2/qrtc_lld.c
 * @brief   STM32L1xx/STM32F2xx/STM32F4xx RTC low level driver.
 *
 * @addtogroup RTC
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_RTC || defined(__DOXYGEN__)

#include <string.h>

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Convert from RTCTime to struct tm.
 *
 * @param[in] timespec      pointer to RTCTime structure
 * @param[out] result       pointer to tm structure
 *
 * @api
 */
void rtcRTCTime2TM(const RTCTime *timespec, struct tm *result)
{
    memset(result, 0, sizeof(*result));

    uint32_t v;

    v = (timespec->tv_time & RTC_TR_SU) >> RTC_TR_SU_OFFSET;
    v += ((timespec->tv_time & RTC_TR_ST) >> RTC_TR_ST_OFFSET) * 10;
    result->tm_sec = v;

    v = (timespec->tv_time & RTC_TR_MNU) >> RTC_TR_MNU_OFFSET;
    v += ((timespec->tv_time & RTC_TR_MNT) >> RTC_TR_MNT_OFFSET) * 10;
    result->tm_min = v;

    v = (timespec->tv_time & RTC_TR_HU) >> RTC_TR_HU_OFFSET;
    v += ((timespec->tv_time & RTC_TR_HT) >> RTC_TR_HT_OFFSET) * 10;
    v += 12 * ((timespec->tv_time & RTC_TR_PM) >> RTC_TR_PM_OFFSET);
    result->tm_hour = v;

    v =  (timespec->tv_date & RTC_DR_DU) >> RTC_DR_DU_OFFSET;
    v += ((timespec->tv_date & RTC_DR_DT) >> RTC_DR_DT_OFFSET) * 10;
    result->tm_mday = v;

    v = (timespec->tv_date & RTC_DR_MU) >> RTC_DR_MU_OFFSET;
    v += ((timespec->tv_date & RTC_DR_MT) >> RTC_DR_MT_OFFSET) * 10;
    result->tm_mon = v - 1;

    v = (timespec->tv_date & RTC_DR_YU) >> RTC_DR_YU_OFFSET;
    v += ((timespec->tv_date & RTC_DR_YT) >> RTC_DR_YT_OFFSET) * 10;
    result->tm_year = v + 2000 - 1900;

    v = (timespec->tv_date & RTC_DR_WDU) >> RTC_DR_WDU_OFFSET;
    if (v == 7)
        result->tm_wday = 0;
    else
        result->tm_wday = v;

    result->tm_yday = 0;
    result->tm_isdst = 0;
}

/**
 * @brief   Convert from struct tm to RTCTime.
 *
 * @param[in] result        pointer to tm structure
 * @param[out] timespec     pointer to RTCTime structure
 *
 * @api
 */
void rtcTM2RTCTime(const struct tm *timespec, RTCTime *result)
{
    memset(result, 0, sizeof(*result));

    result->tv_time |=
            ((timespec->tm_sec / 10) << RTC_TR_ST_OFFSET) & RTC_TR_ST;
    result->tv_time |=
            ((timespec->tm_sec % 10) << RTC_TR_SU_OFFSET) & RTC_TR_SU;

    result->tv_time |=
            ((timespec->tm_min / 10) << RTC_TR_MNT_OFFSET) & RTC_TR_MNT;
    result->tv_time |=
            ((timespec->tm_min % 10) << RTC_TR_MNU_OFFSET) & RTC_TR_MNU;

    result->tv_time |=
            ((timespec->tm_hour / 10) << RTC_TR_HT_OFFSET) & RTC_TR_HT;
    result->tv_time |=
            ((timespec->tm_hour % 10) << RTC_TR_HU_OFFSET) & RTC_TR_HU;

    result->tv_date |=
            ((timespec->tm_mday / 10) << RTC_DR_DT_OFFSET) & RTC_DR_DT;
    result->tv_date |=
            ((timespec->tm_mday % 10) << RTC_DR_DU_OFFSET) & RTC_DR_DU;

    result->tv_date |=
            (((timespec->tm_mon + 1) / 10) << RTC_DR_MT_OFFSET) & RTC_DR_MT;
    result->tv_date |=
            (((timespec->tm_mon + 1) % 10) << RTC_DR_MU_OFFSET) & RTC_DR_MU;

    result->tv_date |= (((timespec->tm_year + 1900 - 2000) / 10)
            << RTC_DR_YT_OFFSET) & RTC_DR_YT;
    result->tv_date |= (((timespec->tm_year + 1900 - 2000) % 10)
            << RTC_DR_YU_OFFSET) & RTC_DR_YU;

    result->tv_date |=
            ((timespec->tm_wday + 1) << RTC_DR_WDU_OFFSET) & RTC_DR_WDU;
}

/**
 * @brief     Gets status of the periodic wake-up flag.
 *
 * @param[in] rtcp          pointer to RTC driver structure
 * @return                  TRUE if flag is set
 *
 * @api
 */
#if RTC_HAS_PERIODIC_WAKEUPS
bool_t rtcGetPeriodicWakeupFlag_v2(RTCDriver *rtcp)
{
    if ((rtcp->id_rtc->ISR & RTC_ISR_WUTF) != 0)
        return TRUE;
    else
        return FALSE;
}
#endif /* RTC_HAS_PERIODIC_WAKEUPS */

/**
 * @brief     Clears periodic wake-up flag.
 *
 * @param[in] rtcp          pointer to RTC driver structure
 *
 * @api
 */
#if RTC_HAS_PERIODIC_WAKEUPS
void rtcClearPeriodicWakeupFlag_v2(RTCDriver *rtcp)
{
    /* Clear RTC wake-up flag. */
    rtcp->id_rtc->ISR &= ~RTC_ISR_WUTF;
}
#endif /* RTC_HAS_PERIODIC_WAKEUPS */

#endif /* HAL_USE_RTC */

/** @} */
