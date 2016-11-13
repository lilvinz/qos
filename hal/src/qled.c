/**
 * @file    qled.c
 * @brief   Driver for status and notification LEDs.
 *
 * @addtogroup LED
 * @{
 */

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
static void blink_timer_off_cb(void *par);
static void blink_timer_on_cb(void *par);

static void led_on(LedDriver* ledp)
{
    if (ledp->config->drive == LED_ACTIVE_LOW)
        palClearPad(ledp->config->ledport, ledp->config->ledpad);
    else
        palSetPad(ledp->config->ledport, ledp->config->ledpad);
}

static void led_off(LedDriver* ledp)
{
    if (ledp->config->drive == LED_ACTIVE_LOW)
        palSetPad(ledp->config->ledport, ledp->config->ledpad);
    else
        palClearPad(ledp->config->ledport, ledp->config->ledpad);
}

static void blink_timer_on_cb(void *par)
{
    LedDriver* ledp = (LedDriver*)par;

    osalSysLockFromISR();

    /* Set new timer. */
    chVTSetI(&ledp->blink_vt, ledp->blink_off, blink_timer_off_cb, ledp);

    led_off(ledp);

    osalSysUnlockFromISR();
}

static void blink_timer_off_cb(void *par)
{
    LedDriver* ledp = (LedDriver*)par;

    osalSysLockFromISR();

    /* Decrement counter. */
    if (ledp->blink_loop > 0)
        --ledp->blink_loop;

    if (ledp->blink_loop != 0)
    {
        /* Set new timer. */
        chVTSetI(&ledp->blink_vt, ledp->blink_on, blink_timer_on_cb, ledp);

        led_on(ledp);
    }

    osalSysUnlockFromISR();
}

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
    osalDbgCheck(ledp != NULL);
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
    osalDbgCheck(ledp != NULL);
    osalDbgCheck(config != NULL);

    osalSysLock();

    osalDbgAssert((ledp->state == LED_STOP) || (ledp->state == LED_READY),
            "invalid state");

    ledp->config = config;

    ledp->state = LED_READY;

    osalSysUnlock();
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
    osalDbgCheck(ledp != NULL);

    osalSysLock();

    /* Reset blink timer in case it is armed. */
    chVTResetI(&ledp->blink_vt);

    osalDbgAssert((ledp->state == LED_STOP) || (ledp->state == LED_READY),
            "invalid state");

    /* Driver in stopped state. */
    ledp->state = LED_STOP;

    osalSysUnlock();
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
    osalSysLock();
    ledOnI(ledp);
    osalSysUnlock();
}

/**
 * @brief   Switches LED on.
 *
 * @param[in] ledp      pointer to a @p LedDriver object
 *
 * @iclass
 */
void ledOnI(LedDriver* ledp)
{
    osalDbgCheck(ledp != NULL);
    /* Verify device status. */
    osalDbgAssert(ledp->state >= LED_READY, "invalid state");

    /* Reset blink timer in case it is armed. */
    chVTResetI(&ledp->blink_vt);

    led_on(ledp);
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
    osalSysLock();
    ledOffI(ledp);
    osalSysUnlock();
}

/**
 * @brief   Switches LED off.
 *
 * @param[in] ledp      pointer to a @p LedDriver object
 *
 * @iclass
 */
void ledOffI(LedDriver* ledp)
{
    osalDbgCheck(ledp != NULL);
    /* Verify device status. */
    osalDbgAssert(ledp->state >= LED_READY, "invalid state");

    /* Reset blink timer in case it is armed. */
    chVTResetI(&ledp->blink_vt);

    led_off(ledp);
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
    osalSysLock();
    ledToggleI(ledp);
    osalSysUnlock();
}

/**
 * @brief   Toggle LED state.
 *
 * @param[in] ledp      pointer to a @p LedDriver object
 *
 * @iclass
 */
void ledToggleI(LedDriver* ledp)
{
    osalDbgCheck(ledp != NULL);
    /* Verify device status. */
    osalDbgAssert(ledp->state >= LED_READY, "invalid state");

    /* Reset blink timer in case it is armed. */
    chVTResetI(&ledp->blink_vt);

    palTogglePad(ledp->config->ledport, ledp->config->ledpad);
}

/**
 * @brief   Blink LED with defined period and number of loops.
 *
 * @param[in] ledp      pointer to a @p LedDriver object
 * @param[in] on        time specifier for led on duration
 * @param[in] off       time specifier for led off duration
 * @param[in] loop      number of blink periods or <= 0 for infinite loop
 *
 * @api
 */
void ledBlink(LedDriver* ledp, systime_t on, systime_t off, int32_t loop)
{
    osalSysLock();
    ledBlinkI(ledp, on, off, loop);
    osalSysUnlock();
}

/**
 * @brief   Blink LED with defined period and number of loops.
 *
 * @param[in] ledp      pointer to a @p LedDriver object
 * @param[in] on        time specifier for led on duration
 * @param[in] off       time specifier for led off duration
 * @param[in] loop      number of blink periods or <= 0 for infinite loop
 *
 * @iclass
 */
void ledBlinkI(LedDriver* ledp, systime_t on, systime_t off, int32_t loop)
{
    osalDbgCheck(ledp != NULL);
    /* Verify device status. */
    osalDbgAssert(ledp->state >= LED_READY, "invalid state");
    /* Verify parameters. */
    osalDbgAssert(on > TIME_IMMEDIATE && off > TIME_IMMEDIATE,
            "invalid parameters");

    if (loop <= 0)
        loop = -1;

    ledp->blink_on = on;
    ledp->blink_off = off;
    ledp->blink_loop = loop;

    /* Set new timer. */
    chVTSetI(&ledp->blink_vt, ledp->blink_on, blink_timer_on_cb, ledp);

    led_on(ledp);
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
bool ledIsLedOn(LedDriver* ledp)
{
    osalSysLock();
    bool result = ledIsLedOnI(ledp);
    osalSysUnlock();

    return result;
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
 * @iclass
 */
bool ledIsLedOnI(LedDriver* ledp)
{
    osalDbgCheck(ledp != NULL);
    /* Verify device status. */
    osalDbgAssert(ledp->state >= LED_READY, "invalid state");

    uint8_t palstate = palReadPad(ledp->config->ledport, ledp->config->ledpad);

    if (ledp->config->drive == LED_ACTIVE_LOW)
        return palstate == PAL_LOW ? TRUE : FALSE;
    else
        return palstate == PAL_HIGH ? TRUE : FALSE;
}

#endif /* HAL_USE_LED */

/** @} */
