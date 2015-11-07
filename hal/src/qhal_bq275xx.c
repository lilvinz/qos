/**
 * @file    qbq275xx.c
 * @brief   BQ275XX driver code.
 *
 * @addtogroup BQ275XX
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_BQ275XX || defined(__DOXYGEN__)

#include "nelems.h"

#include <string.h>

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

static const uint16_t DEVICE_TYPE[] =
{
    0x0520,
};

enum reg_addr_e
{
    REG_CNTL = 0x00,
    REG_AR = 0x02,
    REG_ARTTE = 0x04,
    REG_TEMP = 0x06,
    REG_VOLT = 0x08,
    REG_FLAGS = 0x0a,
    REG_NAC = 0x0c,
    REG_FAC = 0x0e,
    REG_RM = 0x10,
    REG_FCC = 0x12,
    REG_AI = 0x14,
    REG_TTE = 0x16,
    REG_SI = 0x18,
    REG_STTE = 0x1a,
    REG_SOH = 0x1c,
    REG_CC = 0x1e,
    REG_SOC = 0x20,
    REG_I = 0x22,
    REG_INTTEMP = 0x28,
    REG_RSCALE = 0x2a,
    REG_OPERATION_CONFIG = 0x2c,
    REG_DESIGN_CAPACITY = 0x2e,
    REG_UFRM = 0x6c,
    REG_FRM = 0x6e,
    REG_UFFCC = 0x70,
    REG_FFCC = 0x72,
    REG_UFSOC = 0x74,
};

enum cntl_subcommand_e
{
    CNTL_STATUS = 0x0000,
    CNTL_DEVICE_TYPE = 0x0001,
    CNTL_FW_VERSION = 0x0002,
    CNTL_PREV_MACWRITE = 0x0007,
    CNTL_CHEM_ID = 0x0008,
    CNTL_OCV_CMD = 0x000c,
    CNTL_BAT_INSERT = 0x000d,
    CNTL_BAT_REMOVE = 0x000e,
    CNTL_SET_HIBERNATE = 0x0011,
    CNTL_CLEAR_HIBERNATE = 0x0012,
    CNTL_SET_SNOOZE = 0x0013,
    CNTL_CLEAR_SNOOZE = 0x0014,
    CNTL_DF_VERSION = 0x001f,
    CNTL_SEALED = 0x0020,
    CNTL_IT_ENABLE = 0x0021,
    CNTL_RESET = 0x0041,
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

static bool register_read(BQ275XXDriver* bq275xxp, enum reg_addr_e reg,
        uint16_t *valuep)
{
    const uint8_t txbuf[] = { reg };
    uint8_t rxbuf[] = { 0x00, 0x00, };

    msg_t result;
    result = i2cMasterTransmitTimeout(bq275xxp->configp->i2cp,
            bq275xxp->configp->i2c_address >> 1,
            txbuf,
            sizeof(txbuf),
            rxbuf,
            sizeof(rxbuf),
            bq275xxp->configp->i2c_timeout);

#if BQ275XX_NICE_WAITING
    chThdSleepMilliseconds(1);
#else
    osalSysPolledDelayX(OSAL_US2RTC(STM32_HCLK, 66));
#endif /* BQ275XX_NICE_WAITING */

    if (result == MSG_OK)
    {
        *valuep = ((uint16_t)rxbuf[1] << 8) | ((uint16_t)rxbuf[0] << 0);
        return true;
    }
    else
        return false;
}

static bool register_write(BQ275XXDriver* bq275xxp, enum reg_addr_e reg,
        uint16_t value)
{
    {
        const uint8_t txbuf[] = { reg + 0, (uint8_t)(value >> 0) };

        msg_t result;
        result = i2cMasterTransmitTimeout(bq275xxp->configp->i2cp,
                bq275xxp->configp->i2c_address >> 1,
                txbuf,
                sizeof(txbuf),
                NULL,
                0,
                bq275xxp->configp->i2c_timeout);

#if BQ275XX_NICE_WAITING
        chThdSleepMilliseconds(1);
#else
        osalSysPolledDelayX(OSAL_US2RTC(STM32_HCLK, 66));
#endif /* BQ275XX_NICE_WAITING */

        if (result != MSG_OK)
            return false;
    }

    {
        const uint8_t txbuf[] = { reg + 1, (uint8_t)(value >> 8) };

        msg_t result;
        result = i2cMasterTransmitTimeout(bq275xxp->configp->i2cp,
                bq275xxp->configp->i2c_address >> 1,
                txbuf,
                sizeof(txbuf),
                NULL,
                0,
                bq275xxp->configp->i2c_timeout);

#if BQ275XX_NICE_WAITING
        chThdSleepMilliseconds(1);
#else
        osalSysPolledDelayX(OSAL_US2RTC(STM32_HCLK, 66));
#endif /* BQ275XX_NICE_WAITING */

        if (result != MSG_OK)
            return false;
    }

    return true;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   BQ275XX driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void bq275xxInit(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[in] bq275xxp  pointer to the @p BQ275XXDriver object
 *
 * @init
 */
void bq275xxObjectInit(BQ275XXDriver* bq275xxp)
{
    bq275xxp->state = BQ275XX_STOP;
    bq275xxp->configp = NULL;
#if BQ275XX_USE_MUTUAL_EXCLUSION
#if CH_USE_MUTEXES
    chMtxInit(&bq275xxp->mutex);
#else
    chSemInit(&bq275xxp->semaphore, 1);
#endif
#endif /* BQ275XX_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the gas gauge.
 *
 * @param[in] bq275xxp  pointer to the @p BQ275XXDriver object
 * @param[in] config    pointer to the @p BQ275XXConfig object.
 *
 * @api
 */
void bq275xxStart(BQ275XXDriver* bq275xxp, const BQ275XXConfig* configp)
{
    osalDbgCheck((bq275xxp != NULL) && (configp != NULL));
    /* Verify device status. */
    osalDbgAssert((bq275xxp->state == BQ275XX_STOP) || (bq275xxp->state == BQ275XX_READY),
            "invalid state");

    bq275xxp->configp = configp;

    /* Verify device identification. */
    {
        if (register_write(bq275xxp, REG_CNTL, CNTL_DEVICE_TYPE) != true)
        {
            bq275xxp->state = BQ275XX_STOP;
            return;
        }

        uint16_t devid;
        if (register_read(bq275xxp, REG_CNTL, &devid) != true)
        {
            bq275xxp->state = BQ275XX_STOP;
            return;
        }

        bool devid_ok = false;
        for (size_t i = 0; i < NELEMS(DEVICE_TYPE); ++i)
        {
            if (devid == DEVICE_TYPE[i])
            {
                devid_ok = true;
                break;
            }
        }

        if (devid_ok == false)
        {
            bq275xxp->state = BQ275XX_STOP;
            return;
        }
    }

    bq275xxp->state = BQ275XX_READY;
}

/**
 * @brief   Disables the gas gauge driver.
 *
 * @param[in] bq275xxp  pointer to the @p BQ275XXDriver object
 *
 * @api
 */
void bq275xxStop(BQ275XXDriver* bq275xxp)
{
    osalDbgCheck(bq275xxp != NULL);
    /* Verify device status. */
    osalDbgAssert((bq275xxp->state == BQ275XX_STOP) || (bq275xxp->state == BQ275XX_READY),
            "invalid state");
#if 0
    /* Enter device standby mode. */
    const uint8_t txbuf[] = { REG_CTRL_REG1, 0x00 };

    msg_t result;
    result = i2cMasterTransmitTimeout(bq275xxp->configp->i2cp,
            bq275xxp->configp->i2c_address >> 1,
            txbuf,
            sizeof(txbuf),
            NULL,
            0,
            bq275xxp->configp->i2c_timeout);
#endif
    bq275xxp->state = BQ275XX_STOP;
}

/**
 * @brief   Gains exclusive access to the flash device.
 * @details This function tries to gain ownership to the device, if the
 *          device is already being used then the invoking thread is queued.
 * @pre     In order to use this function the option
 *          @p BQ275XX_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] bq275xxp  pointer to the @p BQ275XXDriver object
 *
 * @api
 */
void bq275xxAcquireBus(BQ275XXDriver* bq275xxp)
{
    osalDbgCheck(bq275xxp != NULL);

#if BQ275XX_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    chMtxLock(&bq275xxp->mutex);
#elif CH_USE_SEMAPHORES
    chSemWait(&bq275xxp->semaphore);
#endif

#if I2C_USE_MUTUAL_EXCLUSION
    /* Acquire the underlying device as well. */
    i2cAcquireBus(bq275xxp->config->i2cp);
#endif /* I2C_USE_MUTUAL_EXCLUSION */
#endif /* BQ275XX_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the device.
 * @pre     In order to use this function the option
 *          @p BQ275XX_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] bq275xxp  pointer to the @p BQ275XXDriver object
 *
 * @api
 */
void bq275xxReleaseBus(BQ275XXDriver* bq275xxp)
{
    osalDbgCheck(bq275xxp != NULL);

#if BQ275XX_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    (void)bq275xxp;
    chMtxUnlock();
#elif CH_USE_SEMAPHORES
    chSemSignal(&bq275xxp->semaphore);
#endif

#if I2C_USE_MUTUAL_EXCLUSION
    /* Release the underlying device as well. */
    i2cReleaseBus(bq275xxp->config->i2cp);
#endif /* I2C_USE_MUTUAL_EXCLUSION */
#endif /* BQ275XX_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Reads all battery data from gauge.
 *
 * @param[in] bq275xxp  pointer to the @p BQ275XXDriver object
 * @param[out] datap    pointer to @p bq275xx_bat_data_s structure
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool bq275xxReadData(BQ275XXDriver* bq275xxp, bq275xx_bat_data_s* datap)
{
    osalDbgCheck(bq275xxp != NULL);
    /* Verify device status. */
    osalDbgAssert(bq275xxp->state == BQ275XX_READY,
            "invalid state");

    bq275xxp->state = BQ275XX_ACTIVE;

    /* Check if device finished initialization. */
    uint16_t status;
    if (register_write(bq275xxp, REG_CNTL, CNTL_STATUS) != true)
        goto out_failed;
    if (register_read(bq275xxp, REG_CNTL, &status) == false)
        goto out_failed;
    if ((status & 0x0080) == 0x0000)
        goto out_failed;

    int16_t temp;

    if (register_read(bq275xxp, REG_TEMP, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->temperature = temp / 10 - 273.15f;

    if (register_read(bq275xxp, REG_VOLT, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->voltage = temp / 1000.0f;

    if (register_read(bq275xxp, REG_NAC, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->nom_available_capacity = temp / 1000.0f;

    if (register_read(bq275xxp, REG_FAC, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->full_available_capacity = temp / 1000.0f;

    if (register_read(bq275xxp, REG_RM, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->remaining_capacity = temp / 1000.0f;

    if (register_read(bq275xxp, REG_FCC, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->full_charge_capacity = temp / 1000.0f;

    if (register_read(bq275xxp, REG_AI, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->effective_current = temp / 1000.0f;

    if (register_read(bq275xxp, REG_SOC, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->state_of_charge = temp / 100.0f;

    if (register_read(bq275xxp, REG_INTTEMP, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->internal_temperature = temp / 10 - 273.15f;

    if (register_read(bq275xxp, REG_UFRM, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->rem_capacity_unfiltered = temp / 1000.0f;

    if (register_read(bq275xxp, REG_FRM, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->rem_capacity_filtered = temp / 1000.0f;

    if (register_read(bq275xxp, REG_UFFCC, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->full_charge_capacity_unfiltered = temp / 1000.0f;

    if (register_read(bq275xxp, REG_FFCC, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->full_charge_capacity_filtered = temp / 1000.0f;

    if (register_read(bq275xxp, REG_UFSOC, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->state_of_charge_unfiltered = temp / 100.0f;

    if (register_read(bq275xxp, REG_TTE, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->time_to_empty = temp / 60.0f;

    if (register_read(bq275xxp, REG_STTE, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->standby_time_to_empty = temp / 60.0f;

    bq275xxp->state = BQ275XX_READY;
    return HAL_SUCCESS;

out_failed:
    bq275xxp->state = BQ275XX_READY;
    return HAL_FAILED;
}

/**
 * @brief   Executes battery insert command.
 *
 * @param[in] bq275xxp  pointer to the @p BQ275XXDriver object
 * @param[out] datap    pointer to @p bq275xx_bat_data_s structure
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool bq275xxCommandBatInsert(BQ275XXDriver* bq275xxp)
{
    osalDbgCheck(bq275xxp != NULL);
    /* Verify device status. */
    osalDbgAssert(bq275xxp->state == BQ275XX_READY,
            "invalid state");

    bq275xxp->state = BQ275XX_ACTIVE;

    bool result = register_write(bq275xxp, REG_CNTL, CNTL_BAT_INSERT);

#if BQ275XX_NICE_WAITING
    chThdSleepMilliseconds(1);
#else
    osalSysPolledDelayX(OSAL_US2RTC(STM32_HCLK, 66));
#endif /* BQ275XX_NICE_WAITING */

    bq275xxp->state = BQ275XX_READY;

    if (result != true)
        return HAL_FAILED;

    return HAL_SUCCESS;
}

/**
 * @brief   Executes battery remove command.
 *
 * @param[in] bq275xxp  pointer to the @p BQ275XXDriver object
 * @param[out] datap    pointer to @p bq275xx_bat_data_s structure
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool bq275xxCommandBatRemove(BQ275XXDriver* bq275xxp)
{
    osalDbgCheck(bq275xxp != NULL);
    /* Verify device status. */
    osalDbgAssert(bq275xxp->state == BQ275XX_READY,
            "invalid state");

    bq275xxp->state = BQ275XX_ACTIVE;

    bool result = register_write(bq275xxp, REG_CNTL, CNTL_BAT_REMOVE);

#if BQ275XX_NICE_WAITING
    chThdSleepMilliseconds(1);
#else
    osalSysPolledDelayX(OSAL_US2RTC(STM32_HCLK, 66));
#endif /* BQ275XX_NICE_WAITING */

    bq275xxp->state = BQ275XX_READY;

    if (result != true)
        return HAL_FAILED;

    return HAL_SUCCESS;
}

#endif /* HAL_USE_BQ275XX */

/** @} */
