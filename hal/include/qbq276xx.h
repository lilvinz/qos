/**
 * @file    qbq276xx.h
 * @brief   BQ276XX driver header.
 *
 * @addtogroup BQ276XX
 * @{
 */

#ifndef _QBQ276XX_H_
#define _QBQ276XX_H_

#if HAL_USE_BQ276XX || defined(__DOXYGEN__)

#include <stdint.h>

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    BQ276XX configuration options
 * @{
 */

/**
 * @brief   Delays insertions.
 * @details If enabled this options inserts unpolled delays into the polling
 *          routines releasing some extra CPU time for the threads with
 *          lower priority, this may slow down the driver a bit however.
 */
#if !defined(BQ276XX_NICE_WAITING) || defined(__DOXYGEN__)
#define BQ276XX_NICE_WAITING            TRUE
#endif

/**
 * @brief   Enables the @p bq276xxAcquireBus() and @p bq276xxReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(BQ276XX_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define BQ276XX_USE_MUTUAL_EXCLUSION     FALSE
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !HAL_USE_I2C
#error "BQ276XX driver requires HAL_USE_I2C"
#endif

#if BQ276XX_USE_MUTUAL_EXCLUSION && !CH_USE_MUTEXES && !CH_USE_SEMAPHORES
#error "BQ276XX_USE_MUTUAL_EXCLUSION requires CH_USE_MUTEXES "
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
    BQ276XX_UNINIT = 0,              /**< Not initialized.                   */
    BQ276XX_STOP = 1,                /**< Stopped.                           */
    BQ276XX_READY = 3,               /**< Device ready.                      */
    BQ276XX_ACTIVE= 4,               /**< Device converting.                 */
} bq276xxstate_t;

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
    float average_power;
    float state_of_charge;
    float internal_temperature;
    float rem_capacity_unfiltered;
    float rem_capacity_filtered;
    float full_charge_capacity_unfiltered;
    float full_charge_capacity_filtered;
    float state_of_charge_unfiltered;
} bq276xx_bat_data_s;

/**
 * @brief   Type of a structure representing an I2C driver.
 */
typedef struct BQ276XXDriver BQ276XXDriver;

/**
 * @brief   BQ276XX over I2C driver configuration structure.
 */
struct dm_reg_setup_s
{
    uint8_t subclass;
    uint8_t offset;
    uint8_t len;
    uint32_t data;
};

typedef struct
{
    /**
     * @brief I2C driver associated to this BQ276XX driver.
     */
    I2CDriver* i2cp;
    uint8_t i2c_address;
    systime_t i2c_timeout;
    /**
     * @brief Gauge configuration.
     */
    const struct dm_reg_setup_s* dm_reg_setup;
    size_t dm_reg_count;
} BQ276XXConfig;

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a BQ276XX driver.
 */
struct BQ276XXDriver
{
    /**
    * @brief Driver state.
    */
    bq276xxstate_t state;
    /**
    * @brief Current configuration data.
    */
    const BQ276XXConfig* configp;
#if BQ276XX_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief Mutex protecting the device.
     */
    Mutex mutex;
#elif CH_USE_SEMAPHORES
    Semaphore semaphore;
#endif
#endif /* BQ276XX_USE_MUTUAL_EXCLUSION */
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
    void bq276xxInit(void);
    void bq276xxObjectInit(BQ276XXDriver* bq276xxp);
    void bq276xxStart(BQ276XXDriver* bq276xxp, const BQ276XXConfig* configp);
    void bq276xxStop(BQ276XXDriver* bq276xxp);
    void bq276xxAcquireBus(BQ276XXDriver* bq276xxp);
    void bq276xxReleaseBus(BQ276XXDriver* bq276xxp);
    bool_t bq276xxReadData(BQ276XXDriver* bq276xxp, bq276xx_bat_data_s* datap);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_BQ276XX */

#endif /* _QBQ276XX_H_ */

/** @} */
