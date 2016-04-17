/**
 * @file    qads111x.h
 * @brief   ADS111X driver header.
 *
 * @addtogroup ADS111X
 * @{
 */

#ifndef _QADS111X_H_
#define _QADS111X_H_

#if HAL_USE_ADS111X || defined(__DOXYGEN__)

#include <stdint.h>

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    ADS111X configuration options
 * @{
 */

/**
 * @brief   Enables the @p ads111xAcquireBus() and @p ads111xReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(ADS111X_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define ADS111X_USE_MUTUAL_EXCLUSION    FALSE
#endif

/**
 * @brief   Delays insertions.
 * @details If enabled this options inserts delays into the ADS111X waiting
 *          routines releasing some extra CPU time for the threads with
 *          lower priority, this may slow down the driver a bit however.
 */
#if !defined(ADS111X_NICE_WAITING) || defined(__DOXYGEN__)
#define ADS111X_NICE_WAITING            TRUE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !HAL_USE_I2C
#error "ADS111X driver requires HAL_USE_I2C"
#endif

#if ADS111X_USE_MUTUAL_EXCLUSION && !CH_CFG_USE_MUTEXES && !CH_CFG_USE_SEMAPHORES
#error "ADS111X_USE_MUTUAL_EXCLUSION requires CH_CFG_USE_MUTEXES "
       "and/or CH_CFG_USE_SEMAPHORES"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum
{
    ADS111X_UNINIT = 0,              /**< Not initialized.                   */
    ADS111X_STOP = 1,                /**< Stopped.                           */
    ADS111X_READY = 3,               /**< Device ready.                      */
    ADS111X_ACTIVE= 4,               /**< Device converting.                 */
} ads111xstate_t;

/**
 * @brief   Type of a structure representing an I2C driver.
 */
typedef struct ADS111XDriver ADS111XDriver;

/**
 * @brief   ADS111X over I2C driver configuration structure.
 */
typedef struct
{
    /**
     * @brief I2C driver associated to this ADS111X driver.
     */
    I2CDriver* i2cp;
    uint8_t i2c_address;
    systime_t i2c_timeout;
    uint16_t config_reg;
} ADS111XConfig;

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a ADS111X driver.
 */
struct ADS111XDriver
{
    /**
    * @brief Driver state.
    */
    ads111xstate_t state;
    /**
    * @brief Current configuration data.
    */
    const ADS111XConfig* configp;
#if ADS111X_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief Mutex protecting the device.
     */
    Mutex mutex;
#elif CH_USE_SEMAPHORES
    Semaphore semaphore;
#endif
#endif /* ADS111X_USE_MUTUAL_EXCLUSION */
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
    void ads111xInit(void);
    void ads111xObjectInit(ADS111XDriver* ads111xp);
    void ads111xStart(ADS111XDriver* ads111xp, const ADS111XConfig* configp);
    void ads111xStop(ADS111XDriver* ads111xp);
    void ads111xAcquireBus(ADS111XDriver* ads111xp);
    void ads111xReleaseBus(ADS111XDriver* ads111xp);
    bool ads111xStartConversion(ADS111XDriver* ads111xp, uint16_t config);
    bool ads111xReadResult(ADS111XDriver* ads111xp, int16_t* datap);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_ADS111X */

#endif /* _QADS111X_H_ */

/** @} */
