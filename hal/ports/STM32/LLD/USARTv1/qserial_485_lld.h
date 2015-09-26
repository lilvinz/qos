/**
 * @file    STM32/USARTv1/qserial_485_lld.h
 * @brief   STM32 low level serial driver header.
 *
 * @addtogroup SERIAL_485
 * @{
 */

#ifndef _QSERIAL_485_LLD_H_
#define _QSERIAL_485_LLD_H_

#if HAL_USE_SERIAL_485 || defined(__DOXYGEN__)

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
 * @brief   USART1 driver enable switch.
 * @details If set to @p TRUE the support for USART1 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_SERIAL_485_USE_USART1) || defined(__DOXYGEN__)
#define STM32_SERIAL_485_USE_USART1         FALSE
#endif

/**
 * @brief   USART2 driver enable switch.
 * @details If set to @p TRUE the support for USART2 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_SERIAL_485_USE_USART2) || defined(__DOXYGEN__)
#define STM32_SERIAL_485_USE_USART2         FALSE
#endif

/**
 * @brief   USART3 driver enable switch.
 * @details If set to @p TRUE the support for USART3 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_SERIAL_485_USE_USART3) || defined(__DOXYGEN__)
#define STM32_SERIAL_485_USE_USART3         FALSE
#endif

/**
 * @brief   UART4 driver enable switch.
 * @details If set to @p TRUE the support for UART4 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_SERIAL_485_USE_UART4) || defined(__DOXYGEN__)
#define STM32_SERIAL_485_USE_UART4          FALSE
#endif

/**
 * @brief   UART5 driver enable switch.
 * @details If set to @p TRUE the support for UART5 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_SERIAL_485_USE_UART5) || defined(__DOXYGEN__)
#define STM32_SERIAL_485_USE_UART5          FALSE
#endif

/**
 * @brief   USART6 driver enable switch.
 * @details If set to @p TRUE the support for USART6 is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_SERIAL_485_USE_USART6) || defined(__DOXYGEN__)
#define STM32_SERIAL_485_USE_USART6         FALSE
#endif

/**
 * @brief   USART1 interrupt priority level setting.
 */
#if !defined(STM32_SERIAL_485_SART1_PRIORITY) || defined(__DOXYGEN__)
#define STM32_SERIAL_485_USART1_PRIORITY    12
#endif

/**
 * @brief   USART2 interrupt priority level setting.
 */
#if !defined(STM32_SERIAL_485_USART2_PRIORITY) || defined(__DOXYGEN__)
#define STM32_SERIAL_485_USART2_PRIORITY    12
#endif

/**
 * @brief   USART3 interrupt priority level setting.
 */
#if !defined(STM32_SERIAL_485_USART3_PRIORITY) || defined(__DOXYGEN__)
#define STM32_SERIAL_485_USART3_PRIORITY    12
#endif

/**
 * @brief   UART4 interrupt priority level setting.
 */
#if !defined(STM32_SERIAL_485_UART4_PRIORITY) || defined(__DOXYGEN__)
#define STM32_SERIAL_485_UART4_PRIORITY     12
#endif

/**
 * @brief   UART5 interrupt priority level setting.
 */
#if !defined(STM32_SERIAL_485_UART5_PRIORITY) || defined(__DOXYGEN__)
#define STM32_SERIAL_485_UART5_PRIORITY     12
#endif

/**
 * @brief   USART6 interrupt priority level setting.
 */
#if !defined(STM32_SERIAL_485_USART6_PRIORITY) || defined(__DOXYGEN__)
#define STM32_SERIAL_485_USART6_PRIORITY    12
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if STM32_SERIAL_485_USE_USART1 && !STM32_HAS_USART1
#error "USART1 not present in the selected device"
#endif

#if STM32_SERIAL_485_USE_USART2 && !STM32_HAS_USART2
#error "USART2 not present in the selected device"
#endif

#if STM32_SERIAL_485_USE_USART3 && !STM32_HAS_USART3
#error "USART3 not present in the selected device"
#endif

#if STM32_SERIAL_485_USE_UART4 && !STM32_HAS_UART4
#error "UART4 not present in the selected device"
#endif

#if STM32_SERIAL_485_USE_UART5 && !STM32_HAS_UART5
#error "UART5 not present in the selected device"
#endif

#if STM32_SERIAL_485_USE_USART6 && !STM32_HAS_USART6
#error "USART6 not present in the selected device"
#endif

#if !STM32_SERIAL_485_USE_USART1 && !STM32_SERIAL_485_USE_USART2 &&                 \
    !STM32_SERIAL_485_USE_USART3 && !STM32_SERIAL_485_USE_UART4  &&                 \
    !STM32_SERIAL_485_USE_UART5  && !STM32_SERIAL_485_USE_USART6
#error "SERIAL 485 driver activated but no USART/UART peripheral assigned"
#endif

#if STM32_SERIAL_485_USE_USART1 &&                                              \
    !CH_IRQ_IS_VALID_KERNEL_PRIORITY(STM32_SERIAL_485_USART1_PRIORITY)
#error "Invalid IRQ priority assigned to USART1"
#endif

#if STM32_SERIAL_485_USE_USART2 &&                                              \
    !CH_IRQ_IS_VALID_KERNEL_PRIORITY(STM32_SERIAL_485_USART2_PRIORITY)
#error "Invalid IRQ priority assigned to USART2"
#endif

#if STM32_SERIAL_485_USE_USART3 &&                                              \
    !CH_IRQ_IS_VALID_KERNEL_PRIORITY(STM32_SERIAL_485_USART3_PRIORITY)
#error "Invalid IRQ priority assigned to USART3"
#endif

#if STM32_SERIAL_485_USE_UART4 &&                                               \
    !CH_IRQ_IS_VALID_KERNEL_PRIORITY(STM32_SERIAL_485_UART4_PRIORITY)
#error "Invalid IRQ priority assigned to UART4"
#endif

#if STM32_SERIAL_485_USE_UART5 &&                                               \
    !CH_IRQ_IS_VALID_KERNEL_PRIORITY(STM32_SERIAL_485_UART5_PRIORITY)
#error "Invalid IRQ priority assigned to UART5"
#endif

#if STM32_SERIAL_485_USE_USART6 &&                                              \
    !CH_IRQ_IS_VALID_KERNEL_PRIORITY(STM32_SERIAL_485_USART6_PRIORITY)
#error "Invalid IRQ priority assigned to USART6"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   STM32 Serial Driver configuration structure.
 * @details An instance of this structure must be passed to @p sdStart()
 *          in order to configure and start a serial driver operations.
 * @note    This structure content is architecture dependent, each driver
 *          implementation defines its own version and the custom static
 *          initializers.
 */
typedef struct {
  /**
   * @brief Bit rate.
   */
  uint32_t                  speed;
  /* End of the mandatory fields.*/
  /**
   * @brief Initialization value for the CR1 register.
   */
  uint16_t                  cr1;
  /**
   * @brief Initialization value for the CR2 register.
   */
  uint16_t                  cr2;
  /**
   * @brief Initialization value for the CR3 register.
   */
  uint16_t                  cr3;
  /**
   * @brief The chip select line port.
   */
  ioportid_t                ssport;
  /**
   * @brief The chip select line pad number.
   */
  uint16_t                  sspad;
} Serial485Config;

/**
 * @brief   @p Serial485Driver specific data.
 */
#define _serial_485_driver_data                                             \
  _base_asynchronous_channel_data                                           \
  /* Driver state.*/                                                        \
  s485dstate_t              state;                                          \
  /* Input queue.*/                                                         \
  input_queue_t             iqueue;                                         \
  /* Output queue.*/                                                        \
  output_queue_t            oqueue;                                         \
  /* Input circular buffer.*/                                               \
  uint8_t                   ib[SERIAL_BUFFERS_SIZE];                        \
  /* Output circular buffer.*/                                              \
  uint8_t                   ob[SERIAL_BUFFERS_SIZE];                        \
  /* End of the mandatory fields.*/                                         \
  /* Pointer to the USART registers block.*/                                \
  USART_TypeDef             *usart;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*
 * Extra USARTs definitions here (missing from the ST header file).
 */
#define USART_CR2_STOP1_BITS    (0 << 12)   /**< @brief CR2 1 stop bit value.*/
#define USART_CR2_STOP0P5_BITS  (1 << 12)   /**< @brief CR2 0.5 stop bit value.*/
#define USART_CR2_STOP2_BITS    (2 << 12)   /**< @brief CR2 2 stop bit value.*/
#define USART_CR2_STOP1P5_BITS  (3 << 12)   /**< @brief CR2 1.5 stop bit value.*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if STM32_SERIAL_485_USE_USART1 && !defined(__DOXYGEN__)
extern Serial485Driver S485D1;
#endif
#if STM32_SERIAL_485_USE_USART2 && !defined(__DOXYGEN__)
extern Serial485Driver S485D2;
#endif
#if STM32_SERIAL_485_USE_USART3 && !defined(__DOXYGEN__)
extern Serial485Driver S485D3;
#endif
#if STM32_SERIAL_485_USE_UART4 && !defined(__DOXYGEN__)
extern Serial485Driver S485D4;
#endif
#if STM32_SERIAL_485_USE_UART5 && !defined(__DOXYGEN__)
extern Serial485Driver S485D5;
#endif
#if STM32_SERIAL_485_USE_USART6 && !defined(__DOXYGEN__)
extern Serial485Driver S485D6;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void s485d_lld_init(void);
  void s485d_lld_start(Serial485Driver *sdp, const Serial485Config *config);
  void s485d_lld_stop(Serial485Driver *sdp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SERIAL_485 */

#endif /* _QSERIAL_485_LLD_H_ */

/** @} */
