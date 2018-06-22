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
 * @file    STM32/USARTv2/serial_485_lld.c
 * @brief   STM32 low level serial driver code.
 *
 * @addtogroup SERIAL_485
 * @{
 */

#include "qhal.h"

#if HAL_USE_SERIAL_485 || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* For compatibility for those devices without LIN support in the USARTs.*/
#if !defined(USART_ISR_LBDF)
#define USART_ISR_LBDF                      0
#endif

#if !defined(USART_CR2_LBDIE)
#define USART_CR2_LBDIE                     0
#endif

/* STM32L0xx/STM32F7xx ST headers difference.*/
#if !defined(USART_ISR_LBDF)
#define USART_ISR_LBDF                      USART_ISR_LBD
#endif

/* Handling the case where UART4 and UART5 are actually USARTs, this happens
   in the STM32F0xx.*/
#if defined(USART4)
#define UART4                               USART4
#endif

#if defined(USART5)
#define UART5                               USART5
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief USART1 serial driver identifier.*/
#if STM32_SERIAL_485_USE_USART1 || defined(__DOXYGEN__)
Serial485Driver S485D1;
#endif

/** @brief USART2 serial driver identifier.*/
#if STM32_SERIAL_485_USE_USART2 || defined(__DOXYGEN__)
Serial485Driver S485D2;
#endif

/** @brief USART3 serial driver identifier.*/
#if STM32_SERIAL_485_USE_USART3 || defined(__DOXYGEN__)
Serial485Driver S485D3;
#endif

/** @brief UART4 serial driver identifier.*/
#if STM32_SERIAL_485_USE_UART4 || defined(__DOXYGEN__)
Serial485Driver S485D4;
#endif

/** @brief UART5 serial driver identifier.*/
#if STM32_SERIAL_485_USE_UART5 || defined(__DOXYGEN__)
Serial485Driver S485D5;
#endif

/** @brief USART6 serial driver identifier.*/
#if STM32_SERIAL_485_USE_USART6 || defined(__DOXYGEN__)
Serial485Driver S485D6;
#endif

/** @brief UART7 serial driver identifier.*/
#if STM32_SERIAL_485_USE_UART7 || defined(__DOXYGEN__)
Serial485Driver S485D7;
#endif

/** @brief UART8 serial driver identifier.*/
#if STM32_SERIAL_485_USE_UART8 || defined(__DOXYGEN__)
Serial485Driver S485D8;
#endif

/** @brief LPUART1 serial driver identifier.*/
#if STM32_SERIAL_485_USE_LPUART1 || defined(__DOXYGEN__)
Serial485Driver LPS485D1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/** @brief Driver default configuration.*/
static const Serial485Config default_config =
{
  .speed = SERIAL_485_DEFAULT_BITRATE,
  .cr1 = 0,
  .cr2 = USART_CR2_STOP1_BITS | USART_CR2_LINEN,
  .cr3 = 0,
  .ssport = 0,
  .sspad = 0,
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   USART initialization.
 * @details This function must be invoked with interrupts disabled.
 *
 * @param[in] s485dp    pointer to a @p Serial485Driver object
 * @param[in] config    the architecture-dependent serial driver configuration
 */
static void usart_init(Serial485Driver *s485dp, const Serial485Config *config) {
  USART_TypeDef *u = s485dp->usart;

  /* Baud rate setting.*/
#if STM32_SERIAL_485_USE_LPUART1
  if ( s485dp == &LPS485D1 )
  {
      u->BRR = (uint32_t)( ( (uint64_t)s485dp->clock * 256 ) / config->speed);
  }
  else
#endif
  u->BRR = (uint32_t)(s485dp->clock / config->speed);

  /* Note that some bits are enforced.*/
  u->CR2 = config->cr2 | USART_CR2_LBDIE;
  u->CR3 = config->cr3 | USART_CR3_EIE;
  u->CR1 = config->cr1 | USART_CR1_UE | USART_CR1_PEIE |
                         USART_CR1_RXNEIE | USART_CR1_TE |
                         USART_CR1_RE;
  u->ICR = 0xFFFFFFFFU;
}

/**
 * @brief   USART de-initialization.
 * @details This function must be invoked with interrupts disabled.
 *
 * @param[in] u         pointer to an USART I/O block
 */
static void usart_deinit(USART_TypeDef *u) {

  u->CR1 = 0;
  u->CR2 = 0;
  u->CR3 = 0;
}

/**
 * @brief   Error handling routine.
 *
 * @param[in] s485dp    pointer to a @p Serial485Driver object
 * @param[in] isr       USART ISR register value
 */
static void set_error(Serial485Driver *s485dp, uint32_t isr) {
  eventflags_t sts = 0;

  if (isr & USART_ISR_ORE)
    sts |= S485D_OVERRUN_ERROR;
  if (isr & USART_ISR_PE)
    sts |= S485D_PARITY_ERROR;
  if (isr & USART_ISR_FE)
    sts |= S485D_FRAMING_ERROR;
  if (isr & USART_ISR_NE)
    sts |= S485D_NOISE_ERROR;
  osalSysLockFromISR();
  chnAddFlagsI(s485dp, sts);
  osalSysUnlockFromISR();
}

/**
 * @brief   Common IRQ handler.
 *
 * @param[in] s485dp    communication channel associated to the USART
 */
static void serve_interrupt(Serial485Driver *s485dp) {
  USART_TypeDef *u = s485dp->usart;
  uint32_t cr1 = u->CR1;
  uint32_t isr;

  /* Reading and clearing status.*/
  isr = u->ISR;
  u->ICR = isr;

  /* Error condition detection.*/
  if (isr & (USART_ISR_ORE | USART_ISR_NE | USART_ISR_FE  | USART_ISR_PE))
    set_error(s485dp, isr);

  /* Special case, LIN break detection.*/
  if (isr & USART_ISR_LBDF) {
    osalSysLockFromISR();
    chnAddFlagsI(s485dp, S485D_BREAK_DETECTED);
    osalSysUnlockFromISR();
  }

  /* Data available.*/
  if (isr & USART_ISR_RXNE) {
    osalSysLockFromISR();
    /* Mask parity bit according to configuration. */
    uint16_t b = u->RDR;
    switch (u->CR1 & (USART_CR1_PCE | USART_CR1_M1 | USART_CR1_M0))
    {
    case USART_CR1_PCE:
        b &= 0x007f;
        break;
    case USART_CR1_PCE | USART_CR1_M1:
        b &= 0x00ff;
        break;
    case USART_CR1_PCE | USART_CR1_M0:
        b &= 0x01ff;
        break;
    default:
        break;
    }
    sdIncomingDataI(s485dp, b);
    osalSysUnlockFromISR();
  }

  /* Physical transmission end.*/
  if (isr & USART_ISR_TC) {
    /* Clear driver enable pad. */
    if (s485dp->config->ssport != NULL)
      palClearPad(s485dp->config->ssport, s485dp->config->sspad);
    osalSysLockFromISR();
    if (oqIsEmptyI(&s485dp->oqueue))
      chnAddFlagsI(s485dp, CHN_TRANSMISSION_END);
    u->CR1 = cr1 & ~USART_CR1_TCIE;
    osalSysUnlockFromISR();
  }

  /* Transmission buffer empty.*/
  if ((cr1 & USART_CR1_TXEIE) && (isr & USART_ISR_TXE)) {
    msg_t b;
    osalSysLockFromISR();
    b = oqGetI(&s485dp->oqueue);
    if (b < Q_OK) {
      chnAddFlagsI(s485dp, CHN_OUTPUT_EMPTY);
      u->CR1 = (cr1 & ~USART_CR1_TXEIE) | USART_CR1_TCIE;
    }
    else
    {
      /* Disable RX if enabled. */
      if (cr1 & USART_CR1_RE)
        u->CR1 = cr1 & ~USART_CR1_RE;
      /* Set driver enable pad. */
      if (s485dp->config->ssport != NULL)
        palSetPad(s485dp->config->ssport, s485dp->config->sspad);
      u->TDR = b;
    }
    osalSysUnlockFromISR();
  }
}

#if STM32_SERIAL_485_USE_USART1 || defined(__DOXYGEN__)
static void notify1(io_queue_t *qp) {

  (void)qp;
  USART1->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_485_USE_USART2 || defined(__DOXYGEN__)
static void notify2(io_queue_t *qp) {

  (void)qp;
  USART2->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_485_USE_USART3 || defined(__DOXYGEN__)
static void notify3(io_queue_t *qp) {

  (void)qp;
  USART3->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_485_USE_UART4 || defined(__DOXYGEN__)
static void notify4(io_queue_t *qp) {

  (void)qp;
  UART4->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_485_USE_UART5 || defined(__DOXYGEN__)
static void notify5(io_queue_t *qp) {

  (void)qp;
  UART5->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_485_USE_USART6 || defined(__DOXYGEN__)
static void notify6(io_queue_t *qp) {

  (void)qp;
  USART6->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_485_USE_UART7 || defined(__DOXYGEN__)
static void notify7(io_queue_t *qp) {

  (void)qp;
  UART7->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_485_USE_UART8 || defined(__DOXYGEN__)
static void notify8(io_queue_t *qp) {

  (void)qp;
  UART8->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_485_USE_LPUART1 || defined(__DOXYGEN__)
static void notifylp1(io_queue_t *qp) {

  (void)qp;
  LPUART1->CR1 |= USART_CR1_TXEIE;
}
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_SERIAL_485_USE_USART1 || defined(__DOXYGEN__)
#if !defined(STM32_USART1_HANDLER)
#error "STM32_USART1_HANDLER not defined"
#endif
/**
 * @brief   USART1 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_USART1_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  serve_interrupt(&S485D1);

  OSAL_IRQ_EPILOGUE();
}
#endif

#if STM32_SERIAL_485_USE_USART2 || defined(__DOXYGEN__)
#if !defined(STM32_USART2_HANDLER)
#error "STM32_USART2_HANDLER not defined"
#endif
/**
 * @brief   USART2 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_USART2_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  serve_interrupt(&S485D2);

  OSAL_IRQ_EPILOGUE();
}
#endif

#if defined(STM32_USART3_8_HANDLER)
#if STM32_SERIAL_485_USE_USART3 || STM32_SERIAL_485_USE_UART4  ||                   \
    STM32_SERIAL_485_USE_UART5  || STM32_SERIAL_485_USE_USART6 ||                   \
    STM32_SERIAL_485_USE_UART7  || STM32_SERIAL_485_USE_UART8  || defined(__DOXYGEN__)
/**
 * @brief   USART2 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_USART3_8_HANDLER) {

  OSAL_IRQ_PROLOGUE();

#if STM32_SERIAL_485_USE_USART3
  serve_interrupt(&S485D3);
#endif
#if STM32_SERIAL_485_USE_UART4
  serve_interrupt(&S485D4);
#endif
#if STM32_SERIAL_485_USE_UART5
  serve_interrupt(&S485D5);
#endif
#if STM32_SERIAL_485_USE_USART6
  serve_interrupt(&S485D6);
#endif

  OSAL_IRQ_EPILOGUE();
}
#endif

#else /* !defined(STM32_USART3_8_HANDLER) */

#if STM32_SERIAL_485_USE_USART3 || defined(__DOXYGEN__)
#if !defined(STM32_USART3_HANDLER)
#error "STM32_USART3_HANDLER not defined"
#endif
/**
 * @brief   USART3 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_USART3_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  serve_interrupt(&S485D3);

  OSAL_IRQ_EPILOGUE();
}
#endif

#if STM32_SERIAL_485_USE_UART4 || defined(__DOXYGEN__)
#if !defined(STM32_UART4_HANDLER)
#error "STM32_UART4_HANDLER not defined"
#endif
/**
 * @brief   UART4 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_UART4_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  serve_interrupt(&S485D4);

  OSAL_IRQ_EPILOGUE();
}
#endif

#if STM32_SERIAL_485_USE_UART5 || defined(__DOXYGEN__)
#if !defined(STM32_UART5_HANDLER)
#error "STM32_UART5_HANDLER not defined"
#endif
/**
 * @brief   UART5 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_UART5_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  serve_interrupt(&S485D5);

  OSAL_IRQ_EPILOGUE();
}
#endif

#if STM32_SERIAL_485_USE_USART6 || defined(__DOXYGEN__)
#if !defined(STM32_USART6_HANDLER)
#error "STM32_USART6_HANDLER not defined"
#endif
/**
 * @brief   USART6 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_USART6_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  serve_interrupt(&S485D6);

  OSAL_IRQ_EPILOGUE();
}
#endif

#endif /* !defined(STM32_USART3_8_HANDLER) */

#if STM32_SERIAL_485_USE_UART7 || defined(__DOXYGEN__)
#if !defined(STM32_UART7_HANDLER)
#error "STM32_UART7_HANDLER not defined"
#endif
/**
 * @brief   UART7 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_UART7_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  serve_interrupt(&S485D7);

  OSAL_IRQ_EPILOGUE();
}
#endif

#if STM32_SERIAL_485_USE_UART8 || defined(__DOXYGEN__)
#if !defined(STM32_UART8_HANDLER)
#error "STM32_UART8_HANDLER not defined"
#endif
/**
 * @brief   UART8 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_UART8_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  serve_interrupt(&S485D8);

  OSAL_IRQ_EPILOGUE();
}
#endif

#if STM32_SERIAL_485_USE_LPUART1 || defined(__DOXYGEN__)
#if !defined(STM32_LPUART1_HANDLER)
#error "STM32_LPUART1_HANDLER not defined"
#endif
/**
 * @brief   LPUART1 interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_LPUART1_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  serve_interrupt(&LPS485D1);

  OSAL_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level serial driver initialization.
 *
 * @notapi
 */
void s485d_lld_init(void) {

#if STM32_SERIAL_485_USE_USART1
  s485dObjectInit(&S485D1, NULL, notify1);
  S485D1.usart = USART1;
  S485D1.clock = STM32_USART1CLK;
#if defined(STM32_USART1_NUMBER)
  nvicEnableVector(STM32_USART1_NUMBER, STM32_SERIAL_485_USART1_PRIORITY);
#endif
#endif

#if STM32_SERIAL_485_USE_USART2
  s485dObjectInit(&S485D2, NULL, notify2);
  S485D2.usart = USART2;
  S485D2.clock = STM32_USART2CLK;
#if defined(STM32_USART2_NUMBER)
  nvicEnableVector(STM32_USART2_NUMBER, STM32_SERIAL_485_USART2_PRIORITY);
#endif
#endif

#if STM32_SERIAL_485_USE_USART3
  s485dObjectInit(&S485D3, NULL, notify3);
  S485D3.usart = USART3;
  S485D3.clock = STM32_USART3CLK;
#if defined(STM32_USART3_NUMBER)
  nvicEnableVector(STM32_USART3_NUMBER, STM32_SERIAL_485_USART3_PRIORITY);
#endif
#endif

#if STM32_SERIAL_485_USE_UART4
  s485dObjectInit(&S485D4, NULL, notify4);
  S485D4.usart = UART4;
  S485D4.clock = STM32_UART4CLK;
#if defined(STM32_UART4_NUMBER)
  nvicEnableVector(STM32_UART4_NUMBER, STM32_SERIAL_485_UART4_PRIORITY);
#endif
#endif

#if STM32_SERIAL_485_USE_UART5
  s485dObjectInit(&S485D5, NULL, notify5);
  S485D5.usart = UART5;
  S485D5.clock = STM32_UART5CLK;
#if defined(STM32_UART5_NUMBER)
  nvicEnableVector(STM32_UART5_NUMBER, STM32_SERIAL_485_UART5_PRIORITY);
#endif
#endif

#if STM32_SERIAL_485_USE_USART6
  s485dObjectInit(&S485D6, NULL, notify6);
  S485D6.usart = USART6;
  S485D6.clock = STM32_USART6CLK;
#if defined(STM32_USART6_NUMBER)
  nvicEnableVector(STM32_USART6_NUMBER, STM32_SERIAL_485_USART6_PRIORITY);
#endif
#endif

#if STM32_SERIAL_485_USE_UART7
  s485dObjectInit(&S485D7, NULL, notify7);
  S485D7.usart = UART7;
  S485D7.clock = STM32_UART7CLK;
#if defined(STM32_UART7_NUMBER)
  nvicEnableVector(STM32_UART7_NUMBER, STM32_SERIAL_485_UART7_PRIORITY);
#endif
#endif

#if STM32_SERIAL_485_USE_UART8
  s485dObjectInit(&S485D8, NULL, notify8);
  S485D8.usart = UART8;
  S485D8.clock = STM32_UART8CLK;
#if defined(STM32_UART8_NUMBER)
  nvicEnableVector(STM32_UART8_NUMBER, STM32_SERIAL_485_UART8_PRIORITY);
#endif
#endif

#if STM32_SERIAL_485_USE_LPUART1
  s485dObjectInit(&LPS485D1, NULL, notifylp1);
  LPS485D1.usart = LPUART1;
  LPS485D1.clock = STM32_LPUART1CLK;
#if defined(STM32_LPUART1_NUMBER)
  nvicEnableVector(STM32_LPUART1_NUMBER, STM32_SERIAL_485_LPUART1_PRIORITY);
#endif
#endif

#if STM32_SERIAL_485_USE_USART3 || STM32_SERIAL_485_USE_UART4  ||                   \
    STM32_SERIAL_485_USE_UART5  || STM32_SERIAL_485_USE_USART6 ||                   \
    STM32_SERIAL_485_USE_UART7  ||  STM32_SERIAL_485_USE_UART8 || defined(__DOXYGEN__)
#if defined(STM32_USART3_8_HANDLER)
  nvicEnableVector(STM32_USART3_8_NUMBER, STM32_SERIAL_485_USART3_8_PRIORITY);
#endif
#endif
}

/**
 * @brief   Low level serial driver configuration and (re)start.
 *
 * @param[in] s485dp    pointer to a @p Serial485Driver object
 * @param[in] config    the architecture-dependent serial driver configuration.
 *                      If this parameter is set to @p NULL then a default
 *                      configuration is used.
 *
 * @notapi
 */
void s485d_lld_start(Serial485Driver *s485dp, const Serial485Config *config) {

  if (config == NULL)
    config = &default_config;

  if (s485dp->state == S485D_STOP) {
#if STM32_SERIAL_485_USE_USART1
    if (&S485D1 == s485dp) {
      rccEnableUSART1(true);
    }
#endif
#if STM32_SERIAL_485_USE_USART2
    if (&S485D2 == s485dp) {
      rccEnableUSART2(true);
    }
#endif
#if STM32_SERIAL_485_USE_USART3
    if (&S485D3 == s485dp) {
      rccEnableUSART3(true);
    }
#endif
#if STM32_SERIAL_485_USE_UART4
    if (&S485D4 == s485dp) {
      rccEnableUART4(true);
    }
#endif
#if STM32_SERIAL_485_USE_UART5
    if (&S485D5 == s485dp) {
      rccEnableUART5(true);
    }
#endif
#if STM32_SERIAL_485_USE_USART6
    if (&S485D6 == s485dp) {
      rccEnableUSART6(true);
    }
#endif
#if STM32_SERIAL_485_USE_UART7
    if (&S485D7 == s485dp) {
      rccEnableUART7(true);
    }
#endif
#if STM32_SERIAL_485_USE_UART8
    if (&S485D8 == s485dp) {
      rccEnableUART8(true);
    }
#endif
#if STM32_SERIAL_485_USE_LPUART1
    if (&LPS485D1 == s485dp) {
      rccEnableLPUART1(true);
    }
#endif
  }
  usart_init(s485dp, config);
}

/**
 * @brief   Low level serial driver stop.
 * @details De-initializes the USART, stops the associated clock, resets the
 *          interrupt vector.
 *
 * @param[in] s485dp    pointer to a @p Serial485Driver object
 *
 * @notapi
 */
void s485d_lld_stop(Serial485Driver *s485dp) {

  if (s485dp->state == S485D_READY) {
    /* UART is de-initialized then clocks are disabled.*/
    usart_deinit(s485dp->usart);

#if STM32_SERIAL_485_USE_USART1
    if (&S485D1 == s485dp) {
      rccDisableUSART1();
      return;
    }
#endif
#if STM32_SERIAL_485_USE_USART2
    if (&S485D2 == s485dp) {
      rccDisableUSART2();
      return;
    }
#endif
#if STM32_SERIAL_485_USE_USART3
    if (&S485D3 == s485dp) {
      rccDisableUSART3();
      return;
    }
#endif
#if STM32_SERIAL_485_USE_UART4
    if (&S485D4 == s485dp) {
      rccDisableUART4();
      return;
    }
#endif
#if STM32_SERIAL_485_USE_UART5
    if (&S485D5 == s485dp) {
      rccDisableUART5();
      return;
    }
#endif
#if STM32_SERIAL_485_USE_USART6
    if (&S485D6 == s485dp) {
      rccDisableUSART6();
      return;
    }
#endif
#if STM32_SERIAL_485_USE_UART7
    if (&S485D7 == s485dp) {
      rccDisableUART7();
      return;
    }
#endif
#if STM32_SERIAL_485_USE_UART8
    if (&S485D8 == s485dp) {
      rccDisableUART8();
      return;
    }
#endif
#if STM32_SERIAL_485_USE_LPUART1
    if (&LPS485D1 == s485dp) {
      rccDisableLPUART1();
      return;
    }
#endif
  }
}

#endif /* HAL_USE_SERIAL_485 */

/** @} */
