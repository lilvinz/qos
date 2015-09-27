/**
 * @file    qms5541.h
 * @brief   MS5541 driver header.
 *
 * @addtogroup MS5541
 * @{
 */

#ifndef _QMS5541_H_
#define _QMS5541_H_

#if HAL_USE_MS5541 || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    MS5541 configuration options
 * @{
 */

/**
 * @brief   Enables the @p ms5541AcquireBus() and @p ms5541ReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(MS5541_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define MS5541_USE_MUTUAL_EXCLUSION     FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !HAL_USE_SPI || !SPI_USE_WAIT
#error "MS5541 driver requires HAL_USE_SPI and SPI_USE_WAIT"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum
{
    MS5541_UNINIT = 0,              /**< Not initialized.                   */
    MS5541_STOP = 1,                /**< Stopped.                           */
    MS5541_READY = 3,               /**< Device ready.                      */
    MS5541_ACTIVE= 4,               /**< Device converting.                 */
} ms5541state_t;

/**
 * @brief   Type of a structure representing an SPI driver.
 */
typedef struct MS5541Driver MS5541Driver;

/**
 * @brief   MS5541 mclk configuration callback
 *
 * @param[in] ms5541p   pointer to the @p MS5541Driver object triggering the
 *                      callback
 */
typedef void (*ms5541callback_t)(MS5541Driver* ms5541p, bool enable);

/**
 * @brief   MS5541 over SPI driver configuration structure.
 */
typedef struct
{
    /**
     * @brief SPI driver associated to this MS5541 driver.
     */
    SPIDriver* spip;
    /**
     * @brief Pointers to SPI configurations
     */
    const SPIConfig* spiconfig_readp;
    const SPIConfig* spiconfig_writep;
    /**
     * @brief Callback to enable or disable conversion clock @p NULL.
     */
    ms5541callback_t mclk_cb;
} MS5541Config;

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a MS5541 driver.
 */
struct MS5541Driver
{
    /**
    * @brief Driver state.
    */
    ms5541state_t state;
    /**
    * @brief Current configuration data.
    */
    const MS5541Config* config;
#if MS5541_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
    /**
     * @brief mutex_t protecting the device.
     */
    mutex_t mutex;
#endif /* MS5541_USE_MUTUAL_EXCLUSION */
    /**
    * @brief Calibration data read from chip rom.
    */
    uint16_t calibration[6];
    /**
    * @brief Cached last raw readings
    */
    uint16_t last_d1;
    uint16_t last_d2;
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
    void ms5541Init(void);
    void ms5541ObjectInit(MS5541Driver* ms5541p);
    void ms5541Start(MS5541Driver* ms5541p, const MS5541Config* config);
    void ms5541Stop(MS5541Driver* ms5541p);
    void ms5541AcquireBus(MS5541Driver* ms5541p);
    void ms5541ReleaseBus(MS5541Driver* ms5541p);
    void ms5541TemperatureStart(MS5541Driver* ms5541p);
    int16_t ms5541TemperatureResult(MS5541Driver* ms5541p);
    void ms5541PressureStart(MS5541Driver* ms5541p);
    uint16_t ms5541PressureResult(MS5541Driver* ms5541p);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_MS5541 */

#endif /* _QMS5541_H_ */

/** @} */
