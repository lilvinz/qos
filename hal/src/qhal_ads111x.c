/**
 * @file    qads111x.c
 * @brief   ADS111X driver code.
 *
 * @addtogroup ADS111X
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_ADS111X || defined(__DOXYGEN__)

#include "nelems.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

enum reg_addr_e
{
    REG_CONVERSION = 0x00,
    REG_CONFIG = 0x01,
    REG_TRESHOLD_MSL = 0x02,
    REG_TRESHOLD_MSB = 0x03,
};

static const uint16_t STANDBY_CONFIG =
        0b1 << 15 |     /* OS: no single shot conversion */
        0b000 << 12 |   /* MUX: AIN P = AIN0 and AIN N = AIN1 (default) */
        0b010 << 9 |    /* PGA: +-2.048V */
        0b1 << 8 |      /* MODE: power down single shot */
        0b100 << 5 |    /* DR: 128SPS */
        0b0 << 4 |      /* COMP_MODE: traditional comparator */
        0b0 << 3 |      /* COMP_POL: active low */
        0b0 << 2 |      /* COMP_LAT: non-latching comparator */
        0b11 << 0;      /* COMP_QUE: disable comparator */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   ADS111X driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void ads111xInit(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] ads111xp     pointer to the @p ADS111XDriver object
 *
 * @init
 */
void ads111xObjectInit(ADS111XDriver* ads111xp)
{
    ads111xp->state = ADS111X_STOP;
    ads111xp->configp = NULL;
#if ADS111X_USE_MUTUAL_EXCLUSION
#if CH_USE_MUTEXES
    chMtxInit(&ads111xp->mutex);
#else
    chSemInit(&ads111xp->semaphore, 1);
#endif
#endif /* ADS111X_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the FLASH peripheral.
 *
 * @param[in] ads111xp  pointer to the @p ADS111XDriver object
 * @param[in] config    pointer to the @p ADS111XConfig object.
 *
 * @api
 */
void ads111xStart(ADS111XDriver* ads111xp, const ADS111XConfig* configp)
{
    osalDbgCheck((ads111xp != NULL) && (configp != NULL));
    /* Verify device status. */
    osalDbgAssert((ads111xp->state == ADS111X_STOP) || (ads111xp->state == ADS111X_READY),
            "invalid state");

    ads111xp->configp = configp;

    /* Enter device standby mode. */
    const uint8_t txbuf[] = {
            REG_CONFIG,
            (const uint8_t)(STANDBY_CONFIG >> 8),
            (const uint8_t)(STANDBY_CONFIG >> 0),
    };

    msg_t result;
    result = i2cMasterTransmitTimeout(ads111xp->configp->i2cp,
            ads111xp->configp->i2c_address >> 1,
            txbuf,
            sizeof(txbuf),
            NULL,
            0,
            ads111xp->configp->i2c_timeout);

    ads111xp->state = ADS111X_READY;
}

/**
 * @brief   Disables the FLASH peripheral.
 *
 * @param[in] ads111xp  pointer to the @p ADS111XDriver object
 *
 * @api
 */
void ads111xStop(ADS111XDriver* ads111xp)
{
    osalDbgCheck(ads111xp != NULL);
    /* Verify device status. */
    osalDbgAssert((ads111xp->state == ADS111X_STOP) || (ads111xp->state == ADS111X_READY),
            "invalid state");

    /* Enter device standby mode. */
    const uint8_t txbuf[] = {
            REG_CONFIG,
            (const uint8_t)(STANDBY_CONFIG >> 8),
            (const uint8_t)(STANDBY_CONFIG >> 0),
    };

    msg_t result;
    result = i2cMasterTransmitTimeout(ads111xp->configp->i2cp,
            ads111xp->configp->i2c_address >> 1,
            txbuf,
            sizeof(txbuf),
            NULL,
            0,
            ads111xp->configp->i2c_timeout);

    ads111xp->state = ADS111X_STOP;
}

/**
 * @brief   Gains exclusive access to the flash device.
 * @details This function tries to gain ownership to the flash device, if the
 *          device is already being used then the invoking thread is queued.
 * @pre     In order to use this function the option
 *          @p ADS111X_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] ads111xp  pointer to the @p ADS111XDriver object
 *
 * @api
 */
void ads111xAcquireBus(ADS111XDriver* ads111xp)
{
    osalDbgCheck(ads111xp != NULL);

#if ADS111X_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    chMtxLock(&ads111xp->mutex);
#elif CH_USE_SEMAPHORES
    chSemWait(&ads111xp->semaphore);
#endif

#if I2C_USE_MUTUAL_EXCLUSION
    /* Acquire the underlying device as well. */
    i2cAcquireBus(ads111xp->config->i2cp);
#endif /* I2C_USE_MUTUAL_EXCLUSION */
#endif /* ADS111X_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the flash device.
 * @pre     In order to use this function the option
 *          @p ADS111X_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] ads111xp  pointer to the @p ADS111XDriver object
 *
 * @api
 */
void ads111xReleaseBus(ADS111XDriver* ads111xp)
{
    osalDbgCheck(ads111xp != NULL);

#if ADS111X_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    (void)ads111xp;
    chMtxUnlock();
#elif CH_USE_SEMAPHORES
    chSemSignal(&ads111xp->semaphore);
#endif

#if I2C_USE_MUTUAL_EXCLUSION
    /* Release the underlying device as well. */
    i2cReleaseBus(ads111xp->config->i2cp);
#endif /* I2C_USE_MUTUAL_EXCLUSION */
#endif /* ADS111X_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Starts a data conversion.
 *
 * @param[in] ads111xp  pointer to the @p ADS111XDriver object
 * @param[in] config    device configuration for conversion
 *
 * @return              The operation status.
 * @retval OSAL_SUCCESS the operation succeeded.
 * @retval OSAL_FAILED  the operation failed.
 *
 * @api
 */
bool ads111xStartConversion(ADS111XDriver* ads111xp, uint16_t config)
{
    osalDbgCheck(ads111xp != NULL);
    /* Verify device status. */
    osalDbgAssert(ads111xp->state == ADS111X_READY,
            "invalid state");

    ads111xp->state = ADS111X_ACTIVE;

    /* Setup device configuration register. */
    const uint8_t txbuf[] = {
            REG_CONFIG,
            (const uint8_t)(config >> 8),
            (const uint8_t)(config >> 0),
    };

    msg_t result;
    result = i2cMasterTransmitTimeout(ads111xp->configp->i2cp,
            ads111xp->configp->i2c_address >> 1,
            txbuf,
            sizeof(txbuf),
            NULL,
            0,
            ads111xp->configp->i2c_timeout);

    if (result != MSG_OK)
    {
        ads111xp->state = ADS111X_STOP;
        return OSAL_FAILED;
    }

    ads111xp->state = ADS111X_READY;

    return OSAL_SUCCESS;
}

/**
 * @brief   Reads out conversion result.
 *
 * @param[in] ads111xp  pointer to the @p ADS111XDriver object
 * @param[out] datap    pointer to @p fxps8700_data_s structure
 *
 * @return              The operation status.
 * @retval OSAL_SUCCESS the operation succeeded.
 * @retval OSAL_FAILED  the operation failed.
 *
 * @api
 */
bool ads111xReadResult(ADS111XDriver* ads111xp, int16_t* datap)
{
    osalDbgCheck(ads111xp != NULL && datap != NULL);
    /* Verify device status. */
    osalDbgAssert(ads111xp->state == ADS111X_READY,
            "invalid state");

    ads111xp->state = ADS111X_ACTIVE;

    /* Poll until conversion has finished. */
    {
        const uint8_t txbuf[] = { REG_CONFIG, };
        uint8_t rxbuf[] = { 0x00, 0x00, };

        while ((rxbuf[0] & 0x80) == 0x00)
        {
            msg_t result;
            result = i2cMasterTransmitTimeout(ads111xp->configp->i2cp,
                    ads111xp->configp->i2c_address >> 1,
                    txbuf,
                    sizeof(txbuf),
                    rxbuf,
                    sizeof(rxbuf),
                    ads111xp->configp->i2c_timeout);

            if (result != MSG_OK)
            {
                ads111xp->state = ADS111X_STOP;
                return OSAL_FAILED;
            }

#if ADS111X_NICE_WAITING
            if ((rxbuf[0] & 0x80) == 0x00)
                /* Trying to be nice with the other threads.*/
                osalThreadSleepMilliseconds(1);
#endif /* ADS111X_NICE_WAITING */
        }
    }

    /* Read conversion result. */
    {
        const uint8_t txbuf[] = { REG_CONVERSION, };
        uint8_t rxbuf[] = { 0x00, 0x00, };

        msg_t result;
        result = i2cMasterTransmitTimeout(ads111xp->configp->i2cp,
                ads111xp->configp->i2c_address >> 1,
                txbuf,
                sizeof(txbuf),
                rxbuf,
                sizeof(rxbuf),
                ads111xp->configp->i2c_timeout);

        if (result != MSG_OK)
        {
            ads111xp->state = ADS111X_STOP;
            return OSAL_FAILED;
        }

        uint16_t temp = (rxbuf[0] << 8) | (rxbuf[1] << 0);
        *datap = *((int16_t*)&temp);
    }

    ads111xp->state = ADS111X_READY;

    return OSAL_SUCCESS;
}

#endif /* HAL_USE_ADS111X */

/** @} */
