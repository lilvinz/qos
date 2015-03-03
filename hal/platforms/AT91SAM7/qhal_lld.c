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
 * @file    AT91SAM7/qhal_lld.c
 * @brief   AT91SAM7 QHAL subsystem low level driver source.
 *
 * @addtogroup HAL
 * @{
 */

#include "ch.h"
#include "qhal.h"

#include <stdint.h>
#include <stdbool.h>

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

/*
 * @brief: This is a simple trampoline put at the start of the image in flash.
 *
 */
__attribute__((section(".reset_trampoline_1")))
__attribute__((naked))
__attribute__((used))
static void reset_trampoline_1(void)
{
    asm(".arm\n"
        "b       reset_trampoline_2_arm\n");
}

__attribute__((section(".reset_trampoline_2")))
__attribute__((naked))
__attribute__((used))
static void reset_trampoline_2(void)
{
    asm(".arm\n"
        ".global reset_trampoline_2_arm\n"
        "reset_trampoline_2_arm:\n"
        "ldr     pc, _reset\n"
        "_reset:\n"
        ".word   ResetHandler");
}

__attribute__((optimize(1)))
void qhal_lld_init(void)
{
    /* Ensure that 0x00000000 is mapped to sram. */
    {
        /* Save remap value. */
        volatile uint8_t* remap = (volatile uint8_t*)0x00000000;
        volatile uint8_t* ram = (volatile uint8_t*)AT91C_ISRAM;

        /* Probe remap. */
        const bool probe_1 = *remap == *ram;

        /* Modify sram content. */
        *ram += 1;

        const bool probe_2 = *remap == *ram;

        /* Restore sram content. */
        *ram -= 1;

        if (!probe_1 || !probe_2)
        {
            /* 0x00000000 is mapped to flash, remap it to sram. */
            AT91C_BASE_MC->MC_RCR = AT91C_MC_RCB;
        }
    }
}

/** @} */
