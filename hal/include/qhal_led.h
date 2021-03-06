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
    virtual_timer_t blink_vt;
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
    void ledOnI(LedDriver* ledp);
    void ledOff(LedDriver* ledp);
    void ledOffI(LedDriver* ledp);
    void ledToggle(LedDriver* ledp);
    void ledToggleI(LedDriver* ledp);
    void ledBlink(LedDriver* ledp, systime_t on, systime_t off, int32_t loop);
    void ledBlinkI(LedDriver* ledp, systime_t on, systime_t off, int32_t loop);
    bool ledIsLedOn(LedDriver* ledp);
    bool ledIsLedOnI(LedDriver* ledp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_LED */

#endif /* _QLED_H_ */

/** @} */
