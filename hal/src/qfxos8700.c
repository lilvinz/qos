/**
 * @file    qfxos8700.c
 * @brief   FXOS8700 driver code.
 *
 * @addtogroup FXOS8700
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_FXOS8700 || defined(__DOXYGEN__)

#include "nelems.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

static const uint8_t WHO_AM_I_VALUE = 0xc7;

enum reg_addr_e
{
    REG_STATUS = 0x00,
    REG_OUT_X_MSB = 0x01,
    REG_OUT_X_LSB = 0x02,
    REG_OUT_Y_MSB = 0x03,
    REG_OUT_Y_LSB = 0x04,
    REG_OUT_Z_MSB = 0x05,
    REG_OUT_Z_LSB = 0x06,
    REG_F_SETUP = 0x09,
    REG_TRIG_CFG = 0x0a,
    REG_SYSMOD = 0x0b,
    REG_INT_SOURCE = 0x0c,
    REG_WHO_AM_I = 0x0d,
    REG_XYZ_DATA_CFG = 0x0e,
    REG_HP_FILTER_CUROFF = 0x0f,
    REG_PL_STATUS = 0x10,
    REG_PL_CFG = 0x11,
    REG_PL_COUNT = 0x12,
    REG_PL_BF_ZCOMP = 0x13,
    REG_PL_THS_REG = 0x14,
    REG_A_FFMT_CFG = 0x15,
    REG_A_FFMT_SRC = 0x16,
    REG_A_FFMT_THS = 0x17,
    REG_FFMT_COUNT = 0x18,
    REG_TRANSIENT_CFG = 0x1d,
    REG_TRANSIENT_SRC = 0x1e,
    REG_TRANSIENT_THS = 0x1f,
    REG_TRANSIENT_COUNT = 0x20,
    REG_PULSE_CFG = 0x21,
    REG_PULSE_SRC = 0x22,
    REG_PULSE_THSX = 0x23,
    REG_PULSE_THSY = 0x24,
    REG_PULSE_THSZ = 0x25,
    REG_PULSE_TMLT = 0x26,
    REG_PULSE_LTCY = 0x27,
    REG_PULSE_WIND = 0x28,
    REG_ASLP_COUNT = 0x29,
    REG_CTRL_REG1 = 0x2a,
    REG_CTRL_REG2 = 0x2b,
    REG_CTRL_REG3 = 0x2c,
    REG_CTRL_REG4 = 0x2d,
    REG_CTRL_REG5 = 0x2e,
    REG_OFF_X = 0x2f,
    REG_OFF_Y = 0x30,
    REG_OFF_Z = 0x31,
    REG_M_DR_STATUS = 0x32,
    REG_M_OUT_X_MSB = 0x33,
    REG_M_OUT_X_LSB = 0x34,
    REG_M_OUT_Y_MSB = 0x35,
    REG_M_OUT_Y_LSB = 0x36,
    REG_M_OUT_Z_MSB = 0x37,
    REG_M_OUT_Z_LSB = 0x38,
    REG_CMP_X_MSB = 0x39,
    REG_CMP_X_LSB = 0x3a,
    REG_CMP_Y_MSB = 0x3b,
    REG_CMP_Y_LSB = 0x3c,
    REG_CMP_Z_MSB = 0x3d,
    REG_CMP_Z_LSB = 0x3e,
    REG_M_OFF_X_MSB = 0x3f,
    REG_M_OFF_X_LSB = 0x40,
    REG_M_OFF_Y_MSB = 0x41,
    REG_M_OFF_Y_LSB = 0x42,
    REG_M_OFF_Z_MSB = 0x43,
    REG_M_OFF_Z_LSB = 0x44,
    REG_MAX_X_MSB = 0x45,
    REG_MAX_X_LSB = 0x46,
    REG_MAX_Y_MSB = 0x47,
    REG_MAX_Y_LSB = 0x48,
    REG_MAX_Z_MSB = 0x49,
    REG_MAX_Z_LSB = 0x4a,
    REG_MIN_X_MSB = 0x4b,
    REG_MIN_X_LSB = 0x4c,
    REG_MIN_Y_MSB = 0x4d,
    REG_MIN_Y_LSB = 0x4e,
    REG_MIN_Z_MSB = 0x4f,
    REG_MIN_Z_LSB = 0x50,
    REG_TEMP = 0x51,
    REG_M_THS_CFG = 0x52,
    REG_M_THS_SRC = 0x53,
    REG_M_THS_X_MSB = 0x54,
    REG_M_THS_X_LSB = 0x55,
    REG_M_THS_Y_MSB = 0x56,
    REG_M_THS_Y_LSB = 0x57,
    REG_M_THS_Z_MSB = 0x58,
    REG_M_THS_Z_LSB = 0x59,
    REG_M_THS_COUNT = 0x5a,
    REG_M_CTRL_REG1 = 0x5b,
    REG_M_CTRL_REG2 = 0x5c,
    REG_M_CTRL_REG3 = 0x5d,
    REG_M_INT_SRC = 0x5e,
    REG_A_VECM_CFG = 0x5f,
    REG_A_VECM_THS_MSB = 0x60,
    REG_A_VECM_THS_LSB = 0x61,
    REG_A_VECM_CNT = 0x62,
    REG_A_VECM_INTX_MSB = 0x63,
    REG_A_VECM_INTX_LSB = 0x64,
    REG_A_VECM_INTY_MSB = 0x65,
    REG_A_VECM_INTY_LSB = 0x66,
    REG_A_VECM_INTZ_MSB = 0x67,
    REG_A_VECM_INTZ_LSB = 0x68,
    REG_M_VECM_CFG = 0x69,
    REG_M_VECM_THS_MSB = 0x6a,
    REG_M_VECM_THS_LSB = 0x6b,
    REG_M_VECM_CNT = 0x6c,
    REG_M_VECM_INTX_MSB = 0x6d,
    REG_M_VECM_INTX_LSB = 0x6e,
    REG_M_VECM_INTY_MSB = 0x6f,
    REG_M_VECM_INTY_LSB = 0x70,
    REG_M_VECM_INTZ_MSB = 0x71,
    REG_M_VECM_INTZ_LSB = 0x72,
    REG_A_FFMT_THS_X_MSB = 0x73,
    REG_A_FFMT_THS_X_LSB = 0x74,
    REG_A_FFMT_THS_Y_MSB = 0x75,
    REG_A_FFMT_THS_Y_LSB = 0x76,
    REG_A_FFMT_THS_Z_MSB = 0x77,
    REG_A_FFMT_THS_Z_LSB = 0x78,
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
 * @brief   FXOS8700 driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void fxos8700Init(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] fxos8700p pointer to the @p FXOS8700Driver object
 *
 * @init
 */
void fxos8700ObjectInit(FXOS8700Driver* fxos8700p)
{
    fxos8700p->state = FXOS8700_STOP;
    fxos8700p->configp = NULL;
#if FXOS8700_USE_MUTUAL_EXCLUSION
#if CH_USE_MUTEXES
    chMtxInit(&fxos8700p->mutex);
#else
    chSemInit(&fxos8700p->semaphore, 1);
#endif
#endif /* FXOS8700_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the FLASH peripheral.
 *
 * @param[in] fxos8700p pointer to the @p FXOS8700Driver object
 * @param[in] config    pointer to the @p FXOS8700Config object.
 *
 * @api
 */
void fxos8700Start(FXOS8700Driver* fxos8700p, const FXOS8700Config* configp)
{
    osalDbgCheck((fxos8700p != NULL) && (configp != NULL));
    /* Verify device status. */
    osalDbgAssert((fxos8700p->state == FXOS8700_STOP) || (fxos8700p->state == FXOS8700_READY),
            "invalid state");

    fxos8700p->configp = configp;

    /* Verify device identification. */
    {
        const uint8_t txbuf[] = { REG_WHO_AM_I };
        uint8_t rxbuf[] = { 0x00 };

        msg_t result;
        result = i2cMasterTransmitTimeout(fxos8700p->configp->i2cp,
                fxos8700p->configp->i2c_address >> 1,
                txbuf,
                sizeof(txbuf),
                rxbuf,
                sizeof(rxbuf),
                fxos8700p->configp->i2c_timeout);

        if (result != MSG_OK || rxbuf[0] != WHO_AM_I_VALUE)
        {
            fxos8700p->state = FXOS8700_STOP;
            return;
        }
    }

    /* Setup device mode registers. */
    static const struct
    {
        enum reg_addr_e reg;
        uint8_t value;
    } register_setup[] =
    {
        /* [7-1] = 0000 000
         * [0]: active=0
         */
        { .reg = REG_CTRL_REG1, .value = 0x00, },

        /* [7]: m_acal=0: auto calibration disabled
         * [6]: m_rst=0: one-shot magnetic reset disabled
         * [5]: m_ost=0: one-shot magnetic measurement disabled
         * [4-2]: m_os=111=7: 8x oversampling (for 200Hz) to reduce magnetometer noise
         * [1-0]: m_hms=11=3: select hybrid mode with accel and magnetometer active
         */
        { .reg = REG_M_CTRL_REG1, .value = 0x1f, },

        /* [7]: reserved
         * [6]: reserved
         * [5]: hyb_autoinc_mode=1 to map the magnetometer registers to follow the accelerometer registers
         * [4]: m_maxmin_dis=0 to retain default min/max latching even though not used
         * [3]: m_maxmin_dis_ths=0
         * [2]: m_maxmin_rst=0
         * [1-0]: m_rst_cnt=00 to enable magnetic reset each cycle
         */
        { .reg = REG_M_CTRL_REG2, .value = 0x20, },

        /* [7]: reserved
         * [6]: reserved
         * [5]: reserved
         * [4]: hpf_out=0
         * [3]: reserved
         * [2]: reserved
         * [1-0]: fs=01 for 4g mode: 2048 counts / g = 8192 counts / g after 2 bit left shift
         */
        { .reg = REG_XYZ_DATA_CFG, .value = 0x01, },

        /* [7]: st=0: self test disabled
         * [6]: rst=0: reset disabled
         * [5]: unused
         * [4-3]: smods=00
         * [2]: slpe=0: auto sleep disabled
         * [1-0]: mods=10 for high resolution (maximum over sampling)
         */
        { .reg = REG_CTRL_REG2, .value = 0x02, },

        /* [7-6]: aslp_rate=00
         * [5-3]: dr=001=1 for 200Hz data rate (when in hybrid mode)
         * [2]: lnoise=1 for low noise mode (since we're in 4g mode)
         * [1]: f_read=0 for normal 16 bit reads
         * [0]: active=1 to take the part out of standby and enable sampling
         */
        { .reg = REG_CTRL_REG1, .value = 0x0d, },
    };

    for (size_t i_reg = 0; i_reg < NELEMS(register_setup); ++i_reg)
    {
        const uint8_t txbuf[] =
            { register_setup[i_reg].reg, register_setup[i_reg].value };

        msg_t result;
        result = i2cMasterTransmitTimeout(fxos8700p->configp->i2cp,
                fxos8700p->configp->i2c_address >> 1,
                txbuf,
                sizeof(txbuf),
                NULL,
                0,
                fxos8700p->configp->i2c_timeout);

        if (result != MSG_OK)
        {
            fxos8700p->state = FXOS8700_STOP;
            return;
        }
    }

    fxos8700p->state = FXOS8700_READY;
}

/**
 * @brief   Disables the FLASH peripheral.
 *
 * @param[in] fxos8700p pointer to the @p FXOS8700Driver object
 *
 * @api
 */
void fxos8700Stop(FXOS8700Driver* fxos8700p)
{
    osalDbgCheck(fxos8700p != NULL);
    /* Verify device status. */
    osalDbgAssert((fxos8700p->state == FXOS8700_STOP) || (fxos8700p->state == FXOS8700_READY),
            "invalid state");

    /* Enter device standby mode. */
    const uint8_t txbuf[] = { REG_CTRL_REG1, 0x00 };

    msg_t result;
    result = i2cMasterTransmitTimeout(fxos8700p->configp->i2cp,
            fxos8700p->configp->i2c_address >> 1,
            txbuf,
            sizeof(txbuf),
            NULL,
            0,
            fxos8700p->configp->i2c_timeout);

    fxos8700p->state = FXOS8700_STOP;
}

/**
 * @brief   Gains exclusive access to the flash device.
 * @details This function tries to gain ownership to the flash device, if the
 *          device is already being used then the invoking thread is queued.
 * @pre     In order to use this function the option
 *          @p FXOS8700_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] fxos8700p pointer to the @p FXOS8700Driver object
 *
 * @api
 */
void fxos8700AcquireBus(FXOS8700Driver* fxos8700p)
{
    osalDbgCheck(fxos8700p != NULL);

#if FXOS8700_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    chMtxLock(&fxos8700p->mutex);
#elif CH_USE_SEMAPHORES
    chSemWait(&fxos8700p->semaphore);
#endif

#if I2C_USE_MUTUAL_EXCLUSION
    /* Acquire the underlying device as well. */
    i2cAcquireBus(fxos8700p->config->i2cp);
#endif /* I2C_USE_MUTUAL_EXCLUSION */
#endif /* FXOS8700_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the flash device.
 * @pre     In order to use this function the option
 *          @p FXOS8700_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] fxos8700p pointer to the @p FXOS8700Driver object
 *
 * @api
 */
void fxos8700ReleaseBus(FXOS8700Driver* fxos8700p)
{
    osalDbgCheck(fxos8700p != NULL);

#if FXOS8700_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    (void)fxos8700p;
    chMtxUnlock();
#elif CH_USE_SEMAPHORES
    chSemSignal(&fxos8700p->semaphore);
#endif

#if I2C_USE_MUTUAL_EXCLUSION
    /* Release the underlying device as well. */
    i2cReleaseBus(fxos8700p->config->i2cp);
#endif /* I2C_USE_MUTUAL_EXCLUSION */
#endif /* FXOS8700_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Starts a temperature conversion.
 *
 * @param[in] fxos8700p pointer to the @p FXOS8700Driver object
 * @param[out] datap    pointer to @p fxps8700_data_s structure
 *
 * @return              The operation status.
 * @retval OSAL_SUCCESS the operation succeeded.
 * @retval OSAL_FAILED  the operation failed.
 *
 * @api
 */
bool fxos8700ReadData(FXOS8700Driver* fxos8700p, fxos8700_data_s* datap)
{
    osalDbgCheck(fxos8700p != NULL);
    /* Verify device status. */
    osalDbgAssert(fxos8700p->state == FXOS8700_READY,
            "invalid state");

    fxos8700p->state = FXOS8700_ACTIVE;

    /* Read raw sensor data. */
    const uint8_t txbuf[] = { REG_OUT_X_MSB };
    uint8_t rxbuf[] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
    };

    msg_t result;
    result = i2cMasterTransmitTimeout(fxos8700p->configp->i2cp,
            fxos8700p->configp->i2c_address >> 1,
            txbuf,
            sizeof(txbuf),
            rxbuf,
            sizeof(rxbuf),
            fxos8700p->configp->i2c_timeout);

    fxos8700p->state = FXOS8700_READY;

    if (result != MSG_OK)
    {
        return OSAL_FAILED;
    }

    datap->accel_x = (rxbuf[0] << 8) | (rxbuf[1] << 0);
    datap->accel_y = (rxbuf[2] << 8) | (rxbuf[3] << 0);
    datap->accel_z = (rxbuf[4] << 8) | (rxbuf[5] << 0);

    datap->mag_x = (rxbuf[6] << 8) | (rxbuf[7] << 0);
    datap->mag_y = (rxbuf[8] << 8) | (rxbuf[9] << 0);
    datap->mag_z = (rxbuf[10] << 8) | (rxbuf[11] << 0);

    return OSAL_SUCCESS;
}

#endif /* HAL_USE_FXOS8700 */

/** @} */
