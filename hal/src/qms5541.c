/**
 * @file    qms5541.c
 * @brief   MS5541 driver code.
 *
 * @addtogroup MS5541
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_MS5541 || defined(__DOXYGEN__)

#include <string.h>

/*
 * @todo    - add sanity check for calibration values
 *          - add conversion error detection and recovery
 */

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

static const uint8_t* command_table[] =
{
    (uint8_t*)"\x15\x55\x40",
    (uint8_t*)"\x0f\x40",
    (uint8_t*)"\x0f\x20",
    (uint8_t*)"\x1d\x50",
    (uint8_t*)"\x1d\x60",
    (uint8_t*)"\x1d\x90",
    (uint8_t*)"\x1d\xa0",
};

enum command_e
{
    MS5541_COMMAND_RESET = 0,
    MS5451_COMMAND_ACQUIRE_D1,
    MS5451_COMMAND_ACQUIRE_D2,
    MS5451_COMMAND_READ_CALIB_1,
    MS5451_COMMAND_READ_CALIB_2,
    MS5451_COMMAND_READ_CALIB_3,
    MS5451_COMMAND_READ_CALIB_4,
};

enum
{
    MS5541_CAL_1 = 0,
    MS5541_CAL_SENST1 = 0,
    MS5541_CAL_2 = 1,
    MS5541_CAL_OFFT1 = 1,
    MS5541_CAL_3 = 2,
    MS5541_CAL_TCS = 2,
    MS5541_CAL_4 = 3,
    MS5541_CAL_TCO = 3,
    MS5541_CAL_5 = 4,
    MS5541_CAL_TREF = 4,
    MS5541_CAL_6 = 5,
    MS5541_CAL_TEMPSENS = 5,
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

static void ms5541_write(MS5541Driver* ms5541p, enum command_e command)
{
    const uint8_t *data = command_table[command];

    /* Reconfigure spi driver. */
    spiStart(ms5541p->config->spip, ms5541p->config->spiconfig_writep);

    if (command == MS5541_COMMAND_RESET)
        spiSend(ms5541p->config->spip, 3, data);
    else
        spiSend(ms5541p->config->spip, 2, data);
}

static uint16_t ms5541_read(MS5541Driver* ms5541p)
{
    uint8_t temp[2];

    /* Reconfigure spi driver. */
    spiStart(ms5541p->config->spip, ms5541p->config->spiconfig_readp);

    spiReceive(ms5541p->config->spip, 2, temp);

    return (temp[0] << 8) | (temp[1] << 0);
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   MS5541 driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void ms5541Init(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] ms5541p  pointer to the @p MS5541Driver object
 *
 * @init
 */
void ms5541ObjectInit(MS5541Driver* ms5541p)
{
    ms5541p->state = MS5541_STOP;
    ms5541p->config = NULL;
    ms5541p->last_d1 = 0;
    ms5541p->last_d2 = 0;
    memset(ms5541p->calibration, 0, sizeof(ms5541p->calibration));
#if MS5541_USE_MUTUAL_EXCLUSION
#if CH_USE_MUTEXES
    chMtxInit(&ms5541p->mutex);
#else
    chSemInit(&ms5541p->semaphore, 1);
#endif
#endif /* MS5541_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the FLASH peripheral.
 *
 * @param[in] ms5541p   pointer to the @p MS5541Driver object
 * @param[in] config    pointer to the @p MS5541Config object.
 *
 * @api
 */
void ms5541Start(MS5541Driver* ms5541p, const MS5541Config* config)
{
    chDbgCheck((ms5541p != NULL) && (config != NULL), "ms5541Start");
    /* Verify device status. */
    chDbgAssert((ms5541p->state == MS5541_STOP) || (ms5541p->state == MS5541_READY),
            "ms5541Start(), #1", "invalid state");

    ms5541p->config = config;

    /* Read calibration data. */
    uint16_t temp;
    ms5541_write(ms5541p, MS5541_COMMAND_RESET);
    ms5541_write(ms5541p, MS5451_COMMAND_READ_CALIB_1);
    temp = ms5541_read(ms5541p);
    ms5541p->calibration[MS5541_CAL_1] = (temp >> 3) & 0x1fff;
    ms5541p->calibration[MS5541_CAL_2] = (temp << 10) & 0x1c00;

    ms5541_write(ms5541p, MS5451_COMMAND_READ_CALIB_2);
    temp = ms5541_read(ms5541p);
    ms5541p->calibration[MS5541_CAL_2] |= (temp >> 6) & 0x03ff;
    ms5541p->calibration[MS5541_CAL_5] = (temp << 6) & 0x0fc0;

    ms5541_write(ms5541p, MS5451_COMMAND_READ_CALIB_3);
    temp = ms5541_read(ms5541p);
    ms5541p->calibration[MS5541_CAL_5] |= (temp >> 0) & 0x003f;
    ms5541p->calibration[MS5541_CAL_3] = (temp >> 6) & 0x03ff;

    ms5541_write(ms5541p, MS5451_COMMAND_READ_CALIB_4);
    temp = ms5541_read(ms5541p);
    ms5541p->calibration[MS5541_CAL_4] = (temp >> 7) & 0x01ff;
    ms5541p->calibration[MS5541_CAL_6] = (temp >> 0) & 0x007f;

    ms5541p->state = MS5541_READY;
}

/**
 * @brief   Disables the FLASH peripheral.
 *
 * @param[in] ms5541p      pointer to the @p MS5541Driver object
 *
 * @api
 */
void ms5541Stop(MS5541Driver* ms5541p)
{
    chDbgCheck(ms5541p != NULL, "ms5541Stop");
    /* Verify device status. */
    chDbgAssert((ms5541p->state == MS5541_STOP) || (ms5541p->state == MS5541_READY),
            "ms5541Stop(), #1", "invalid state");

    ms5541p->state = MS5541_STOP;
}

/**
 * @brief   Gains exclusive access to the flash device.
 * @details This function tries to gain ownership to the flash device, if the
 *          device is already being used then the invoking thread is queued.
 * @pre     In order to use this function the option
 *          @p MS5541_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] ms5541p      pointer to the @p MS5541Driver object
 *
 * @api
 */
void ms5541AcquireBus(MS5541Driver* ms5541p)
{
    chDbgCheck(ms5541p != NULL, "ms5541AcquireBus");

#if MS5541_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    chMtxLock(&ms5541p->mutex);
#elif CH_USE_SEMAPHORES
    chSemWait(&ms5541p->semaphore);
#endif

#if SPI_USE_MUTUAL_EXCLUSION
    /* Acquire the underlying device as well. */
    spiAcquireBus(ms5541p->config->spip);
#endif /* SPI_USE_MUTUAL_EXCLUSION */
#endif /* MS5541_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the flash device.
 * @pre     In order to use this function the option
 *          @p MS5541_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] ms5541p   pointer to the @p MS5541Driver object
 *
 * @api
 */
void ms5541ReleaseBus(MS5541Driver* ms5541p)
{
    chDbgCheck(ms5541p != NULL, "ms5541ReleaseBus");

#if MS5541_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    (void)ms5541p;
    chMtxUnlock();
#elif CH_USE_SEMAPHORES
    chSemSignal(&ms5541p->semaphore);
#endif

#if SPI_USE_MUTUAL_EXCLUSION
    /* Release the underlying device as well. */
    spiReleaseBus(ms5541p->config->spip);
#endif /* SPI_USE_MUTUAL_EXCLUSION */
#endif /* MS5541_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Starts a temperature conversion.
 *
 * @param[in] ms5541p   pointer to the @p MS5541Driver object
 *
 * @api
 */
void ms5541TemperatureStart(MS5541Driver* ms5541p)
{
    chDbgCheck(ms5541p != NULL, "ms5541TemperatureStart");
    /* Verify device status. */
    chDbgAssert(ms5541p->state == MS5541_READY,
            "ms5541TemperatureStart(), #1", "invalid stateMS5541Driver");

    ms5541p->state = MS5541_ACTIVE;

    /* Enable clock output if configured. */
    if (ms5541p->config->mclk_cb != NULL)
        ms5541p->config->mclk_cb(ms5541p, TRUE);

    /* Initiate temperature conversion. */
    ms5541_write(ms5541p, MS5541_COMMAND_RESET);
    ms5541_write(ms5541p, MS5451_COMMAND_ACQUIRE_D2);
}

/**
 * @brief   Finishes a temperature conversion.
 *
 * @param[in] ms5541p   pointer to the @p MS5541Driver object
 * @return              Temperature result in 1/10 degree C
 *
 * @api
 */
int16_t ms5541TemperatureResult(MS5541Driver* ms5541p)
{
    chDbgCheck(ms5541p != NULL, "ms5541TemperatureResult");
    /* Verify device status. */
    chDbgAssert(ms5541p->state == MS5541_ACTIVE,
            "ms5541TemperatureResult(), #1", "invalid state");

    /* Disable clock output if configured. */
    if (ms5541p->config->mclk_cb != NULL)
        ms5541p->config->mclk_cb(ms5541p, FALSE);

    /* Read result from chip. */
    ms5541p->last_d2 = ms5541_read(ms5541p);

    /* Reset driver state. */
    ms5541p->state = MS5541_READY;

    /* Calculate result. */
    int32_t UT1 = (int32_t)ms5541p->calibration[MS5541_CAL_TREF] * 8 + 10000;
    int32_t dT = (int32_t)ms5541p->last_d2 - UT1;

    int32_t dT2;
    if (dT < 0)
        dT2 = dT - (dT / 128 * dT / 128) / 2;
    else
        dT2 = dT - (dT / 128 * dT / 128) / 8;

    int32_t TEMP = 200 + dT2 *
            (ms5541p->calibration[MS5541_CAL_TEMPSENS] + 100) / 2048;

    return TEMP;
}

/**
 * @brief   Starts a pressure conversion.
 *
 * @param[in] ms5541p   pointer to the @p MS5541Driver object
 *
 * @api
 */
void ms5541PressureStart(MS5541Driver* ms5541p)
{
    chDbgCheck(ms5541p != NULL, "ms5541PressureStart");
    /* Verify device status. */
    chDbgAssert(ms5541p->state == MS5541_READY,
            "ms5541PressureStart(), #1", "invalid state");

    ms5541p->state = MS5541_ACTIVE;

    /* Enable clock output if configured. */
    if (ms5541p->config->mclk_cb != NULL)
        ms5541p->config->mclk_cb(ms5541p, TRUE);

    /* Initiate pressure conversion. */
    ms5541_write(ms5541p, MS5541_COMMAND_RESET);
    ms5541_write(ms5541p, MS5451_COMMAND_ACQUIRE_D1);
}

/**
 * @brief   Finishes a pressure conversion.
 *
 * @param[in] ms5541p   pointer to the @p MS5541Driver object
 * @return              Pressure result in mbar
 *
 * @api
 */
uint16_t ms5541PressureResult(MS5541Driver* ms5541p)
{
    chDbgCheck(ms5541p != NULL, "ms5541PressureResult");
    /* Verify device status. */
    chDbgAssert(ms5541p->state == MS5541_ACTIVE,
            "ms5541PressureResult(), #1", "invalid state");

    /* Disable clock output if configured. */
    if (ms5541p->config->mclk_cb != NULL)
        ms5541p->config->mclk_cb(ms5541p, FALSE);

    /* Read result from chip. */
    ms5541p->last_d1 = ms5541_read(ms5541p);

    /* Reset driver state. */
    ms5541p->state = MS5541_READY;

    /* Calculate result. */
    int32_t UT1 = (int32_t)ms5541p->calibration[MS5541_CAL_TREF] * 8 + 10000;
    int32_t dT = (int32_t)ms5541p->last_d2 - UT1;

    int32_t dT2;
    if (dT < 0)
        dT2 = dT - (dT / 128 * dT / 128) / 2;
    else
        dT2 = dT - (dT / 128 * dT / 128) / 8;

    int32_t OFF = (int32_t)ms5541p->calibration[MS5541_CAL_OFFT1] +
            (((int32_t)ms5541p->calibration[MS5541_CAL_TCO] - 250) * dT2) / 4096 + 10000;

    int32_t SENS = (int32_t)ms5541p->calibration[MS5541_CAL_SENST1] / 2 +
            (((int32_t)ms5541p->calibration[MS5541_CAL_TCS] + 200) * dT2) / 8192 + 3000;

    int32_t P = (SENS * (ms5541p->last_d1 - OFF)) / 4096 + 1000;

    return P;
}

#endif /* HAL_USE_MS5541 */

/** @} */
