/**
 * @file    STM32/USARTv1/qserial_485_lld.c
 * @brief   STM32 low level serial 485 driver code.
 *
 * @addtogroup SERIAL_485
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_SERIAL_485 || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

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

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/** @brief Driver default configuration.*/
static const Serial485Config default_config =
{
  SERIAL_DEFAULT_BITRATE,
  0,
  USART_CR2_STOP1_BITS | USART_CR2_LINEN,
  0,
  0,
  0,
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
#if STM32_HAS_USART6
  if ((s485dp->usart == USART1) || (s485dp->usart == USART6))
#else
  if (s485dp->usart == USART1)
#endif
    u->BRR = STM32_PCLK2 / config->speed;
  else
    u->BRR = STM32_PCLK1 / config->speed;

  /* Note that some bits are enforced.*/
  u->CR2 = config->cr2 | USART_CR2_LBDIE;
  u->CR3 = config->cr3 | USART_CR3_EIE;
  u->CR1 = config->cr1 | USART_CR1_UE | USART_CR1_PEIE |
                         USART_CR1_RXNEIE | USART_CR1_TE |
                         USART_CR1_RE;
  u->SR = 0;
  (void)u->SR;  /* SR reset step 1.*/
  (void)u->DR;  /* SR reset step 2.*/
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
 * @param[in] sr        USART SR register value
 */
static void set_error(Serial485Driver *s485dp, uint16_t sr) {
  flagsmask_t sts = 0;

  if (sr & USART_SR_ORE)
    sts |= S485D_OVERRUN_ERROR;
  if (sr & USART_SR_PE)
    sts |= S485D_PARITY_ERROR;
  if (sr & USART_SR_FE)
    sts |= S485D_FRAMING_ERROR;
  if (sr & USART_SR_NE)
    sts |= S485D_NOISE_ERROR;
  chnAddFlagsI(s485dp, sts);
}

/**
 * @brief   Common IRQ handler.
 *
 * @param[in] s485dp    communication channel associated to the USART
 */
static void serve_interrupt(Serial485Driver *s485dp) {
  USART_TypeDef *u = s485dp->usart;
  uint16_t cr1 = u->CR1;
  uint16_t sr = u->SR;

  /* Special case, LIN break detection.*/
  if (sr & USART_SR_LBD) {
    chSysLockFromISR();
    chnAddFlagsI(s485dp, S485D_BREAK_DETECTED);
    chSysUnlockFromISR();
    u->SR = ~USART_SR_LBD;
  }

  /* Data available.*/
  chSysLockFromISR();
  while (sr & (USART_SR_RXNE | USART_SR_ORE | USART_SR_NE | USART_SR_FE |
               USART_SR_PE)) {
    /* Error condition detection.*/
    if (sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE))
      set_error(s485dp, sr);
    uint16_t c = u->DR;
    if (sr & USART_SR_RXNE) {
      /* Mask parity bit according to configuration. */
      switch (u->CR1 & (USART_CR1_PCE | USART_CR1_M))
      {
      case USART_CR1_PCE:
          c &= 0x7f;
          break;
      case USART_CR1_PCE | USART_CR1_M:
          c &= 0xff;
          break;
      default:
          break;
      }
      s485dIncomingDataI(s485dp, c);
    }
    sr = u->SR;
  }
  chSysUnlockFromISR();

  /* Physical transmission end.
   * Note: This must be handled before TXE to prevent a startup glitch
   * on the driver enable pin because TC fires once on driver startup.
   */
  if (sr & USART_SR_TC) {
    /* Clear driver enable pad. */
    if (s485dp->config->ssport != NULL)
      palClearPad(s485dp->config->ssport, s485dp->config->sspad);
    chSysLockFromISR();
    if (chOQIsEmptyI(&s485dp->oqueue))
        chnAddFlagsI(s485dp, CHN_TRANSMISSION_END);
    u->CR1 = (cr1 & ~USART_CR1_TCIE) | USART_CR1_RE;
    u->SR = ~USART_SR_TC;
    chSysUnlockFromISR();
  }

  /* Transmission buffer empty.*/
  if ((cr1 & USART_CR1_TXEIE) && (sr & USART_SR_TXE)) {
    msg_t b;
    chSysLockFromISR();
    b = chOQGetI(&s485dp->oqueue);
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
      u->DR = b;
    }
    chSysUnlockFromISR();
  }
}

#if STM32_SERIAL_485_USE_USART1 || defined(__DOXYGEN__)
static void notify1(GenericQueue *qp) {

  (void)qp;
  USART1->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_485_USE_USART2 || defined(__DOXYGEN__)
static void notify2(GenericQueue *qp) {

  (void)qp;
  USART2->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_485_USE_USART3 || defined(__DOXYGEN__)
static void notify3(GenericQueue *qp) {

  (void)qp;
  USART3->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_485_USE_UART4 || defined(__DOXYGEN__)
static void notify4(GenericQueue *qp) {

  (void)qp;
  UART4->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_485_USE_UART5 || defined(__DOXYGEN__)
static void notify5(GenericQueue *qp) {

  (void)qp;
  UART5->CR1 |= USART_CR1_TXEIE;
}
#endif

#if STM32_SERIAL_485_USE_USART6 || defined(__DOXYGEN__)
static void notify6(GenericQueue *qp) {

  (void)qp;
  USART6->CR1 |= USART_CR1_TXEIE;
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
CH_IRQ_HANDLER(STM32_USART1_HANDLER) {

  CH_IRQ_PROLOGUE();

  serve_interrupt(&S485D1);

  CH_IRQ_EPILOGUE();
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
CH_IRQ_HANDLER(STM32_USART2_HANDLER) {

  CH_IRQ_PROLOGUE();

  serve_interrupt(&S485D2);

  CH_IRQ_EPILOGUE();
}
#endif

#if STM32_SERIAL_485_USE_USART3 || defined(__DOXYGEN__)
#if !defined(STM32_USART3_HANDLER)
#error "STM32_USART3_HANDLER not defined"
#endif
/**
 * @brief   USART3 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_USART3_HANDLER) {

  CH_IRQ_PROLOGUE();

  serve_interrupt(&S485D3);

  CH_IRQ_EPILOGUE();
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
CH_IRQ_HANDLER(STM32_UART4_HANDLER) {

  CH_IRQ_PROLOGUE();

  serve_interrupt(&S485D4);

  CH_IRQ_EPILOGUE();
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
CH_IRQ_HANDLER(STM32_UART5_HANDLER) {

  CH_IRQ_PROLOGUE();

  serve_interrupt(&S485D5);

  CH_IRQ_EPILOGUE();
}
#endif

#if STM32_SERIAL_485_USE_USART6 || defined(__DOXYGEN__)
#if !defined(STM32_USART6_HANDLER)
#error "STM32_USART6_HANDLER not defined"
#endif
/**
 * @brief   USART1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_USART6_HANDLER) {

  CH_IRQ_PROLOGUE();

  serve_interrupt(&S485D6);

  CH_IRQ_EPILOGUE();
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
#endif

#if STM32_SERIAL_485_USE_USART2
  s485dObjectInit(&S485D2, NULL, notify2);
  S485D2.usart = USART2;
#endif

#if STM32_SERIAL_485_USE_USART3
  s485dObjectInit(&S485D3, NULL, notify3);
  S485D3.usart = USART3;
#endif

#if STM32_SERIAL_485_USE_UART4
  s485dObjectInit(&S485D4, NULL, notify4);
  S485D4.usart = UART4;
#endif

#if STM32_SERIAL_485_USE_UART5
  s485dObjectInit(&S485D5, NULL, notify5);
  S485D5.usart = UART5;
#endif

#if STM32_SERIAL_485_USE_USART6
  s485dObjectInit(&S485D6, NULL, notify6);
  S485D6.usart = USART6;
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
      rccEnableUSART1(FALSE);
      nvicEnableVector(STM32_USART1_NUMBER, STM32_SERIAL_485_USART1_PRIORITY);
    }
#endif
#if STM32_SERIAL_485_USE_USART2
    if (&S485D2 == s485dp) {
      rccEnableUSART2(FALSE);
      nvicEnableVector(STM32_USART2_NUMBER, STM32_SERIAL_485_USART2_PRIORITY);
    }
#endif
#if STM32_SERIAL_485_USE_USART3
    if (&S485D3 == s485dp) {
      rccEnableUSART3(FALSE);
      nvicEnableVector(STM32_USART3_NUMBER, STM32_SERIAL_485_USART3_PRIORITY);
    }
#endif
#if STM32_SERIAL_485_USE_UART4
    if (&S485D4 == s485dp) {
      rccEnableUART4(FALSE);
      nvicEnableVector(STM32_UART4_NUMBER, STM32_SERIAL_485_UART4_PRIORITY);
    }
#endif
#if STM32_SERIAL_485_USE_UART5
    if (&S485D5 == s485dp) {
      rccEnableUART5(FALSE);
      nvicEnableVector(STM32_UART5_NUMBER, STM32_SERIAL_485_UART5_PRIORITY);
    }
#endif
#if STM32_SERIAL_485_USE_USART6
    if (&S485D6 == s485dp) {
      rccEnableUSART6(FALSE);
      nvicEnableVector(STM32_USART6_NUMBER, STM32_SERIAL_485_USART6_PRIORITY);
    }
#endif
    /* Clear driver enable pad. */
    if (s485dp->config->ssport != NULL)
      palClearPad(s485dp->config->ssport, s485dp->config->sspad);
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
    usart_deinit(s485dp->usart);
#if STM32_SERIAL_485_USE_USART1
    if (&S485D1 == s485dp) {
      rccDisableUSART1(FALSE);
      nvicDisableVector(STM32_USART1_NUMBER);
      return;
    }
#endif
#if STM32_SERIAL_485_USE_USART2
    if (&S485D2 == s485dp) {
      rccDisableUSART2(FALSE);
      nvicDisableVector(STM32_USART2_NUMBER);
      return;
    }
#endif
#if STM32_SERIAL_485_USE_USART3
    if (&S485D3 == s485dp) {
      rccDisableUSART3(FALSE);
      nvicDisableVector(STM32_USART3_NUMBER);
      return;
    }
#endif
#if STM32_SERIAL_485_USE_UART4
    if (&S485D4 == s485dp) {
      rccDisableUART4(FALSE);
      nvicDisableVector(STM32_UART4_NUMBER);
      return;
    }
#endif
#if STM32_SERIAL_485_USE_UART5
    if (&S485D5 == s485dp) {
      rccDisableUART5(FALSE);
      nvicDisableVector(STM32_UART5_NUMBER);
      return;
    }
#endif
#if STM32_SERIAL_485_USE_USART6
    if (&S485D6 == s485dp) {
      rccDisableUSART6(FALSE);
      nvicDisableVector(STM32_USART6_NUMBER);
      return;
    }
#endif
    /* Clear driver enable pad. */
    if (s485dp->config->ssport != NULL)
      palClearPad(s485dp->config->ssport, s485dp->config->sspad);
  }
}

#endif /* HAL_USE_SERIAL_485 */

/** @} */
