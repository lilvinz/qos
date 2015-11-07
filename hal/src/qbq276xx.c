/**
 * @file    qbq276xx.c
 * @brief   BQ276XX driver code.
 *
 * @addtogroup BQ276XX
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_BQ276XX || defined(__DOXYGEN__)

#include "nelems.h"

#include <string.h>

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

static const uint16_t DEVICE_TYPE[] = { 0x0621, };
static const systime_t SEALING_TIMEOUT = S2ST(10);
static const systime_t CFGUPDATE_TIMEOUT = S2ST(5);

enum reg_addr_e
{
    REG_CNTL = 0x00,
    REG_TEMP = 0x02,
    REG_VOLT = 0x04,
    REG_FLAGS = 0x06,
    REG_NOM_AVAIL_CAP = 0x08,
    REG_FULL_AVAIL_CAP = 0x0a,
    REG_REM_CAP = 0x0c,
    REG_FULL_CHARGE_CAP = 0x0e,
    REG_EFF_CURR = 0x10,
    REG_AVG_POWER = 0x18,
    REG_SOC = 0x1c,
    REG_INT_TEMP = 0x1e,
    REG_REM_CAP_UNFILTERED = 0x28,
    REG_REM_CAP_FILTERED = 0x2a,
    REG_FULL_CARGE_CAP_UNFILTERED = 0x2c,
    REG_FULL_CHARGE_CAP_FILTERED = 0x2e,
    REG_SOC_UNFILTERED = 0x30,
    REG_OPERACTION_CONFIG = 0x3a,
    REG_DESIGN_CAPACITY = 0x3c,
    REG_DATA_CLASS = 0x3e,                  /* 1 byte */
    REG_DATA_BLOCK = 0x3f,                  /* 1 byte */
    REG_BLOCK_DATA = 0x40,                  /* 32 bytes */
    REG_BLOCK_DATA_CHECKSUM = 0x60,         /* 1 byte */
    REG_BLOCK_DATA_CONTROL = 0x61,          /* 1 byte */
};

enum cntl_subcommand_e
{
    CNTL_STATUS = 0x0000,
    CNTL_DEVICE_TYPE = 0x0001,
    CNTL_FW_VERSION = 0x0002,
    CNTL_PREV_MACWRITE = 0x0007,
    CNTL_CHEM_ID = 0x0008,
    CNTL_BAT_INSERT = 0x000c,
    CNTL_BAT_REMOVE = 0x000d,
    CNTL_TOGGLE_POWERMIN = 0x0010,
    CNTL_SET_HIBERNATE = 0x0011,
    CNTL_CLEAR_HIBERNATE = 0x0012,
    CNTL_SET_CFGUPDATE = 0x0013,
    CNTL_SHUTDOWN_ENABLE = 0x001b,
    CNTL_SHUTDOWN = 0x001c,
    CNTL_SEALED = 0x0020,
    CNTL_TOGGLE_GPOUT = 0x0023,
    CNTL_ALT_CHEM1 = 0x0031,
    CNTL_ALT_CHEM2 = 0x0032,
    CNTL_RESET = 0x0041,
    CNTL_SOFT_RESET = 0x0042,
    CNTL_EXIT_CFGUPDATE = 0x0043,
    CNTL_EXIT_RESIM = 0x0044,
    CNTL_UNSEALED = 0x8000,
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

static bool register_read(BQ276XXDriver* bq276xxp, enum reg_addr_e reg,
        uint16_t *valuep)
{
    const uint8_t txbuf[] = { reg };
    uint8_t rxbuf[] = { 0x00, 0x00, };

    msg_t result;
    result = i2cMasterTransmitTimeout(bq276xxp->configp->i2cp,
            bq276xxp->configp->i2c_address >> 1,
            txbuf,
            sizeof(txbuf),
            rxbuf,
            sizeof(rxbuf),
            bq276xxp->configp->i2c_timeout);

#if BQ276XX_NICE_WAITING
    chThdSleepMilliseconds(1);
#else
    halPolledDelay(US2RTT(66));
#endif /* BQ276XX_NICE_WAITING */

    if (result == RDY_OK)
    {
        *valuep = ((uint16_t)rxbuf[1] << 8) | ((uint16_t)rxbuf[0] << 0);
        return true;
    }
    else
        return false;
}

static bool register_write(BQ276XXDriver* bq276xxp, enum reg_addr_e reg,
        uint16_t value)
{
    {
        const uint8_t txbuf[] = { reg + 0, (uint8_t)(value >> 0) };

        msg_t result;
        result = i2cMasterTransmitTimeout(bq276xxp->configp->i2cp,
                bq276xxp->configp->i2c_address >> 1,
                txbuf,
                sizeof(txbuf),
                NULL,
                0,
                bq276xxp->configp->i2c_timeout);

#if BQ276XX_NICE_WAITING
        chThdSleepMilliseconds(1);
#else
        halPolledDelay(US2RTT(66));
#endif /* BQ276XX_NICE_WAITING */

        if (result != RDY_OK)
            return false;
    }

    {
        const uint8_t txbuf[] = { reg + 1, (uint8_t)(value >> 8) };

        msg_t result;
        result = i2cMasterTransmitTimeout(bq276xxp->configp->i2cp,
                bq276xxp->configp->i2c_address >> 1,
                txbuf,
                sizeof(txbuf),
                NULL,
                0,
                bq276xxp->configp->i2c_timeout);

#if BQ276XX_NICE_WAITING
        chThdSleepMilliseconds(1);
#else
        halPolledDelay(US2RTT(66));
#endif /* BQ276XX_NICE_WAITING */

        if (result != RDY_OK)
            return false;
    }

    return true;
}

static bool register_read_byte(BQ276XXDriver* bq276xxp, enum reg_addr_e reg,
        uint8_t *valuep)
{
    const uint8_t txbuf[] = { reg, };
    uint8_t rxbuf[] = { 0x00, };

    msg_t result;
    result = i2cMasterTransmitTimeout(bq276xxp->configp->i2cp,
            bq276xxp->configp->i2c_address >> 1,
            txbuf,
            sizeof(txbuf),
            rxbuf,
            sizeof(rxbuf),
            bq276xxp->configp->i2c_timeout);

#if BQ276XX_NICE_WAITING
    chThdSleepMilliseconds(1);
#else
    halPolledDelay(US2RTT(66));
#endif /* BQ276XX_NICE_WAITING */

    if (result == RDY_OK)
    {
        *valuep = rxbuf[0];
        return true;
    }
    else
        return false;
}

static bool register_write_byte(BQ276XXDriver* bq276xxp, enum reg_addr_e reg,
        uint8_t value)
{
    {
        const uint8_t txbuf[] = { reg, value };

        msg_t result;
        result = i2cMasterTransmitTimeout(bq276xxp->configp->i2cp,
                bq276xxp->configp->i2c_address >> 1,
                txbuf,
                sizeof(txbuf),
                NULL,
                0,
                bq276xxp->configp->i2c_timeout);

#if BQ276XX_NICE_WAITING
        chThdSleepMilliseconds(1);
#else
        halPolledDelay(US2RTT(66));
#endif /* BQ276XX_NICE_WAITING */

        if (result != RDY_OK)
            return false;
    }

    return true;
}

static bool device_unseal(BQ276XXDriver* bq276xxp)
{
    uint16_t status;

    /* Check if device is sealed. */
    if (register_write(bq276xxp, REG_CNTL, CNTL_STATUS) != true)
        return false;
    if (register_read(bq276xxp, REG_CNTL, &status) == false)
        return false;
    if ((status & 0x2000) == 0x0000)
        return true;

    /* Unseal device. */
    if (register_write(bq276xxp, REG_CNTL, CNTL_UNSEALED) == false)
        return false;
    chThdSleepMilliseconds(5);

    if (register_write(bq276xxp, REG_CNTL, CNTL_UNSEALED) == false)
        return false;
    chThdSleepMilliseconds(5);

    /* Wait until unsealing is completed or timeout. */
    systime_t start = chTimeNow();
    do
    {
        if (register_write(bq276xxp, REG_CNTL, CNTL_STATUS) != true)
            return false;
        if (register_read(bq276xxp, REG_CNTL, &status) == false)
            return false;
        if ((status & 0x2000) == 0x2000)
            chThdSleepMilliseconds(10);
    } while ((status & 0x2000) == 0x2000 && chTimeElapsedSince(start) <= SEALING_TIMEOUT);

    if ((status & 0x2000) == 0x2000)
        return false;

    return true;
}

static bool device_seal(BQ276XXDriver* bq276xxp)
{
    uint16_t status;

    /* Check if device is sealed. */
    if (register_write(bq276xxp, REG_CNTL, CNTL_STATUS) != true)
        return false;
    if (register_read(bq276xxp, REG_CNTL, &status) == false)
        return false;
    if ((status & 0x2000) == 0x2000)
        return true;

    /* Check if sealing feature is enabled. */
    {
        if (register_write_byte(bq276xxp, REG_BLOCK_DATA_CONTROL, 0x00) != true)
            return false;
        chThdSleepMilliseconds(5);

        if (register_write_byte(bq276xxp, REG_DATA_CLASS, 64) != true)
            return false;
        chThdSleepMilliseconds(5);

        if (register_write_byte(bq276xxp, REG_DATA_BLOCK, 0) != true)
            return false;
        chThdSleepMilliseconds(5);

        uint8_t cfg_b;
        if (register_read_byte(bq276xxp, REG_BLOCK_DATA + 2, &cfg_b) != true)
            return false;

        if ((cfg_b & 0x20) == 0x00)
            return true;
    }

    /* Actually seal device. */
    if (register_write(bq276xxp, REG_CNTL, CNTL_SEALED) == false)
        return false;

    systime_t start = chTimeNow();
    do
    {
        if (register_write(bq276xxp, REG_CNTL, CNTL_STATUS) != true)
            return false;
        if (register_read(bq276xxp, REG_CNTL, &status) == false)
            return false;
        if ((status & 0x2000) == 0x0000)
            chThdSleepMilliseconds(10);
    } while ((status & 0x2000) == 0x0000 && chTimeElapsedSince(start) <= SEALING_TIMEOUT);

    if ((status & 0x2000) == 0x0000)
        return false;

    return true;
}

static bool cfg_update_enter(BQ276XXDriver* bq276xxp)
{
    if (register_write(bq276xxp, REG_CNTL, CNTL_SET_CFGUPDATE) == false)
        return false;
    chThdSleepMilliseconds(5);

    /* Wait for configuration update mode. */
    systime_t start = chTimeNow();
    uint16_t flags;
    do
    {
        if (register_read(bq276xxp, REG_FLAGS, &flags) == false)
            return false;
        if ((flags & 0x0010) == 0x0000)
            chThdSleepMilliseconds(100);
    } while ((flags & 0x0010) == 0x0000 && chTimeElapsedSince(start) <= CFGUPDATE_TIMEOUT);

    if (flags & 0x0010)
        return true;
    else
        return false;
}

static bool cfg_update_exit(BQ276XXDriver* bq276xxp)
{
    if (register_write(bq276xxp, REG_CNTL, CNTL_SOFT_RESET) == false)
        return false;

    /* Wait for configuration update to exit. */
    systime_t start = chTimeNow();
    uint16_t flags;
    do
    {
        if (register_read(bq276xxp, REG_FLAGS, &flags) == false)
            return false;
        if ((flags & 0x0010) == 0x0010)
            chThdSleepMilliseconds(100);
    } while ((flags & 0x0010) == 0x0010 && chTimeElapsedSince(start) <= CFGUPDATE_TIMEOUT);

    if ((flags & 0x0010) == 0x0000)
        return true;
    else
        return false;
}

static bool update_configuration(BQ276XXDriver* bq276xxp)
{
    /* Unseal device. */
    if (device_unseal(bq276xxp) != true)
        return false;

    /* Enable block data control. */
    if (register_write_byte(bq276xxp, REG_BLOCK_DATA_CONTROL, 0x00) != true)
        return false;
    chThdSleepMilliseconds(5);

    bool cfg_update_mode = false;

    uint8_t last_data_class = 0xff;
    uint8_t last_data_block = 0xff;

    /* Walk through data memory parameters. */
    for (size_t ireg = 0; ireg < bq276xxp->configp->dm_reg_count; ++ireg)
    {
        const struct dm_reg_setup_s* regp = &bq276xxp->configp->dm_reg_setup[ireg];

        /* Setup window to dm. */
        const uint8_t data_class = regp->subclass;
        const uint8_t data_block = regp->offset / 32;
        if (data_class != last_data_class ||
                data_block != last_data_block)
        {
            if (register_write_byte(bq276xxp, REG_DATA_CLASS, data_class) != true)
                return false;
            chThdSleepMilliseconds(5);
            last_data_class = data_class;

            if (register_write_byte(bq276xxp, REG_DATA_BLOCK, data_block) != true)
                return false;
            chThdSleepMilliseconds(5);
            last_data_block = data_block;
        }

        /* Read old value. */
        uint8_t old_value[] = { 0x00, 0x00, 0x00, 0x00 };
        for (size_t i = 0; i < regp->len; ++i)
        {
            if (register_read_byte(bq276xxp,
                    REG_BLOCK_DATA + ((regp->offset + i) % 32),
                    old_value + i) != true)
                return false;
        }

        /* Generate new value. */
        uint8_t new_value[] = { 0x00, 0x00, 0x00, 0x00 };
        switch (regp->len)
        {
        case 1:
            new_value[0] = (regp->data >> 0) & 0xff;
            break;
        case 2:
            new_value[0] = (regp->data >> 8) & 0xff;
            new_value[1] = (regp->data >> 0) & 0xff;
            break;
        case 4:
            new_value[0] = (regp->data >> 24) & 0xff;
            new_value[1] = (regp->data >> 16) & 0xff;
            new_value[2] = (regp->data >> 8) & 0xff;
            new_value[3] = (regp->data >> 0) & 0xff;
            break;
        default:
            chDbgPanic("Invalid configuration!");
            break;
        }

        /* Check for differences. */
        if (memcmp(new_value, old_value, sizeof(new_value)))
        {
            /* Enter configuration update mode. */
            if (!cfg_update_mode)
            {
                if (cfg_update_enter(bq276xxp) == false)
                    return false;
                cfg_update_mode = true;
            }

            /* Read old checksum. */
            uint8_t old_checksum;
            if (register_read_byte(bq276xxp, REG_BLOCK_DATA_CHECKSUM, &old_checksum) != true)
                return false;

            /* Write new value. */
            for (size_t i = 0; i < regp->len; ++i)
            {
                if (register_write_byte(bq276xxp,
                        REG_BLOCK_DATA + ((regp->offset + i) % 32),
                        new_value[i]) != true)
                    return false;
            }

            /* Calculate new checksum. */
            uint8_t new_checksum = 255 - old_checksum;
            for (size_t i = 0; i < regp->len; ++i)
            {
                new_checksum -= old_value[i];
                new_checksum += new_value[i];
            }
            new_checksum = 255 - new_checksum;

            /* Write new checksum. */
            if (register_write_byte(bq276xxp, REG_BLOCK_DATA_CHECKSUM, new_checksum) != true)
                return false;
        }
    }

    /* Exit configuration update mode. */
    if (cfg_update_mode)
    {
        if (cfg_update_exit(bq276xxp) == false)
            return false;
        cfg_update_mode = false;
    }

    /* Reseal device. */
    if (device_seal(bq276xxp) == false)
        return false;

    return true;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   BQ276XX driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void bq276xxInit(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[in] bq276xxp  pointer to the @p BQ276XXDriver object
 *
 * @init
 */
void bq276xxObjectInit(BQ276XXDriver* bq276xxp)
{
    bq276xxp->state = BQ276XX_STOP;
    bq276xxp->configp = NULL;
#if BQ276XX_USE_MUTUAL_EXCLUSION
#if CH_USE_MUTEXES
    chMtxInit(&bq276xxp->mutex);
#else
    chSemInit(&bq276xxp->semaphore, 1);
#endif
#endif /* BQ276XX_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the gas gauge.
 *
 * @param[in] bq276xxp  pointer to the @p BQ276XXDriver object
 * @param[in] config    pointer to the @p BQ276XXConfig object.
 *
 * @api
 */
void bq276xxStart(BQ276XXDriver* bq276xxp, const BQ276XXConfig* configp)
{
    chDbgCheck((bq276xxp != NULL) && (configp != NULL), "bq276xxStart");
    /* Verify device status. */
    chDbgAssert((bq276xxp->state == BQ276XX_STOP) || (bq276xxp->state == BQ276XX_READY),
            "bq276xxStart(), #1", "invalid state");

    bq276xxp->configp = configp;

    /* Wait for device to finish initialization. */
    {
        systime_t start = chTimeNow();
        uint16_t status;
        do
        {
            if (register_write(bq276xxp, REG_CNTL, CNTL_STATUS) != true)
            {
                bq276xxp->state = BQ276XX_STOP;
                return;
            }
            if (register_read(bq276xxp, REG_CNTL, &status) == false)
            {
                bq276xxp->state = BQ276XX_STOP;
                return;
            }
            if ((status & 0x0080) == 0x0000)
                chThdSleepMilliseconds(100);
        } while ((status & 0x0080) == 0x0000 && chTimeElapsedSince(start) <= S2ST(5));

        if ((status & 0x0080) == 0x0000)
        {
            bq276xxp->state = BQ276XX_STOP;
            return;
        }
    }

    /* Verify device identification. */
    {
        if (register_write(bq276xxp, REG_CNTL, CNTL_DEVICE_TYPE) != true)
        {
            bq276xxp->state = BQ276XX_STOP;
            return;
        }

        uint16_t devid;
        if (register_read(bq276xxp, REG_CNTL, &devid) != true)
        {
            bq276xxp->state = BQ276XX_STOP;
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
            bq276xxp->state = BQ276XX_STOP;
            return;
        }
    }

    /* Update device configuration. */
    if (update_configuration(bq276xxp) != true)
    {
        bq276xxp->state = BQ276XX_STOP;
        return;
    }

    bq276xxp->state = BQ276XX_READY;
}

/**
 * @brief   Disables the gas gauge driver.
 *
 * @param[in] bq276xxp  pointer to the @p BQ276XXDriver object
 *
 * @api
 */
void bq276xxStop(BQ276XXDriver* bq276xxp)
{
    chDbgCheck(bq276xxp != NULL, "bq276xxStop");
    /* Verify device status. */
    chDbgAssert((bq276xxp->state == BQ276XX_STOP) || (bq276xxp->state == BQ276XX_READY),
            "bq276xxStop(), #1", "invalid state");
#if 0
    /* Enter device standby mode. */
    const uint8_t txbuf[] = { REG_CTRL_REG1, 0x00 };

    msg_t result;
    result = i2cMasterTransmitTimeout(bq276xxp->configp->i2cp,
            bq276xxp->configp->i2c_address >> 1,
            txbuf,
            sizeof(txbuf),
            NULL,
            0,
            bq276xxp->configp->i2c_timeout);
#endif
    bq276xxp->state = BQ276XX_STOP;
}

/**
 * @brief   Gains exclusive access to the flash device.
 * @details This function tries to gain ownership to the device, if the
 *          device is already being used then the invoking thread is queued.
 * @pre     In order to use this function the option
 *          @p BQ276XX_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] bq276xxp  pointer to the @p BQ276XXDriver object
 *
 * @api
 */
void bq276xxAcquireBus(BQ276XXDriver* bq276xxp)
{
    chDbgCheck(bq276xxp != NULL, "bq276xxAcquireBus");

#if BQ276XX_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    chMtxLock(&bq276xxp->mutex);
#elif CH_USE_SEMAPHORES
    chSemWait(&bq276xxp->semaphore);
#endif

#if I2C_USE_MUTUAL_EXCLUSION
    /* Acquire the underlying device as well. */
    i2cAcquireBus(bq276xxp->config->i2cp);
#endif /* I2C_USE_MUTUAL_EXCLUSION */
#endif /* BQ276XX_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the device.
 * @pre     In order to use this function the option
 *          @p BQ276XX_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] bq276xxp  pointer to the @p BQ276XXDriver object
 *
 * @api
 */
void bq276xxReleaseBus(BQ276XXDriver* bq276xxp)
{
    chDbgCheck(bq276xxp != NULL, "bq276xxReleaseBus");

#if BQ276XX_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    (void)bq276xxp;
    chMtxUnlock();
#elif CH_USE_SEMAPHORES
    chSemSignal(&bq276xxp->semaphore);
#endif

#if I2C_USE_MUTUAL_EXCLUSION
    /* Release the underlying device as well. */
    i2cReleaseBus(bq276xxp->config->i2cp);
#endif /* I2C_USE_MUTUAL_EXCLUSION */
#endif /* BQ276XX_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Reads all battery data from gauge.
 *
 * @param[in] bq276xxp  pointer to the @p BQ276XXDriver object
 * @param[out] datap    pointer to @p bq276xx_bat_data_s structure
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   the operation succeeded.
 * @retval CH_FAILED    the operation failed.
 *
 * @api
 */
bool_t bq276xxReadData(BQ276XXDriver* bq276xxp, bq276xx_bat_data_s* datap)
{
    chDbgCheck(bq276xxp != NULL, "bq276xxReadData");
    /* Verify device status. */
    chDbgAssert(bq276xxp->state == BQ276XX_READY,
            "bq276xxReadData(), #1", "invalid state");

    bq276xxp->state = BQ276XX_ACTIVE;

    int16_t temp;

    if (register_read(bq276xxp, REG_TEMP, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->temperature = temp / 10 - 273.15f;

    if (register_read(bq276xxp, REG_VOLT, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->voltage = temp / 1000.0f;

    if (register_read(bq276xxp, REG_NOM_AVAIL_CAP, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->nom_available_capacity = temp / 1000.0f;

    if (register_read(bq276xxp, REG_FULL_AVAIL_CAP, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->full_available_capacity = temp / 1000.0f;

    if (register_read(bq276xxp, REG_REM_CAP, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->remaining_capacity = temp / 1000.0f;

    if (register_read(bq276xxp, REG_FULL_CHARGE_CAP, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->full_charge_capacity = temp / 1000.0f;

    if (register_read(bq276xxp, REG_EFF_CURR, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->effective_current = temp / 1000.0f;

    if (register_read(bq276xxp, REG_AVG_POWER, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->average_power = temp / 1000.0f;

    if (register_read(bq276xxp, REG_SOC, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->state_of_charge = temp / 100.0f;

    if (register_read(bq276xxp, REG_INT_TEMP, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->internal_temperature = temp / 10 - 273.15f;

    if (register_read(bq276xxp, REG_REM_CAP_UNFILTERED, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->rem_capacity_unfiltered = temp / 1000.0f;

    if (register_read(bq276xxp, REG_REM_CAP_FILTERED, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->rem_capacity_filtered = temp / 1000.0f;

    if (register_read(bq276xxp, REG_FULL_CARGE_CAP_UNFILTERED, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->full_charge_capacity_unfiltered = temp / 1000.0f;

    if (register_read(bq276xxp, REG_FULL_CHARGE_CAP_FILTERED, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->full_charge_capacity_filtered = temp / 1000.0f;

    if (register_read(bq276xxp, REG_SOC_UNFILTERED, (uint16_t*)&temp) != true)
        goto out_failed;
    datap->state_of_charge_unfiltered = temp / 100.0f;

    bq276xxp->state = BQ276XX_READY;
    return CH_SUCCESS;

out_failed:
    bq276xxp->state = BQ276XX_READY;
    return CH_FAILED;
}

#endif /* HAL_USE_BQ276XX */

/** @} */
