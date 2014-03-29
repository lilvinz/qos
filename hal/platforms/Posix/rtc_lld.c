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
/*
   Concepts and parts of this file have been contributed by Uladzimir Pylinsky
   aka barthess.
 */

/**
 * @file    STM32/RTCv2/rtc_lld.c
 * @brief   STM32L1xx/STM32F2xx/STM32F4xx RTC low level driver.
 *
 * @addtogroup RTC
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_RTC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief RTC driver identifier.
 */
RTCDriver RTCD1;

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
 * @brief   Lowlevel init of RTC
 *
 * @api
 */
void rtc_lld_init(void)
{
}

/**
 * @brief   Set current time.
 * @note    Not implemented on Posix.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] timespec  pointer to a @p RTCTime structure
 *
 * @api
 */
void rtc_lld_set_time(RTCDriver *rtcp, const RTCTime *timespec)
{
    (void)rtcp;
    (void)timespec;
}

/**
 * @brief   Get current time.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[out] timespec pointer to a @p RTCTime structure
 *
 * @api
 */
void rtc_lld_get_time(RTCDriver *rtcp, RTCTime *timespec)
{
    (void)rtcp;
    time_t timestamp;
    time(&timestamp);
    localtime_r(&timestamp, &timespec->tm);
}

/**
 * @brief     Set alarm time.
 *
 * @note      Not implemented on Posix.
 *
 * @param[in] rtcp      Pointer to RTC driver structure.
 * @param[in] alarm     Alarm identifier. Can be 1 or 2.
 * @param[in] alarmspec Pointer to a @p RTCAlarm structure.
 *
 * @api
 */
void rtc_lld_set_alarm(RTCDriver *rtcp,
                       rtcalarm_t alarm,
                       const RTCAlarm *alarmspec)
{
}

/**
 * @brief   Get alarm time.
 *
 * @note    Not implemented on Posix.
 *
 * @param[in] rtcp       pointer to RTC driver structure
 * @param[in] alarm      alarm identifier
 * @param[out] alarmspec pointer to a @p RTCAlarm structure
 *
 * @api
 */
void rtc_lld_get_alarm(RTCDriver *rtcp, rtcalarm_t alarm,
        RTCAlarm *alarmspec)
{
}

/**
 * @brief   Get current time in format suitable for usage in FatFS.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @return              FAT time value.
 *
 * @api
 */
uint32_t rtc_lld_get_time_fat(RTCDriver *rtcp)
{
  uint32_t fattime;
  RTCTime timespec;

  chSysLock();
  rtcGetTimeI(rtcp, &timespec);
  chSysUnlock();

  fattime  = timespec.tm.tm_sec >> 1;
  fattime |= timespec.tm.tm_min << 5;
  fattime |= timespec.tm.tm_hour << 11;
  fattime |= timespec.tm.tm_mday << 16;
  fattime |= (timespec.tm.tm_mon + 1) << 21;
  fattime |= (timespec.tm.tm_year % 100) << 25;

  return fattime;
}

#endif /* HAL_USE_RTC */

/** @} */
