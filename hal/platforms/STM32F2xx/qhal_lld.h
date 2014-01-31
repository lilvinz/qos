/**
 * @file    STM32F2xx/qhal_lld.h
 * @brief   STM32F2xx HAL subsystem low level driver header.
 * @pre     This module requires the following macros to be defined in the
 *          @p board.h file:
 *          - STM32_LSECLK.
 *          - STM32_LSE_BYPASS (optionally).
 *          - STM32_HSECLK.
 *          - STM32_HSE_BYPASS (optionally).
 *          - STM32_VDD (as hundredths of Volt).
 *          .
 *          One of the following macros must also be defined:
 *          - STM32F2XX for High-performance STM32 F-2 devices.
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
 * @name    STM32F2xx capabilities
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

#endif /* _QHAL_LLD_H_ */

/** @} */
