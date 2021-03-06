/*
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
 * @file    STM32/RTCv2/qrtc_lld.h
 * @brief   STM32L1xx/STM32F2xx/STM32F4xx RTC low level driver header.
 *
 * @addtogroup RTC
 * @{
 */

#ifndef _QRTC_LLD_H_
#define _QRTC_LLD_H_

#if HAL_USE_RTC || defined(__DOXYGEN__)

#include <hal_rtc_lld.h>

#include <time.h>

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
#if STM32_RTC_HAS_PERIODIC_WAKEUPS
  bool rtcGetPeriodicWakeupFlag_v2(RTCDriver *rtcp);
  void rtcClearPeriodicWakeupFlag_v2(RTCDriver *rtcp);
#endif /* STM32_RTC_HAS_PERIODIC_WAKEUPS */
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_RTC */

#endif /* _QRTC_LLD_H_ */

/** @} */
