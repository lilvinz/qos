/*
 * qserialsoft.c
 *
 *  Created on: 01.09.2016
 *      Author: cba
 */

/**
 * @file    qserialsoft.c
 * @brief   QSerialSoft Driver code.
 *
 * @addtogroup SERIAL_SOFT
 * @{
 */

#include "hal.h"
#include "qhal.h"
#include "qhalconf.h"

#if (HAL_USE_SERIAL_SOFT == TRUE) || defined(__DOXYGEN__)

#include <string.h>
#include <limits.h>

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/
/**
 * @brief     Local function declarations.
 */
#if SERIALSOFT_USE_TRANSMITTER
static void qserialsoft_write_bit_cb(GPTDriver *gptd);
#endif /* SERIALSOFT_USE_TRANSMITTER */
#if SERIALSOFT_USE_RECEIVER
static void qserialsoft_start_bit_cb(EXTDriver* extp, expchannel_t channel);
static void qserialsoft_read_bit_cb(GPTDriver *gptd);
#endif /* SERIALSOFT_USE_RECEIVER */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/
/**
 * @brief qserialsoft driver identifier.
 */
SerialSoftDriver QSERIALSOFTD1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief     Function to get parity of number
 * @note      Must be called from PWM's ISR.
 *
 * @param[in] number    input of which the parity should be calculated
 * @return              return 1 on odd parity, 0 on even
   parity
 *
 * @notapi
 */
static bool getParity(uint16_t number)
{
    bool parity = false;
    while (number)
    {
        parity = !parity;
        number = number & (number - 1);
    }
    return parity;
}

#if SERIALSOFT_USE_TRANSMITTER
/**
 * @brief     qserialsoft bit transmission callback.
 * @note      Must be called from GPT's ISR.
 *
 * @param[in] gptd      pointer to the @p GPTDriver object
 *
 * @notapi
 */
static void qserialsoft_write_bit_cb(GPTDriver *gptd)
{
    chDbgCheck(gptd != NULL, "qserialsoft_write_bit_cb");

    chSysLockFromIsr();
    if (QSERIALSOFTD1.obit == 8)
    {
        if (chOQIsEmptyI(&QSERIALSOFTD1.oqueue) == FALSE)
        {
            QSERIALSOFTD1.obyte = chOQGetI(&QSERIALSOFTD1.oqueue);
            QSERIALSOFTD1.obit = 0;
        }
        else
        {
            /* Done */
            chSysUnlockFromIsr();
            return;
        }
    }

    /* ToDo: Send bit */

    QSERIALSOFTD1.obit++;

    chSysUnlockFromIsr();
}
#endif /* SERIALSOFT_USE_TRANSMITTER */

#if SERIALSOFT_USE_RECEIVER
/**
 * @brief     qserialsoft start bit callback.
 * @note      Must be called from EXT's ISR.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 * @param[in] channel   channel number
 *
 * @notapi
 */
static void qserialsoft_start_bit_cb(EXTDriver* extp, expchannel_t channel)
{
    chDbgCheck(extp != NULL, "qserialsoft_start_bit_cb");

    chSysLockFromIsr();
    extChannelDisableI(QSERIALSOFTD1.config->extd, QSERIALSOFTD1.config->rx_pad);
    gptStartOneShotI(QSERIALSOFTD1.config->gptd, QSERIALSOFTD1.timing_first_bit);
    chSysUnlockFromIsr();
}

/**
 * @brief     qserialsoft read bit callback.
 * @note      Must be called from GPT's ISR.
 *
 * @param[in] gptd      pointer to the @p GPTDriver object
 *
 * @notapi
 */
static void qserialsoft_read_bit_cb(GPTDriver *gptd)
{
    chDbgCheck(gptd != NULL, "qserialsoft_read_bit_cb");

    chSysLockFromIsr();

    if (QSERIALSOFTD1.ibit == 0)
        gptStartContinuousI(QSERIALSOFTD1.config->gptd, QSERIALSOFTD1.timing_bit_interval);

    bool bitstate = palReadPad(QSERIALSOFTD1.config->rx_port, QSERIALSOFTD1.config->rx_pad) == PAL_HIGH;
    bitstate ^= QSERIALSOFTD1.config->rx_invert;

    if (QSERIALSOFTD1.ibit < QSERIALSOFTD1.config->databits)
    {
        /* buffer data bits */
        QSERIALSOFTD1.ibyte |= bitstate << QSERIALSOFTD1.ibit;
    }
    else
    {
        if (QSERIALSOFTD1.ibit == QSERIALSOFTD1.config->databits && QSERIALSOFTD1.config->parity != 0)
        {
            /* check parity */
            if ((getParity(QSERIALSOFTD1.ibyte) ^ bitstate) != (QSERIALSOFTD1.config->parity - 1))
            {
                chnAddFlagsI(&QSERIALSOFTD1, SERIALSOFT_PARITY_ERROR);
            }
        }
        else
        {
            /* reception complete */
            gptStopTimerI(QSERIALSOFTD1.config->gptd);

            /* check stop bit */
            if (bitstate == 1)
            {
                /* stop bit ok, push received byte to queue */
                if (chIQIsEmptyI(&QSERIALSOFTD1.iqueue) == TRUE)
                {
                    chnAddFlagsI(&QSERIALSOFTD1, CHN_INPUT_AVAILABLE);
                }
                if (chIQPutI(&QSERIALSOFTD1.iqueue, QSERIALSOFTD1.ibyte) == Q_FULL)
                {
                    chnAddFlagsI(&QSERIALSOFTD1, SERIALSOFT_OVERRUN_ERROR);
                }
            }
            else
            {
                /* stop bit not ok */
                chnAddFlagsI(&QSERIALSOFTD1, SERIALSOFT_FRAMING_ERROR);
            }

            /* prepare for reception of next frame */
            QSERIALSOFTD1.ibit = 0;
            QSERIALSOFTD1.ibyte = 0;

            extChannelEnableI(QSERIALSOFTD1.config->extd, QSERIALSOFTD1.config->rx_pad);

            chSysUnlockFromIsr();
            return;
        }
    }

    ++QSERIALSOFTD1.ibit;

    chSysUnlockFromIsr();
}

#endif /* SERIALSOFT_USE_RECEIVER */

/*
 * Interface implementation, the following functions just invoke the equivalent
 * queue-level function or macro.
 */
static msg_t putt(void* ip, uint8_t b, systime_t timeout)
{
#if SERIALSOFT_USE_TRANSMITTER
    SerialSoftDriver* qserialsoftdp = (SerialSoftDriver*) ip;
    return chOQPutTimeout(&qserialsoftdp->oqueue, b, timeout);
#else
    return Q_TIMEOUT;
#endif
}

static msg_t gett(void* ip, systime_t timeout)
{
#if SERIALSOFT_USE_RECEIVER
    SerialSoftDriver* qserialsoftdp = (SerialSoftDriver*) ip;
    return chIQGetTimeout(&qserialsoftdp->iqueue, timeout);
#else
    return Q_TIMEOUT;
#endif
}

static size_t writet(void* ip, const uint8_t* bp, size_t n, systime_t timeout)
{
#if SERIALSOFT_USE_TRANSMITTER
    SerialSoftDriver* qserialsoftdp = (SerialSoftDriver*) ip;
    size_t result = chOQWriteTimeout(&qserialsoftdp->oqueue, bp, n, TIME_INFINITE);
    return result;
#else
    return Q_TIMEOUT;
#endif
}

static size_t readt(void* ip, uint8_t* bp, size_t n, systime_t timeout)
{
#if SERIALSOFT_USE_RECEIVER
    SerialSoftDriver* qserialsoftdp = (SerialSoftDriver*) ip;
    size_t result = chIQReadTimeout(&qserialsoftdp->iqueue, bp, n, timeout);
    return result;
#else
    return Q_TIMEOUT;
#endif
}

static size_t write(void* ip, const uint8_t* bp, size_t n)
{
    return writet(ip, bp, n, TIME_INFINITE);
}

static size_t read(void* ip, uint8_t* bp, size_t n)
{
    return readt(ip, bp, n, TIME_INFINITE);
}

static msg_t put(void* ip, uint8_t b)
{
    return putt(ip, b, TIME_INFINITE);
}

static msg_t get(void* ip)
{
    return gett(ip, TIME_INFINITE);
}

static const struct SerialSoftDriverVMT vmt =
{
    write, read, put, get,
    putt, gett, writet, readt
};

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes Driver.
 *
 * @init
 */
void serialsoftInit(void)
{

}

/**
 * @brief   Initializes @p SerialSoftDriver structure.
 *
 * @param[out] qsvip    pointer to the @p SerialSoftDriver object
 *
 * @init
 */
void serialsoftObjectInit(SerialSoftDriver *qsvip)
{
    chDbgCheck(NULL != qsvip, "serialsoftObjectInit");

    qsvip->vmt = &vmt;
    chEvtInit(&qsvip->event);

    qsvip->config = NULL;
    qsvip->state = SERIALSOFT_STOP;

#if SERIALSOFT_USE_TRANSMITTER
    chOQInit(&qsvip->oqueue, qsvip->ob, sizeof(qsvip->ob), NULL, qsvip);
    qsvip->obit = 0;
#endif

#if SERIALSOFT_USE_RECEIVER
    chIQInit(&qsvip->iqueue, qsvip->ib, sizeof(qsvip->ib), NULL, qsvip);
    qsvip->ibit = 0;
#endif
}

/**
 * @brief   Configures and activates the qserialsoft driver.
 *
 * @param[in] qsvip     pointer to the @p SerialSoftDriver object
 * @param[in] config    pointer to the @p SerialSoftConfig object
 *
 * @api
 */
void serialsoftStart(SerialSoftDriver *qsvip, SerialSoftConfig *config)
{
    chDbgCheck((NULL != qsvip) && (NULL != config), "serialsoftStart");

    chSysLock();
    chDbgAssert((qsvip->state == SERIALSOFT_STOP) || (qsvip->state == SERIALSOFT_READY),
            "serialsoftStart(), #1",
            "invalid state");

    chDbgAssert(config->databits == 7 || config->databits == 8,
            "serialsoftStart(), #2", "Invalid data bit count.");
    chDbgAssert(config->parity < 2,
            "serialsoftStart(), #2", "Invalid parity mode.");
    chDbgAssert(config->stopbits <= 2,
            "serialsoftStart(), #2", "Invalid stop bit count.");

    qsvip->config = config;

    chSysUnlock();

#if SERIALSOFT_USE_TRANSMITTER
    /* ToDo: Setup tx environment */
#endif

#if SERIALSOFT_USE_RECEIVER
    chDbgAssert((config->extd != NULL) && (config->extcfg != NULL),
            "serialsoftStart(), #3",
            "ext driver and configuration missing");
    chDbgAssert((config->gptd != NULL) && (config->gptcfg != NULL),
            "serialsoftStart(), #3",
            "gpt driver and configuration missing");
    chDbgAssert(config->gptd->state == GPT_STOP,
            "serialsoftStart(), #3", "GPT will be started by qserialsoft driver internally.");

    /* start GPT driver to obtain gpt clock frequency */
    qsvip->config->gptcfg->frequency = 10000;
    qsvip->config->gptcfg->callback = qserialsoft_read_bit_cb;
    gptStart(qsvip->config->gptd, qsvip->config->gptcfg);
    gptStop(qsvip->config->gptd);

    /* ToDo: Check config->speed is possible with this gpt clock frequency */

    /* start GPT again with maximum clock speed */
    qsvip->config->gptcfg->frequency = qsvip->config->gptd->clock;
    gptStart(qsvip->config->gptd, qsvip->config->gptcfg);

    /* setup and enable start bit edge detection interrupt */
    if (qsvip->config->rx_invert == TRUE)
        qsvip->config->extcfg->channels[qsvip->config->rx_pad].mode = qsvip->config->rx_pad | EXT_CH_MODE_RISING_EDGE;
    else
        qsvip->config->extcfg->channels[qsvip->config->rx_pad].mode = qsvip->config->rx_pad | EXT_CH_MODE_FALLING_EDGE;

    qsvip->config->extcfg->channels[qsvip->config->rx_pad].cb = qserialsoft_start_bit_cb;
    extChannelEnable(qsvip->config->extd, qsvip->config->rx_pad);
#endif

    /* pre-calculate timer value for bit interval */
    qsvip->timing_bit_interval = qsvip->config->gptd->clock / qsvip->config->speed;

    /* pre-calculate timer value for center of 1st data bit after leading edge of start bit */
    qsvip->timing_first_bit = qsvip->timing_bit_interval * 4 / 3;

    qsvip->state = SERIALSOFT_READY;
}

/**
 * @brief   Deactivates the qserialsoft driver.
 *
 * @param[in] qsvip     pointer to the @p SerialSoftDriver object
 *
 * @api
 */
void serialsoftStop(SerialSoftDriver *qsvip)
{
    chDbgCheck(NULL != qsvip, "serialsoftStop");

    chSysLock();
    chDbgAssert((qsvip->state == SERIALSOFT_STOP) || (qsvip->state == SERIALSOFT_READY),
            "serialsoftStop(), #1",
            "invalid state");
    qsvip->state = SERIALSOFT_STOP;

#if SERIALSOFT_USE_TRANSMITTER
    /* ToDo: Stop tx environment */
#endif
#if SERIALSOFT_USE_RECEIVER
    extChannelDisableI(qsvip->config->extd, qsvip->config->rx_pad);
    gptStopTimerI(qsvip->config->gptd);
#endif

#if SERIALSOFT_USE_RECEIVER
    chIQResetI(&qsvip->iqueue);
#endif

#if SERIALSOFT_USE_TRANSMITTER
    chOQResetI(&qsvip->oqueue);
#endif
    chSysUnlock();

#if SERIALSOFT_USE_RECEIVER
    gptStop(qsvip->config->gptd);
#endif

    qsvip->config = NULL;
}

#endif /* HAL_USE_SERIAL_SOFT */

/** @} */
