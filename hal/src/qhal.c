/*
 * qhal.c
 *
 *  Created on: 22.12.2013
 *      Author: vke
 */

/**
 * @file    qhal.c
 * @brief   Quantec HAL subsystem code.
 *
 * @addtogroup HAL
 * @{
 */

#include "qhal.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

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
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Quantec HAL initialization.
 * @details This function invokes the low level initialization code for
 *          additional drivers. It has to be invoked from boardInit().
 *
 * @init
 */
void qhalInit(void)
{
    qhal_lld_init();

#if HAL_USE_SERIAL_485 || defined(__DOXYGEN__)
    s485dInit();
#endif
#if HAL_USE_FLASH_JEDEC_SPI || defined(__DOXYGEN__)
    fjsInit();
#endif
#if HAL_USE_NVM_PARTITION || defined(__DOXYGEN__)
    nvmpartInit();
#endif
#if HAL_USE_NVM_FILE || defined(__DOXYGEN__)
    nvmfileInit();
#endif
#if HAL_USE_NVM_MEMORY || defined(__DOXYGEN__)
    nvmmemoryInit();
#endif
#if HAL_USE_NVM_MIRROR || defined(__DOXYGEN__)
    nvmmirrorInit();
#endif
#if HAL_USE_FLASH || defined(__DOXYGEN__)
    flashInit();
#endif
#if HAL_USE_LED || defined(__DOXYGEN__)
    ledInit();
#endif
#if HAL_USE_WDG || defined(__DOXYGEN__)
    wdgInit();
#endif
#if HAL_USE_GD_SIM || defined(__DOXYGEN__)
    gdsimInit();
#endif
#if HAL_USE_GD_ILI9341 || defined(__DOXYGEN__)
    gdili9341Init();
#endif
}

/** @} */
