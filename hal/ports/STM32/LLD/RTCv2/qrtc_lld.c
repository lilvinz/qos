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
 * @brief     Gets status of the periodic wake-up flag.
 *
 * @param[in] rtcp          pointer to RTC driver structure
 * @return                  TRUE if flag is set
 *
 * @api
 */
#if STM32_RTC_HAS_PERIODIC_WAKEUPS
bool rtcGetPeriodicWakeupFlag_v2(RTCDriver *rtcp)
{
    if ((rtcp->rtc->ISR & RTC_ISR_WUTF) != 0)
        return TRUE;
    else
        return FALSE;
}
#endif /* STM32_RTC_HAS_PERIODIC_WAKEUPS */

/**
 * @brief     Clears periodic wake-up flag.
 *
 * @param[in] rtcp          pointer to RTC driver structure
 *
 * @api
 */
#if STM32_RTC_HAS_PERIODIC_WAKEUPS
void rtcClearPeriodicWakeupFlag_v2(RTCDriver *rtcp)
{
    /* Clear RTC wake-up flag. */
    rtcp->rtc->ISR &= ~RTC_ISR_WUTF;
}
#endif /* STM32_RTC_HAS_PERIODIC_WAKEUPS */

#endif /* HAL_USE_RTC */

/** @} */
