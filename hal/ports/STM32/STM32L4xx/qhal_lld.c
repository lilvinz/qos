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
 * @file    STM32L4xx/qhal_lld.c
 * @brief   STM32L4xx HAL subsystem low level driver source.
 *
 * @addtogroup HAL
 * @{
 */

#include "qhal.h"

#include <string.h>

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
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level HAL driver initialization.
 *
 * @notapi
 */
void qhal_lld_init(void)
{
}

/**
 * @brief   Retrieve mcu GUID.
 *
 * @param[out] uid      array receiving 96 bit mcu guid
 *
 * @notapi
 */
void qhal_lld_get_uid(uint8_t uid[12])
{
    const uint8_t* cpu_uid = (const uint8_t*)0x1fff7590;

    memcpy(uid, cpu_uid, 12);
}

/** @} */
