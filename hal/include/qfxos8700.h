/**
 * @file    qfxos8700.h
 * @brief   FXOS8700 driver header.
 *
 * @addtogroup FXOS8700
 * @{
 */

#ifndef _QFXOS8700_H_
#define _QFXOS8700_H_

#if HAL_USE_FXOS8700 || defined(__DOXYGEN__)

#include <stdint.h>

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    FXOS8700 configuration options
 * @{
 */

/**
 * @brief   Enables the @p fxos8700AcquireBus() and @p fxos8700ReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(FXOS8700_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define FXOS8700_USE_MUTUAL_EXCLUSION     FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !HAL_USE_I2C
#error "FXOS8700 driver requires HAL_USE_I2C"
#endif

#if FXOS8700_USE_MUTUAL_EXCLUSION && !CH_USE_MUTEXES && !CH_USE_SEMAPHORES
#error "FXOS8700_USE_MUTUAL_EXCLUSION requires CH_USE_MUTEXES "
       "and/or CH_USE_SEMAPHORES"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Possible values for api oversampling argument.
 */
typedef struct
{
    uint16_t accel_x;
    uint16_t accel_y;
    uint16_t accel_z;
    uint16_t mag_x;
    uint16_t mag_y;
    uint16_t mag_z;
} fxos8700_data_s;

/**
 * @brief   Driver state machine possible states.
 */
typedef enum
{
    FXOS8700_UNINIT = 0,              /**< Not initialized.                   */
    FXOS8700_STOP = 1,                /**< Stopped.                           */
    FXOS8700_READY = 3,               /**< Device ready.                      */
    FXOS8700_ACTIVE= 4,               /**< Device converting.                 */
} fxos8700state_t;

/**
 * @brief   Type of a structure representing an I2C driver.
 */
typedef struct FXOS8700Driver FXOS8700Driver;

/**
 * @brief   FXOS8700 over I2C driver configuration structure.
 */
typedef struct
{
    /**
     * @brief I2C driver associated to this FXOS8700 driver.
     */
    I2CDriver* i2cp;
    uint8_t i2c_address;
    systime_t i2c_timeout;
} FXOS8700Config;

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a FXOS8700 driver.
 */
struct FXOS8700Driver
{
    /**
    * @brief Driver state.
    */
    fxos8700state_t state;
    /**
    * @brief Current configuration data.
    */
    const FXOS8700Config* configp;
#if FXOS8700_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief Mutex protecting the device.
     */
    Mutex mutex;
#elif CH_USE_SEMAPHORES
    Semaphore semaphore;
#endif
#endif /* FXOS8700_USE_MUTUAL_EXCLUSION */
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
    void fxos8700Init(void);
    void fxos8700ObjectInit(FXOS8700Driver* fxos8700p);
    void fxos8700Start(FXOS8700Driver* fxos8700p, const FXOS8700Config* configp);
    void fxos8700Stop(FXOS8700Driver* fxos8700p);
    void fxos8700AcquireBus(FXOS8700Driver* fxos8700p);
    void fxos8700ReleaseBus(FXOS8700Driver* fxos8700p);
    bool fxos8700ReadData(FXOS8700Driver* fxos8700p, fxos8700_data_s* datap);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_FXOS8700 */

#endif /* _QFXOS8700_H_ */

/** @} */
