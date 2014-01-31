/**
 * @file    qled.c
 * @brief   Driver for status and notification LEDs.
 *
 * @addtogroup LED
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_LED || defined(__DOXYGEN__)

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
 * @brief   LED Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void ledInit(void)
{
}

/**
 * @brief   Initializes a generic driver object.
 * @details The HW dependent part of the initialization has to be performed
 *          outside, usually in the hardware initialization code.
 *
 * @param[out] ledp     pointer to a @p LedDriver structure
 *
 * @init
 */
void ledObjectInit(LedDriver* ledp)
{
    chDbgCheck(ledp != NULL, "ledObjectInit");
    ledp->state = LED_STOP;
}

/**
 * @brief   Configures and starts the driver.
 *
 * @param[in] ledp      pointer to a @p LedDriver object
 * @param[in] config    led driver configuration
 *
 * @api
 */
void ledStart(LedDriver* ledp, const LedConfig* config)
{
    chDbgCheck(ledp != NULL, "ledStart");
    chDbgCheck(config != NULL, "ledStart");

    chSysLock();

    chDbgAssert((ledp->state == LED_STOP) || (ledp->state == LED_READY),
            "ledStart(), #1",
            "invalid state");

    ledp->config = config;

    ledp->state = LED_READY;

    chSysUnlock();
}

/**
 * @brief   Stops the driver.
 *
 * @param[in] ledp      pointer to a @p LedDriver object
 *
 * @api
 */
void ledStop(LedDriver* ledp)
{
    chDbgCheck(ledp != NULL, "ledStop");

    chSysLock();

    chDbgAssert((ledp->state == LED_STOP) || (ledp->state == LED_READY),
            "ledStop(), #1",
            "invalid state");

    /* Driver in stopped state. */
    ledp->state = LED_STOP;

    chSysUnlock();
}

/**
 * @brief   Switches LED on.
 *
 * @param[in] ledp      pointer to a @p LedDriver object
 *
 * @api
 */
void ledOn(LedDriver* ledp)
{
    chDbgCheck(ledp != NULL, "ledOn");
    /* Verify device status. */
    chDbgAssert(ledp->state >= LED_READY, "ledOn(), #1",
            "invalid state");

    if (ledp->config->drive == LED_ACTIVE_LOW)
        palClearPad(ledp->config->ledport, ledp->config->ledpad);
    else
        palSetPad(ledp->config->ledport, ledp->config->ledpad);
}

/**
 * @brief   Switches LED off.
 *
 * @param[in] ledp      pointer to a @p LedDriver object
 *
 * @api
 */
void ledOff(LedDriver* ledp)
{
    chDbgCheck(ledp != NULL, "ledOff");
    /* Verify device status. */
    chDbgAssert(ledp->state >= LED_READY, "ledOff(), #1",
            "invalid state");

    if (ledp->config->drive == LED_ACTIVE_LOW)
        palSetPad(ledp->config->ledport, ledp->config->ledpad);
    else
        palClearPad(ledp->config->ledport, ledp->config->ledpad);
}

/**
 * @brief   Toggle LED state.
 *
 * @param[in] ledp      pointer to a @p LedDriver object
 *
 * @api
 */
void ledToggle(LedDriver* ledp)
{
    chDbgCheck(ledp != NULL, "ledToggle");
    /* Verify device status. */
    chDbgAssert(ledp->state >= LED_READY, "ledToggle(), #1",
            "invalid state");

    palTogglePad(ledp->config->ledport, ledp->config->ledpad);
}

/**
 * @brief   Request current LED state.
 *
 * @param[in] ledp      pointer to a @p LedDriver object
 *
 * @return              The logical status of the led.
 * @retval TRUE         The led is logically on.
 * @retval FALSE        The led is logically off.
 *
 * @api
 */
bool_t ledIsLedOn(LedDriver* ledp)
{
    chDbgCheck(ledp != NULL, "ledIsLedOn");
    /* Verify device status. */
    chDbgAssert(ledp->state >= LED_READY, "ledIsLedOn(), #1",
            "invalid state");

    uint8_t palstate = palReadPad(ledp->config->ledport, ledp->config->ledpad);

    if (ledp->config->drive == LED_ACTIVE_LOW)
        return palstate == PAL_LOW ? TRUE : FALSE;
    else
        return palstate == PAL_HIGH ? TRUE : FALSE;
}

#endif /* HAL_USE_LED */

/** @} */
