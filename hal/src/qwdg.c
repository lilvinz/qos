/**
 * @file    qwdg.c
 * @brief   MCU internal watchdog driver code.
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
 * @brief   WDG internal driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void wdgInit(void)
{
    wdg_lld_init();
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] wdgp     pointer to the @p WDGDriver object
 *
 * @init
 */
void wdgObjectInit(WDGDriver* wdgp)
{
    wdgp->state = WDG_STOP;
    wdgp->config = NULL;
}

/**
 * @brief   Configures and activates the WDG peripheral.
 *
 * @param[in] wdgp      pointer to the @p WDGDriver object
 * @param[in] config    pointer to the @p WDGConfig object.
 *
 * @api
 */
void wdgStart(WDGDriver* wdgp, const WDGConfig* config)
{
    chDbgCheck((wdgp != NULL) && (config != NULL), "wdgStart");

    /* Verify device status. */
    chDbgAssert((wdgp->state == WDG_STOP) || (wdgp->state == WDG_READY),
            "wdgStart(), #1", "invalid state");

    wdgp->config = config;
    wdg_lld_start(wdgp);
    wdgp->state = WDG_READY;
}

/**
 * @brief   Disables the WDG peripheral.
 *
 * @param[in] wdgp      pointer to the @p WDGDriver object
 *
 * @api
 */
void wdgStop(WDGDriver* wdgp)
{
    chDbgCheck(wdgp != NULL, "wdgStop");

    /* Verify device status. */
    chDbgAssert((wdgp->state == WDG_STOP) || (wdgp->state == WDG_READY),
            "wdgStop(), #1", "invalid state");

    wdg_lld_stop(wdgp);
    wdgp->state = WDG_STOP;
}

/**
 * @brief   Reloads the watchdog counter.
 *
 * @param[in] wdgp      pointer to the @p WDGDriver object
 *
 * @api
 */
void wdgReload(WDGDriver* wdgp)
{
    chDbgCheck(wdgp != NULL, "wdgReset");

    /* Verify device status. */
    chDbgAssert(wdgp->state >= WDG_READY, "wdgReset(), #1",
            "invalid state");

    wdg_lld_reload(wdgp);
}

#endif /* HAL_USE_WDG */

/** @} */
