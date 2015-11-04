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
 * @file    STM32F0xx/qhal_lld.c
 * @brief   STM32F0xx HAL subsystem low level driver source.
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

/**
 * @brief   ROM image of the startup segment start.
 * @pre     The symbol must be aligned to a 32 bits boundary.
 */
extern uint32_t _textstartup;

/**
 * @brief   Startup segment start.
 * @pre     The symbol must be aligned to a 32 bits boundary.
 */
extern uint32_t __startup_start;

/**
 * @brief   Startup segment end.
 * @pre     The symbol must be aligned to a 32 bits boundary.
 */
extern uint32_t __startup_end;

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
    /* STARTUP segment initialization.*/
    {
        uint32_t *tp, *dp;

        tp = &_textstartup;
        dp = &__startup_start;
        while (dp < &__startup_end)
            *dp++ = *tp++;
    }

    /* Remap address 0 to embedded SRAM */
    rccEnableAPB2(RCC_APB2ENR_SYSCFGEN, TRUE);
    SYSCFG->CFGR1 &= ~(SYSCFG_CFGR1_MEM_MODE);
    SYSCFG->CFGR1 |= (SYSCFG_CFGR1_MEM_MODE_0 | SYSCFG_CFGR1_MEM_MODE_1);
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
    const uint8_t* cpu_uid = (const uint8_t*)0x1ffff7ac;

    memcpy(uid, cpu_uid, 12);
}

/** @} */
