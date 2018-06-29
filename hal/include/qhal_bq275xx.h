/**
 * @file    qbq275xx.h
 * @brief   BQ275XX driver header.
 *
 * @addtogroup BQ275XX
 * @{
 */

#ifndef _QBQ275XX_H_
#define _QBQ275XX_H_

#if HAL_USE_BQ275XX || defined(__DOXYGEN__)

#include <stdint.h>

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    BQ275XX configuration options
 * @{
 */

/**
 * @brief   Delays insertions.
 * @details If enabled this options inserts unpolled delays into the polling
 *          routines releasing some extra CPU time for the threads with
 *          lower priority, this may slow down the driver a bit however.
 */
#if !defined(BQ275XX_NICE_WAITING) || defined(__DOXYGEN__)
#define BQ275XX_NICE_WAITING            TRUE
#endif

/**
 * @brief   Enables the @p bq275xxAcquireBus() and @p bq275xxReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(BQ275XX_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define BQ275XX_USE_MUTUAL_EXCLUSION     FALSE
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !HAL_USE_I2C
#error "BQ275XX driver requires HAL_USE_I2C"
#endif

#if BQ275XX_USE_MUTUAL_EXCLUSION && !CH_USE_MUTEXES && !CH_USE_SEMAPHORES
#error "BQ275XX_USE_MUTUAL_EXCLUSION requires CH_USE_MUTEXES "
       "and/or CH_USE_SEMAPHORES"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum
{
    BQ275XX_UNINIT = 0,              /**< Not initialized.                   */
    BQ275XX_STOP = 1,                /**< Stopped.                           */
    BQ275XX_READY = 3,               /**< Device ready.                      */
    BQ275XX_ACTIVE= 4,               /**< Device converting.                 */
} bq275xxstate_t;

/**
 * @brief   Type of a structure for battery_data.
 */
typedef struct
{
    float temperature;
    float voltage;
    float nom_available_capacity;
    float full_available_capacity;
    float remaining_capacity;
    float full_charge_capacity;
    float effective_current;
    float state_of_charge;
    float internal_temperature;
    float rem_capacity_unfiltered;
    float rem_capacity_filtered;
    float full_charge_capacity_unfiltered;
    float full_charge_capacity_filtered;
    float state_of_charge_unfiltered;
    float time_to_empty;
    float standby_time_to_empty;
} bq275xx_bat_data_s;

/**
 * @brief   Type of a structure representing an I2C driver.
 */
typedef struct BQ275XXDriver BQ275XXDriver;

/**
 * @brief   BQ275XX over I2C driver configuration structure.
 */

typedef struct
{
    /**
     * @brief I2C driver associated to this BQ275XX driver.
     */
    I2CDriver* i2cp;
    uint8_t i2c_address;
    systime_t i2c_timeout;
} BQ275XXConfig;

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a BQ275XX driver.
 */
struct BQ275XXDriver
{
    /**
    * @brief Driver state.
    */
    bq275xxstate_t state;
    /**
    * @brief Current configuration data.
    */
    const BQ275XXConfig* configp;
#if BQ275XX_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief Mutex protecting the device.
     */
    Mutex mutex;
#elif CH_USE_SEMAPHORES
    Semaphore semaphore;
#endif
#endif /* BQ275XX_USE_MUTUAL_EXCLUSION */
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
    void bq275xxInit(void);
    void bq275xxObjectInit(BQ275XXDriver* bq275xxp);
    void bq275xxStart(BQ275XXDriver* bq275xxp, const BQ275XXConfig* configp);
    void bq275xxStop(BQ275XXDriver* bq275xxp);
    void bq275xxAcquireBus(BQ275XXDriver* bq275xxp);
    void bq275xxReleaseBus(BQ275XXDriver* bq275xxp);
    bool bq275xxReadData(BQ275XXDriver* bq275xxp, bq275xx_bat_data_s* datap);
    bool bq275xxCommandBatInsert(BQ275XXDriver* bq275xxp);
    bool bq275xxCommandBatRemove(BQ275XXDriver* bq275xxp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_BQ275XX */

#endif /* _QBQ275XX_H_ */

/** @} */
