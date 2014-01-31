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
