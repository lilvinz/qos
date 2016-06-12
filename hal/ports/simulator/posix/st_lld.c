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
 * @file    posix/st_lld.c
 * @brief   ST Driver subsystem low level driver code.
 *
 * @addtogroup ST
 * @{
 */

#include "hal.h"

#include <sys/time.h>

#if (OSAL_ST_MODE != OSAL_ST_MODE_NONE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING

#error "Posix port doesn't support free-running timer yet."

#endif /* OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING */

#if OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC

#endif /* OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local types.                                                       */
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

#if (OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING) || defined(__DOXYGEN__)

#endif /* OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING */

#if (OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC) || defined(__DOXYGEN__)

/**
 * @brief   System Timer vector.
 * @details This interrupt is used for system tick in periodic mode.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(port_tick_signal_handler)
{
    OSAL_IRQ_PROLOGUE();

    osalSysLockFromISR();
    osalOsTimerHandlerI();
    osalSysUnlockFromISR();

    OSAL_IRQ_EPILOGUE();
}

static void port_tick_signal_handler_stub(int arg)
{
    if (port_tick_signal_handler() == true)
    {
#if CH_DBG_SYSTEM_STATE_CHECK
        _dbg_check_lock();
#endif
        chSchDoReschedule();
#if CH_DBG_SYSTEM_STATE_CHECK
        _dbg_check_unlock();
#endif
    }
}

#endif /* OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ST driver initialization.
 *
 * @notapi
 */
void st_lld_init(void)
{
#if OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING
    /* Free running counter mode. */

#endif /* OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING */

#if OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC
    /* Periodic systick mode. */
    struct sigaction sigtick = {
        .sa_flags = 0,
        .sa_handler = port_tick_signal_handler_stub,
    };

    /* Set timer signal to be auto masked when entering handler.
     * On exit it will be automatically unmasked as well. */
    if (sigemptyset(&sigtick.sa_mask) < 0)
        chSysHalt("sigemptyset() failed");

    if (sigaddset(&sigtick.sa_mask, PORT_TIMER_SIGNAL) < 0)
        chSysHalt("sigaddset() failed");

    if (sigaction(PORT_TIMER_SIGNAL, &sigtick, NULL) < 0)
        chSysHalt("sigaction() failed");

    const suseconds_t usecs = 1000000 / CH_CFG_ST_FREQUENCY;
    struct itimerval itimer, oitimer;

    /* Initialize the structure with the current timer information. */
    if (getitimer(PORT_TIMER_TYPE, &itimer) < 0)
        chSysHalt("getitimer() failed");

    /* Set the interval between timer events. */
    itimer.it_interval.tv_sec = usecs / 1000000;
    itimer.it_interval.tv_usec = usecs % 1000000;

    /* Set the current count-down. */
    itimer.it_value.tv_sec = usecs / 1000000;
    itimer.it_value.tv_usec = usecs % 1000000;

    /* Set-up the timer interrupt. */
    if (setitimer(PORT_TIMER_TYPE, &itimer, &oitimer) < 0)
        chSysHalt("setitimer() failed");
#endif /* OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC */
}

#endif /* OSAL_ST_MODE != OSAL_ST_MODE_NONE */

/** @} */
