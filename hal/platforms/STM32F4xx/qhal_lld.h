/**
 * @file    STM32F4xx/qhal_lld.h
 * @brief   STM32F4xx HAL subsystem low level driver header.
 * @pre     This module requires the following macros to be defined in the
 *          @p board.h file:
 *          - STM32_LSECLK.
 *          - STM32_LSE_BYPASS (optionally).
 *          - STM32_HSECLK.
 *          - STM32_HSE_BYPASS (optionally).
 *          - STM32_VDD (as hundredths of Volt).
 *          .
 *          One of the following macros must also be defined:
 *          - STM32F401xx for High-performance STM32 F-4 devices.
 *          - STM32F40_41xxx for High-performance STM32 F-4 devices.
 *          - STM32F427_437xx for High-performance STM32 F-4 devices.
 *          - STM32F429_439xx for High-performance STM32 F-4 devices.
 *          .
 *
 * @addtogroup QHAL
 * @{
 */

#ifndef _QHAL_LLD_H_
#define _QHAL_LLD_H_

#include "stm32.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Platform capabilities.                                                    */
/*===========================================================================*/

/**
 * @name    STM32F4xx capabilities
 * @{
 */

/* FLASH attributes.*/
#define STM32_HAS_FLASH         TRUE
/** @} */

/*===========================================================================*/
/* Platform specific friendly IRQ names.                                     */
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

/* STM32 ISR, DMA and RCC helpers.*/
#include "qstm32_isr.h"

#ifdef __cplusplus
extern "C" {
#endif
  void qhal_lld_init(void);
#ifdef __cplusplus
}
#endif

#endif /* _QHAL_LLD_H_ */

/** @} */
