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
 * @brief   Delays insertions.
 * @details If enabled this options inserts delays into the flash waiting
 *          routines releasing some extra CPU time for the threads with
 *          lower priority, this may slow down the driver a bit however.
 *          This option is recommended also if the SPI driver does not
 *          use a DMA channel and heavily loads the CPU.
 */
#if !defined(MS5541_NICE_WAITING) || defined(__DOXYGEN__)
#define MS5541_NICE_WAITING             TRUE
#endif

/**
 * @brief   Enables the @p ms5541AcquireBus() and @p ms5541ReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(MS5541_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define MS5541_USE_MUTUAL_EXCLUSION     TRUE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !HAL_USE_SPI || !SPI_USE_WAIT
#error "MS5541 driver requires HAL_USE_SPI and SPI_USE_WAIT"
#endif

#if MS5541_USE_MUTUAL_EXCLUSION && !CH_USE_MUTEXES && !CH_USE_SEMAPHORES
#error "MS5541_USE_MUTUAL_EXCLUSION requires CH_USE_MUTEXES "
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
    MS5541_UNINIT = 0,              /**< Not initialized.                   */
    MS5541_STOP = 1,                /**< Stopped.                           */
    MS5541_READY = 3,               /**< Device ready.                      */
    MS5541_ACTIVE= 4,               /**< Device converting.                 */
} ms5541state_t;

/**
 * @brief   MS5541 over SPI driver configuration structure.
 */
typedef struct
{
    /**
    * @brief SPI driver associated to this MS5541 driver.
    */
    SPIDriver* spip;
#if HAL_USE_PWM
    /**
    * @brief PWM driver used to provide conversion clock.
    */
    PWMDriver* pwmp;
    /**
    * @brief PWM driver channel used to provide conversion clock.
    */
    pwmchannel_t pwm_channel;
#endif /* HAL_USE_PWM */
} MS5541Config;

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a MS5541 driver.
 */
typedef struct
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
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief Mutex protecting the device.
     */
    Mutex mutex;
#elif CH_USE_SEMAPHORES
    Semaphore semaphore;
#endif
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
} MS5541Driver;

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
    void ms5541Start(MS5541Driver* ms5541p,
            const MS5541Config* config);
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
