/**
 * @file    AT91SAM7/qwdg_lld.c
 * @brief   AT91SAM7 low level WDG driver code.
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
    WDGD.wdg = AT91C_BASE_WDTC;
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
            wdgp->wdg->WDTC_WDMR = wdgp->config->wdt_mr;
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
    wdgp->wdg->WDTC_WDCR = 0xA5000001;
}

#endif /* HAL_USE_WDG */

/** @} */
