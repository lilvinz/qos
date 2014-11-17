/**
 * @file    qled.h
 * @brief   Driver for LEDs.
 *
 * @addtogroup LED
 * @{
 */

#ifndef _QLED_H_
#define _QLED_H_

#if HAL_USE_LED || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief Driver state machine possible states.
 */
typedef enum
{
    LED_UNINIT = 0,                  /**< Not initialized.                   */
    LED_STOP = 1,                    /**< Stopped.                           */
    LED_READY = 2                    /**< Ready.                             */
} ledstate_t;

/**
 * @brief Possible pin states to energize LED.
 */
typedef enum
{
    LED_ACTIVE_LOW = 0,             /**< Low active LED.                     */
    LED_ACTIVE_HIGH = 1,            /**< High active LED.                    */
} ledactive_t;

/**
 * @brief   Serial over USB Driver configuration structure.
 * @details An instance of this structure must be passed to @p sduStart()
 *          in order to configure and start the driver operations.
 */
typedef struct
{
    /**
     * @brief The LED port.
     */
    ioportid_t ledport;
    /**
     * @brief The LED pad number.
     */
    uint16_t ledpad;
    /**
     * @brief The pin state for energized LED.
     */
    ledactive_t drive;
} LedConfig;

/**
 * @brief   Structure representing a led driver.
 */
typedef struct
{
    /* Driver state */
    ledstate_t state;
    /* Current configuration data */
    const LedConfig* config;
    /* Blink timer state */
    VirtualTimer blink_vt;
    systime_t blink_on;
    systime_t blink_off;
    int32_t blink_loop;
} LedDriver;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif
    void ledInit(void);
    void ledObjectInit(LedDriver* ledp);
    void ledStart(LedDriver* ledp, const LedConfig* config);
    void ledStop(LedDriver* ledp);
    void ledOn(LedDriver* ledp);
    void ledOff(LedDriver* ledp);
    void ledToggle(LedDriver* ledp);
    void ledBlink(LedDriver* ledp, systime_t on, systime_t off, int32_t loop);
    bool_t ledIsLedOn(LedDriver* ledp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_LED */

#endif /* _QLED_H_ */

/** @} */
