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
 * @file    Posix/rtc_lld.h
 * @brief   Posix RTC low level driver header.
 *
 * @addtogroup RTC
 * @{
 */

#ifndef _RTC_LLD_H_
#define _RTC_LLD_H_

#if HAL_USE_RTC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Posix has no periodic wakeups.
 */
#define RTC_HAS_PERIODIC_WAKEUPS  FALSE

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a structure representing an RTC alarm time stamp.
 */
typedef struct RTCAlarm RTCAlarm;

/**
 * @brief   Type of a structure representing an RTC wakeup period.
 */
typedef struct RTCWakeup RTCWakeup;

/**
 * @brief   Type of a structure representing an RTC callbacks config.
 */
typedef struct RTCCallbackConfig RTCCallbackConfig;

/**
 * @brief   Type of an RTC alarm.
 * @details Meaningful on platforms with more than 1 alarm comparator.
 */
typedef uint32_t rtcalarm_t;

/**
 * @brief   Structure representing an RTC time stamp.
 */
struct RTCTime
{
    struct tm tm;
};

/**
 * @brief   Structure representing an RTC alarm time stamp.
 */
struct RTCAlarm {
  /**
   * @brief Date and time of alarm in STM32 BCD.
   */
  uint32_t tv_datetime;
};

#if RTC_HAS_PERIODIC_WAKEUPS
/**
 * @brief   Structure representing an RTC periodic wakeup period.
 */
struct RTCWakeup {
  /**
   * @brief   RTC WUTR register.
   * @details Bits [15:0] contain value of WUTR register
   *          Bits [18:16] contain value of WUCKSEL bits in CR register
   *
   * @note    ((WUTR == 0) || (WUCKSEL == 3)) is forbidden combination.
   */
  uint32_t wakeup;
};
#endif /* RTC_HAS_PERIODIC_WAKEUPS */

/**
 * @brief   Structure representing an RTC driver.
 */
struct RTCDriver
{
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(__DOXYGEN__)
extern RTCDriver RTCD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void rtc_lld_init(void);
  void rtc_lld_set_time(RTCDriver *rtcp, const RTCTime *timespec);
  void rtc_lld_get_time(RTCDriver *rtcp, RTCTime *timespec);
  void rtc_lld_set_alarm(RTCDriver *rtcp,
                         rtcalarm_t alarm,
                         const RTCAlarm *alarmspec);
  void rtc_lld_get_alarm(RTCDriver *rtcp,
                         rtcalarm_t alarm,
                         RTCAlarm *alarmspec);
  uint32_t rtc_lld_get_time_fat(RTCDriver *rtcp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_RTC */

#endif /* _RTC_LLD_H_ */

/** @} */
