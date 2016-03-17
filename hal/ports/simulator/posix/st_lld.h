/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

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
 * @file    posix/st_lld.h
 * @brief   ST Driver subsystem low level driver header.
 * @details This header is designed to be include-able without having to
 *          include other files from the HAL.
 *
 * @addtogroup ST
 * @{
 */

#ifndef _ST_LLD_H_
#define _ST_LLD_H_

#include <time.h>

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */

/**
 * @brief   Typer time to be used to generate systick interrupt.
 * @details ITIMER_REAL
 *            Decrements in real time, and delivers SIGALRM upon expiration.
 *            This timer will fail when using debug breakpoints.
 *          ITIMER_VIRTUAL
 *            Decrements only when the process is executing, and delivers
 *            SIGVTALRM upon expiration.
 *          ITIMER_PROF
 *            Decrements both when the process executes and when the system
 *            is executing on behalf of the process. Delivers SIGPROF
 *            upon expiration.
 */
#if !defined(PORT_TIMER_TYPE) || defined(__DOXYGEN__)
#define PORT_TIMER_TYPE                 ITIMER_REAL
#endif

/**
 * @brief   Typer signal to be used to generate systick interrupt.
 * @details SIGALRM
 *            Will be delivered on expiration of ITIMER_REAL
 *          SIGVTALRM
 *            Will be delivered on expiration of ITIMER_VIRTUAL
 *          SIGPROF
 *            Will be delivered on expiration of ITIMER_PROF
 */
#if !defined(PORT_TIMER_SIGNAL) || defined(__DOXYGEN__)
#define PORT_TIMER_SIGNAL               SIGALRM
#endif

/** @} */

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
  void st_lld_init(void);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Driver inline functions.                                                  */
/*===========================================================================*/

/**
 * @brief   Returns the time counter value.
 *
 * @return              The counter value.
 *
 * @notapi
 */
static inline systime_t st_lld_get_counter(void) {

  struct timespec ts;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);

  return (systime_t)ts.tv_nsec;
}

/**
 * @brief   Starts the alarm.
 * @note    Makes sure that no spurious alarms are triggered after
 *          this call.
 *
 * @param[in] time      the time to be set for the first alarm
 *
 * @notapi
 */
static inline void st_lld_start_alarm(systime_t time) {

  (void)time;
}

/**
 * @brief   Stops the alarm interrupt.
 *
 * @notapi
 */
static inline void st_lld_stop_alarm(void) {

}

/**
 * @brief   Sets the alarm time.
 *
 * @param[in] time      the time to be set for the next alarm
 *
 * @notapi
 */
static inline void st_lld_set_alarm(systime_t time) {

  (void)time;
}

/**
 * @brief   Returns the current alarm time.
 *
 * @return              The currently set alarm time.
 *
 * @notapi
 */
static inline systime_t st_lld_get_alarm(void) {

  return (systime_t)0;
}

/**
 * @brief   Determines if the alarm is active.
 *
 * @return              The alarm status.
 * @retval false        if the alarm is not active.
 * @retval true         is the alarm is active
 *
 * @notapi
 */
static inline bool st_lld_is_alarm_active(void) {

  return false;
}

#endif /* _ST_LLD_H_ */

/** @} */
