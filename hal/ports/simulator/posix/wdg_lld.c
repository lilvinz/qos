/**
 * @file    Posix/wdg_lld.c
 * @brief   Posix low level WDG driver code.
 *
 * @addtogroup WDG
 * @{
 */

#include "hal.h"

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
WDGDriver WDGD1;

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
  WDGD1.state = WDG_STOP;
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
        if (wdgp == &WDGD1)
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
        if (wdgp == &WDGD1)
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
void wdg_lld_reset(WDGDriver* wdgp)
{
}

#endif /* HAL_USE_WDG */

/** @} */