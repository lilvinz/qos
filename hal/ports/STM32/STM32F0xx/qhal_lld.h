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
 * @file    STM32F0xx/qhal_lld.h
 * @brief   STM32F0xx HAL subsystem low level driver header.
 * @pre     This module requires the following macros to be defined in the
 *          @p board.h file:
 *          - STM32_LSECLK.
 *          - STM32_LSEDRV.
 *          - STM32_LSE_BYPASS (optionally).
 *          - STM32_HSECLK.
 *          - STM32_HSE_BYPASS (optionally).
 *          .
 *          One of the following macros must also be defined:
 *          - STM32F030 for Value Line devices.
 *          - STM32F0XX_LD for Low Density devices.
 *          - STM32F0XX_MD for Medium Density devices.
 *          .
 *
 * @addtogroup HAL
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
 * @name    STM32F0xx capabilities
 * @{
 */

/* FLASH attributes.*/
#define STM32_HAS_FLASH         TRUE
/** @} */

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
  void qhal_lld_get_uid(uint8_t uid[12]);
#ifdef __cplusplus
}
#endif

#endif /* _QHAL_LLD_H_ */

/** @} */
