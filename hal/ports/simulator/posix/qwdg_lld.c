/**
 * @file    Posix/qwdg.c
 * @brief   Posix low level WDG driver code.
 *
 * @addtogroup WDG
 * @{
 */

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
}

#endif /* HAL_USE_WDG */

/** @} */
