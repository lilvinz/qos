/**
 * @file    qms58xx.h
 * @brief   MS58XX driver header.
 *
 * @addtogroup MS58XX
 * @{
 */

#ifndef _QMS58XX_H_
#define _QMS58XX_H_

#if HAL_USE_MS58XX || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    MS58XX configuration options
 * @{
 */

/**
 * @brief   Enables the @p ms58xxAcquireBus() and @p ms58xxReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(MS58XX_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define MS58XX_USE_MUTUAL_EXCLUSION     FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !HAL_USE_I2C
#error "MS58XX driver requires HAL_USE_I2C"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Possible values for chip type.
 */
typedef enum
{
    MS58XX_TYPE_58XX = 0,
    MS58XX_TYPE_5837,
} ms58xxchiptype_t;

/**
 * @brief   Possible values for api oversampling argument.
 */
enum ms58xx_osr_e
{
    MS58XX_OSR_256 = 0,
    MS58XX_OSR_512 = 2,
    MS58XX_OSR_1024 = 4,
    MS58XX_OSR_2048 = 6,
    MS58XX_OSR_4096 = 8,
    MS58XX_OSR_8192 = 10,
};

/**
 * @brief   Driver state machine possible states.
 */
typedef enum
{
    MS58XX_UNINIT = 0,              /**< Not initialized.                   */
    MS58XX_STOP = 1,                /**< Stopped.                           */
    MS58XX_READY = 3,               /**< Device ready.                      */
    MS58XX_ACTIVE = 4,              /**< Device converting.                 */
} ms58xxstate_t;

/**
 * @brief   Type of a structure representing an I2C driver.
 */
typedef struct MS58XXDriver MS58XXDriver;

/**
 * @brief   MS58XX over I2C driver configuration structure.
 */
typedef struct
{
    /**
     * @brief I2C driver associated to this MS58XX driver.
     */
    I2CDriver* i2cp;
    uint8_t i2c_address;
    systime_t i2c_timeout;
    ms58xxchiptype_t chip_type;
} MS58XXConfig;

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a MS58XX driver.
 */
struct MS58XXDriver
{
    /**
    * @brief Driver state.
    */
    ms58xxstate_t state;
    /**
    * @brief Current configuration data.
    */
    const MS58XXConfig* configp;
#if MS58XX_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
    /**
     * @brief mutex_t protecting the device.
     */
    mutex_t mutex;
#endif /* MS58XX_USE_MUTUAL_EXCLUSION */
    /**
    * @brief Calibration data read from chip rom.
    */
    uint16_t calibration[8];
    /**
    * @brief Cached last raw readings
    */
    uint32_t last_d1;
    uint32_t last_d2;
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
    void ms58xxInit(void);
    void ms58xxObjectInit(MS58XXDriver* ms58xxp);
    void ms58xxStart(MS58XXDriver* ms58xxp, const MS58XXConfig* configp);
    void ms58xxStop(MS58XXDriver* ms58xxp);
    void ms58xxAcquireBus(MS58XXDriver* ms58xxp);
    void ms58xxReleaseBus(MS58XXDriver* ms58xxp);
    bool ms58xxTemperatureStart(MS58XXDriver* ms58xxp, enum ms58xx_osr_e osr);
    bool ms58xxTemperatureResult(MS58XXDriver* ms58xxp, float *resultp);
    bool ms58xxPressureStart(MS58XXDriver* ms58xxp, enum ms58xx_osr_e osr);
    bool ms58xxPressureResult(MS58XXDriver* ms58xxp, float *resultp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_MS58XX */

#endif /* _QMS58XX_H_ */

/** @} */
