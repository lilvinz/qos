/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio.

    This file is part of ChibiOS.

    ChibiOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    SIMIA32/chcore.c
 * @brief   Simulator on IA32 port code.
 *
 * @addtogroup SIMIA32_GCC_CORE
 * @{
 */

#include "ch.h"
#include "osal.h"

#include <time.h>

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

bool port_isr_context_flag;
syssts_t port_irq_sts;

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static void _setcontext(void *arg) {
  thread_t *ntp = (thread_t*)arg;
  if (setcontext(&ntp->ctx.uc) < 0)
    chSysHalt("setcontext() failed");
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Performs a context switch between two threads.
 * @details This is the most critical code in any port, this function
 *          is responsible for the context switch between 2 threads.
 * @note    The implementation of this code affects <b>directly</b> the context
 *          switch performance so optimize here as much as you can.
 *
 * @param[in] ntp       the thread to be switched in
 * @param[in] otp       the thread to be switched out
 */
void _port_switch(thread_t *ntp, thread_t *otp) {

  /* Create temporary context to perform swap. */
  static uint8_t tempstack[PORT_INT_REQUIRED_STACK];
  ucontext_t tempctx;
  if (getcontext(&tempctx) < 0)
    chSysHalt("getcontext() failed");
  tempctx.uc_stack.ss_sp = tempstack;
  tempctx.uc_stack.ss_size = sizeof(tempstack);
  tempctx.uc_stack.ss_flags = 0;
  makecontext(&tempctx, (void(*)(void))_setcontext, 1, ntp);

  /* Save running thread, jump to temporary context. */
  if (swapcontext(&otp->ctx.uc, &tempctx) < 0)
    chSysHalt("swapcontext() failed");
}

/**
 * @brief   Start a thread by invoking its work function.
 * @details If the work function returns @p chThdExit() is automatically
 *          invoked.
 */
void _port_thread_start(void (*pf)(void*), void* arg) {
  osalSysUnlock();
  pf(arg);
  chThdExit(0);
  while (1);
}

/**
 * @brief   Returns the current value of the realtime counter.
 *
 * @return              The realtime counter value.
 */
rtcnt_t port_rt_get_counter_value(void) {

  struct timespec ts;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);

  return (rtcnt_t)ts.tv_nsec;
}

/** @} */
