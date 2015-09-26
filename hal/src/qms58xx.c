/**
 * @file    qms58xx.c
 * @brief   MS58XX driver code.
 *
 * @addtogroup MS58XX
 * @{
 */

#include "qhal.h"

#if HAL_USE_MS58XX || defined(__DOXYGEN__)

#include <string.h>
/*
 * @todo    - add sanity check for calibration values
 *          - add conversion error detection and recovery
 */

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

enum command_e
{
    MS58XX_COMMAND_RESET = 0x1e,
    MS58XX_COMMAND_ACQUIRE_D1 = 0x40,
    MS58XX_COMMAND_ACQUIRE_D2 = 0x50,
    MS58XX_COMMAND_READ_ADC = 0x00,
    MS58XX_COMMAND_READ_PROM = 0xa0,
};

enum
{
    MS58XX_CAL_0_RESERVED = 0,
    MS58XX_CAL_1_SENST1 = 1,
    MS58XX_CAL_2_OFFT1 = 2,
    MS58XX_CAL_3_TCS = 3,
    MS58XX_CAL_4_TCO = 4,
    MS58XX_CAL_5_TREF = 5,
    MS58XX_CAL_6_TEMPSENS = 6,
    MS58XX_CAL_7_CRC = 7,
};

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
 * @brief   MS58XX driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void ms58xxInit(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] ms58xxp  pointer to the @p MS58XXDriver object
 *
 * @init
 */
void ms58xxObjectInit(MS58XXDriver* ms58xxp)
{
    ms58xxp->state = MS58XX_STOP;
    ms58xxp->configp = NULL;
    ms58xxp->last_d1 = 0;
    ms58xxp->last_d2 = 0;
    memset(ms58xxp->calibration, 0, sizeof(ms58xxp->calibration));
#if MS58XX_USE_MUTUAL_EXCLUSION
#if CH_CFG_USE_MUTEXES
    chMtxObjectInit(&ms58xxp->mutex);
#else
    chSemObjectInit(&ms58xxp->semaphore, 1);
#endif
#endif /* MS58XX_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the FLASH peripheral.
 *
 * @param[in] ms58xxp   pointer to the @p MS58XXDriver object
 * @param[in] config    pointer to the @p MS58XXConfig object.
 *
 * @api
 */
void ms58xxStart(MS58XXDriver* ms58xxp, const MS58XXConfig* configp)
{
    chDbgCheck((ms58xxp != NULL) && (configp != NULL));
    /* Verify device status. */
    chDbgAssert((ms58xxp->state == MS58XX_STOP) || (ms58xxp->state == MS58XX_READY),
            "invalid state");

    ms58xxp->configp = configp;

    /* Reset device. */
    {
        const uint8_t txbuf[] = { MS58XX_COMMAND_RESET };

        msg_t result;
        result = i2cMasterTransmitTimeout(ms58xxp->configp->i2cp,
                ms58xxp->configp->i2c_address >> 1,
                txbuf,
                sizeof(txbuf),
                NULL,
                0,
                ms58xxp->configp->i2c_timeout);

        if (result != MSG_OK)
        {
            ms58xxp->state = MS58XX_STOP;
            return;
        }

        /* Sensor requires 2.8 ms to restart. */
        chThdSleepMilliseconds(3);
    }

    /* Read calibration data. */
    for (size_t i = 0; i < 8; ++i)
    {
        const uint8_t txbuf[] = { MS58XX_COMMAND_READ_PROM + (i << 1) };
        uint8_t rxbuf[] = { 0x00, 0x00 };

        msg_t result;
        result = i2cMasterTransmitTimeout(ms58xxp->configp->i2cp,
                ms58xxp->configp->i2c_address >> 1,
                txbuf,
                sizeof(txbuf),
                rxbuf,
                sizeof(rxbuf),
                ms58xxp->configp->i2c_timeout);

        if (result != MSG_OK)
        {
            ms58xxp->state = MS58XX_STOP;
            return;
        }

        ms58xxp->calibration[i] = (rxbuf[0] << 8) | rxbuf[1];
    }

    /* Verify crc. */
    {
        uint16_t n_rem = 0x00;
        uint16_t crc_read = ms58xxp->calibration[MS58XX_CAL_7_CRC];
        ms58xxp->calibration[MS58XX_CAL_7_CRC] =
                ms58xxp->calibration[MS58XX_CAL_7_CRC] & 0xff00;

        for (size_t cnt = 0; cnt < 16; ++cnt)
        {
            if (cnt % 2 == 1)
                n_rem ^= ms58xxp->calibration[cnt >> 1] & 0x00ff;
            else
                n_rem ^= ms58xxp->calibration[cnt >> 1] >> 8;

            for (uint8_t n_bit = 8; n_bit > 0; --n_bit)
            {
                if (n_rem & 0x8000)
                {
                    n_rem = (n_rem << 1) ^ 0x3000;
                }
                else
                {
                    n_rem = n_rem << 1;
                }
            }
        }
        n_rem = (n_rem >> 12) & 0x000f;
        ms58xxp->calibration[MS58XX_CAL_7_CRC] = crc_read;

        if (n_rem != (ms58xxp->calibration[MS58XX_CAL_7_CRC] & 0x0f))
        {
            ms58xxp->state = MS58XX_STOP;
            return;
        }
    }

    ms58xxp->state = MS58XX_READY;
}

/**
 * @brief   Disables the FLASH peripheral.
 *
 * @param[in] ms58xxp      pointer to the @p MS58XXDriver object
 *
 * @api
 */
void ms58xxStop(MS58XXDriver* ms58xxp)
{
    chDbgCheck(ms58xxp != NULL);
    /* Verify device status. */
    chDbgAssert((ms58xxp->state == MS58XX_STOP) || (ms58xxp->state == MS58XX_READY),
            "invalid state");

    ms58xxp->state = MS58XX_STOP;
}

/**
 * @brief   Gains exclusive access to the flash device.
 * @details This function tries to gain ownership to the flash device, if the
 *          device is already being used then the invoking thread is queued.
 * @pre     In order to use this function the option
 *          @p MS58XX_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] ms58xxp      pointer to the @p MS58XXDriver object
 *
 * @api
 */
void ms58xxAcquireBus(MS58XXDriver* ms58xxp)
{
    chDbgCheck(ms58xxp != NULL);

#if MS58XX_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES
    chMtxLock(&ms58xxp->mutex);
#elif CH_CFG_USE_SEMAPHORES
    chSemWait(&ms58xxp->semaphore);
#endif

#if I2C_USE_MUTUAL_EXCLUSION
    /* Acquire the underlying device as well. */
    i2cAcquireBus(ms58xxp->config->i2cp);
#endif /* I2C_USE_MUTUAL_EXCLUSION */
#endif /* MS58XX_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the flash device.
 * @pre     In order to use this function the option
 *          @p MS58XX_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] ms58xxp   pointer to the @p MS58XXDriver object
 *
 * @api
 */
void ms58xxReleaseBus(MS58XXDriver* ms58xxp)
{
    chDbgCheck(ms58xxp != NULL);

#if MS58XX_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES
    (void)ms58xxp;
    chMtxUnlock(&ms58xxp->mutex);
#elif CH_CFG_USE_SEMAPHORES
    chSemSignal(&ms58xxp->semaphore);
#endif

#if I2C_USE_MUTUAL_EXCLUSION
    /* Release the underlying device as well. */
    i2cReleaseBus(ms58xxp->config->i2cp);
#endif /* I2C_USE_MUTUAL_EXCLUSION */
#endif /* MS58XX_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Starts a temperature conversion.
 *
 * @param[in] ms58xxp   pointer to the @p MS58XXDriver object
 * @param[in] osr       oversampling
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool ms58xxTemperatureStart(MS58XXDriver* ms58xxp, enum ms58xx_osr_e osr)
{
    chDbgCheck(ms58xxp != NULL);
    /* Verify device status. */
    chDbgAssert(ms58xxp->state == MS58XX_READY, "invalid state");

    ms58xxp->state = MS58XX_ACTIVE;

    /* Initiate temperature conversion. */
    const uint8_t txbuf[] = { MS58XX_COMMAND_ACQUIRE_D2 + osr };

    msg_t result;
    result = i2cMasterTransmitTimeout(ms58xxp->configp->i2cp,
            ms58xxp->configp->i2c_address >> 1,
            txbuf,
            sizeof(txbuf),
            NULL,
            0,
            ms58xxp->configp->i2c_timeout);

    if (result != MSG_OK)
        return HAL_FAILED;

    return HAL_SUCCESS;
}

/**
 * @brief   Finishes a temperature conversion.
 *
 * @param[in] ms58xxp   pointer to the @p MS58XXDriver object
 * @param[out] resultp  pointer to temperature result in degree C
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool ms58xxTemperatureResult(MS58XXDriver* ms58xxp, float *resultp)
{
    chDbgCheck(ms58xxp != NULL);
    /* Verify device status. */
    chDbgAssert(ms58xxp->state == MS58XX_ACTIVE, "invalid state");

    /* Read result from chip. */
    {
        const uint8_t txbuf[] = { MS58XX_COMMAND_READ_ADC };
        uint8_t rxbuf[] = { 0x00, 0x00, 0x00 };

        msg_t result;
        result = i2cMasterTransmitTimeout(ms58xxp->configp->i2cp,
                ms58xxp->configp->i2c_address >> 1,
                txbuf,
                sizeof(txbuf),
                rxbuf,
                sizeof(rxbuf),
                ms58xxp->configp->i2c_timeout);

        if (result != MSG_OK)
        {
            /* Reset driver state. */
            ms58xxp->state = MS58XX_READY;

            return HAL_FAILED;
        }

        ms58xxp->last_d2 = (rxbuf[0] << 16) | (rxbuf[1] << 8) | (rxbuf[2] << 0);
    }

    /* Reset driver state. */
    ms58xxp->state = MS58XX_READY;

    /* dT is the difference between current adc value and
     * adc value at factory calibration (20 degree celsius). */
    int32_t dT = (int32_t)ms58xxp->last_d2 -
            ((int32_t)ms58xxp->calibration[MS58XX_CAL_5_TREF] << 8);

    /* TEMP is the temperature in degree celsius * 100. */
    int32_t TEMP = 2000 +
            (((int64_t)dT * ms58xxp->calibration[MS58XX_CAL_6_TEMPSENS]) >> 23);

    /* Apply second order temperature compensation. */
    if (TEMP < 2000)
    {
        int32_t T2 = ((int64_t)3 * dT * dT) >> 33;
        TEMP -= T2;
    }
    else
    {
        int32_t T2 = ((int64_t)7 * dT * dT) >> 37;
        TEMP -= T2;
    }

    if (resultp != NULL)
        *resultp = TEMP / 100.0f;

    return HAL_SUCCESS;
}

/**
 * @brief   Starts a pressure conversion.
 *
 * @param[in] ms58xxp   pointer to the @p MS58XXDriver object
 * @param[in] osr       oversampling
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool ms58xxPressureStart(MS58XXDriver* ms58xxp, enum ms58xx_osr_e osr)
{
    chDbgCheck(ms58xxp != NULL);
    /* Verify device status. */
    chDbgAssert(ms58xxp->state == MS58XX_READY, "invalid state");

    ms58xxp->state = MS58XX_ACTIVE;

    /* Initiate pressure conversion. */
    const uint8_t txbuf[] = { MS58XX_COMMAND_ACQUIRE_D1 + osr };

    msg_t result;
    result = i2cMasterTransmitTimeout(ms58xxp->configp->i2cp,
            ms58xxp->configp->i2c_address >> 1,
            txbuf,
            sizeof(txbuf),
            NULL,
            0,
            ms58xxp->configp->i2c_timeout);

    if (result != MSG_OK)
        return HAL_FAILED;

    return HAL_SUCCESS;
}

/**
 * @brief   Finishes a pressure conversion.
 *
 * @param[in] ms58xxp   pointer to the @p MS58XXDriver object
 * @param[out] resultp  pointer to pressure result in bar
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool ms58xxPressureResult(MS58XXDriver* ms58xxp, float *resultp)
{
    chDbgCheck(ms58xxp != NULL);
    /* Verify device status. */
    chDbgAssert(ms58xxp->state == MS58XX_ACTIVE, "invalid state");

    /* Read result from chip. */
    {
        const uint8_t txbuf[] = { MS58XX_COMMAND_READ_ADC };
        uint8_t rxbuf[] = { 0x00, 0x00, 0x00 };

        msg_t result;
        result = i2cMasterTransmitTimeout(ms58xxp->configp->i2cp,
                ms58xxp->configp->i2c_address >> 1,
                txbuf,
                sizeof(txbuf),
                rxbuf,
                sizeof(rxbuf),
                ms58xxp->configp->i2c_timeout);

        if (result != MSG_OK)
        {
            /* Reset driver state. */
            ms58xxp->state = MS58XX_READY;

            return HAL_FAILED;
        }

        ms58xxp->last_d1 = (rxbuf[0] << 16) | (rxbuf[1] << 8) | (rxbuf[2] << 0);
    }

    /* Reset driver state. */
    ms58xxp->state = MS58XX_READY;

    /* dT is the difference between current adc value and
     * adc value at factory calibration (20 degree celsius). */
    int32_t dT = (int32_t)ms58xxp->last_d2 -
            ((int32_t)ms58xxp->calibration[MS58XX_CAL_5_TREF] << 8);

    /* TEMP is the temperature in degree celsius * 100. */
    int32_t TEMP = 2000 +
            (((int64_t)dT * ms58xxp->calibration[MS58XX_CAL_6_TEMPSENS]) >> 23);

    /* OFF is pressure adc offset at the current temperature. */
    int64_t OFF = ((int64_t)ms58xxp->calibration[MS58XX_CAL_2_OFFT1] << 16) +
            (((int64_t)ms58xxp->calibration[MS58XX_CAL_4_TCO] * dT) >> 7);

    /* SENS is pressure adc sensitivity at the current temperature. */
    int64_t SENS = ((int64_t)ms58xxp->calibration[MS58XX_CAL_1_SENST1] << 15) +
            (((int64_t)ms58xxp->calibration[MS58XX_CAL_3_TCS] * dT) >> 8);

    /* Apply second order temperature compensation. */
    if (TEMP < 2000)
    {
        int64_t OFF2 = (3 * (TEMP - 2000) * (TEMP - 2000)) >> 1;
        int64_t SENS2 = (5 * (TEMP - 2000) * (TEMP - 2000)) >> 3;

        if (TEMP < -1500)
        {
            OFF2 += 7 * (TEMP + 1500) * (TEMP + 1500);
            SENS2 += 4 * (TEMP + 1500) * (TEMP + 1500);
        }

        OFF -= OFF2;
        SENS -= SENS2;
    }
    else
    {
        int64_t OFF2 = (1 * (TEMP - 2000) * (TEMP - 2000)) >> 4;

        OFF -= OFF2;
    }

    /* P is the pressure in bar * 10000. */
    int32_t P = (((ms58xxp->last_d1 * SENS) >> 21) - OFF) >> 13;

    if (resultp != NULL)
        *resultp = P / 10000.0f;

    return HAL_SUCCESS;
}

#endif /* HAL_USE_MS58XX */

/** @} */
