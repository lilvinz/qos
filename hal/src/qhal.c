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
#if HAL_USE_NVM_FEE || defined(__DOXYGEN__)
    nvmfeeInit();
#endif
#if HAL_USE_FLASH || defined(__DOXYGEN__)
    flashInit();
#endif
#if HAL_USE_LED || defined(__DOXYGEN__)
    ledInit();
#endif
#if HAL_USE_GD_SIM || defined(__DOXYGEN__)
    gdsimInit();
#endif
#if HAL_USE_GD_ILI9341 || defined(__DOXYGEN__)
    gdili9341Init();
#endif
#if HAL_USE_MS5541 || defined(__DOXYGEN__)
    ms5541Init();
#endif
#if HAL_USE_SERIAL_VIRTUAL || defined(__DOXYGEN__)
    sdvirtualInit();
#endif
#if HAL_USE_SERIAL_FDX || defined(__DOXYGEN__)
    sfdxdInit();
#endif
#if HAL_USE_MS58XX || defined(__DOXYGEN__)
    ms58xxInit();
#endif
#if HAL_USE_BQ276XX || defined(__DOXYGEN__)
    bq276xxInit();
#endif
}

/** @} */
