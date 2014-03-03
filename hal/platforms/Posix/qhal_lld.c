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
 * @file    Posix/qhal_lld.c
 * @brief   Posix HAL subsystem low level driver code.
 *
 * @addtogroup POSIX_HAL
 * @{
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>

#include "ch.h"
#include "qhal.h"

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

CH_IRQ_HANDLER(port_tick_signal_handler);

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief Low level HAL driver initialization.
 */
void qhal_lld_init(void) {

  struct sigaction sigtick = {
    .sa_flags = 0,
    .sa_handler = port_tick_signal_handler,
  };

  if (sigfillset(&sigtick.sa_mask) < 0)
    port_halt();

  if (sigaction(PORT_TIMER_SIGNAL, &sigtick, NULL) < 0)
    port_halt();

  const suseconds_t usecs = 1000000 / CH_FREQUENCY;
  struct itimerval itimer, oitimer;

  /* Initialize the structure with the current timer information. */
  if (getitimer(PORT_TIMER_TYPE, &itimer) < 0)
    port_halt();

  /* Set the interval between timer events. */
  itimer.it_interval.tv_sec = usecs / 1000000;
  itimer.it_interval.tv_usec = usecs % 1000000;

  /* Set the current count-down. */
  itimer.it_value.tv_sec = usecs / 1000000;
  itimer.it_value.tv_usec = usecs % 1000000;

  /* Set-up the timer interrupt. */
  if (setitimer(PORT_TIMER_TYPE, &itimer, &oitimer) < 0)
    port_halt();
}

/** @} */
