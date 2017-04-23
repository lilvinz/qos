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
 * @file    STM32/WDGv1/qwdg.c
 * @brief   STM32 low level WDG driver code.
 *
 * @addtogroup WDG
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_WDG || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief WDG driver identifier.
 */
WDGDriver WDGD;

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
 * @brief   Low level WDG driver initialization.
 *
 * @notapi
 */
void wdg_lld_init(void)
{
    wdgObjectInit(&WDGD);
    WDGD.wdg = IWDG;
}

/**
 * @brief   Configures and activates the WDG peripheral.
 *
 * @param[in] wdgp      pointer to the @p WDGDriver object
 *
 * @notapi
 */
void wdg_lld_start(WDGDriver* wdgp)
{
    if (wdgp->state == WDG_STOP)
    {
        if (wdgp == &WDGD)
        {
            /* Enable register write access. */
            wdgp->wdg->KR = (uint16_t)0x5555;

            /* Set prescaler. */
            while (wdgp->wdg->SR & IWDG_SR_PVU)
                ;
            wdgp->wdg->PR = wdgp->config->prescaler;

            /* Set reload value. */
            while (wdgp->wdg->SR & IWDG_SR_RVU)
                ;
            wdgp->wdg->RLR = wdgp->config->reload;

            /* Enable watchdog. */
            wdgp->wdg->KR = (uint16_t)0xcccc;
            return;
        }
    }
}

/**
 * @brief   Deactivates the WDG peripheral.
 *
 * @param[in] wdgp    pointer to the @p WDGDriver object
 *
 * @notapi
 */
void wdg_lld_stop(WDGDriver* wdgp)
{
    if (wdgp->state == WDG_READY)
    {
        if (wdgp == &WDGD)
        {

            return;
        }
    }
}

/**
 * @brief   Reloads the watchdog counter.
 *
 * @param[in] wdgp    pointer to the @p WDGDriver object
 *
 * @notapi
 */
void wdg_lld_reload(WDGDriver* wdgp)
{
    wdgp->wdg->KR = (uint16_t)0xaaaa;
}

#endif /* HAL_USE_WDG */

/** @} */
