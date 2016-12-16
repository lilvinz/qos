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
 * @addtogroup SVINFO
 * @{
 */

#include "hal.h"
#include "qhal.h"
#include "qhalconf.h"

#if (HAL_USE_QSERIALSOFT == TRUE) || defined(__DOXYGEN__)

#include <string.h>
#include <limits.h>

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/
/**
 * @brief     Local function declarations.
 */
#if QSERIALSOFT_USE_TRANSMITTER
static void qserialsoft_write_bit_cb(GPTDriver *gptd);
#endif /* QSERIALSOFT_USE_TRANSMITTER */
#if QSERIALSOFT_USE_RECEIVER
static void qserialsoft_start_bit_cb(EXTDriver* extp, expchannel_t channel);
static void qserialsoft_read_bit_cb(GPTDriver *gptd);
#endif /* QSERIALSOFT_USE_RECEIVER */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/
/**
 * @brief qserialsoft driver identifier.
 */
qserialsoftDriver QSERIALSOFTD1;

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

/* Function to get parity of number n. It returns 1
   if n has odd parity, and returns 0 if n has even
   parity */
static bool getParity(uint16_t number)
{
    bool parity = 0;
    while (number)
    {
        parity = !parity;
        number = number & (number - 1);
    }
    return parity;
}

#if QSERIALSOFT_USE_TRANSMITTER
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
    chDbgCheck(pwmp != NULL, "qserialsoft_write_bit_cb");

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
#endif /* QSERIALSOFT_USE_TRANSMITTER */

#if QSERIALSOFT_USE_RECEIVER
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
    gptStartOneShotI(QSERIALSOFTD1.config->gptd, (QSERIALSOFTD1.config->gptd->clock / QSERIALSOFTD1.config->speed) * 4 / 3);
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
        gptStartContinuousI(QSERIALSOFTD1.config->gptd, QSERIALSOFTD1.config->gptd->clock / QSERIALSOFTD1.config->speed);

    bool bitstate = (palReadPad(QSERIALSOFTD1.config->rx_port, QSERIALSOFTD1.config->rx_pad) == PAL_HIGH);
    bitstate ^= QSERIALSOFTD1.config->rx_invert;

    ++QSERIALSOFTD1.ibit;

    if (QSERIALSOFTD1.ibit <= QSERIALSOFTD1.config->databits)
    {
        /* buffer data bits */
        QSERIALSOFTD1.ibyte |= bitstate << (QSERIALSOFTD1.ibit - 1);
    }
    else
    {
        bool check_parity = false;

        if (QSERIALSOFTD1.ibit == QSERIALSOFTD1.config->databits + 1)
        {
            /* 1st bit after data bits */
            if (QSERIALSOFTD1.config->parity != 0)
            {
                check_parity = true;
            }
        }

        if (check_parity == true)
        {
            /* check parity */
            if ((getParity(QSERIALSOFTD1.ibyte) ^ bitstate) != (QSERIALSOFTD1.config->parity - 1))
            {
                chnAddFlagsI(&QSERIALSOFTD1, QSERIALSOFT_PARITY_ERROR);
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
                    chnAddFlagsI(&QSERIALSOFTD1, QSERIALSOFT_OVERRUN_ERROR);
                }
            }
            else
            {
                /* stop bit not ok */
                chnAddFlagsI(&QSERIALSOFTD1, QSERIALSOFT_FRAMING_ERROR);
            }

            /* prepare for reception of next frame */
            QSERIALSOFTD1.ibit = 0;
            QSERIALSOFTD1.ibyte = 0;

            extChannelEnableI(QSERIALSOFTD1.config->extd, QSERIALSOFTD1.config->rx_pad);
        }
    }

    chSysUnlockFromIsr();
}

#endif /* QSERIALSOFT_USE_RECEIVER */

/*
 * Interface implementation, the following functions just invoke the equivalent
 * queue-level function or macro.
 */
static msg_t putt(void* ip, uint8_t b, systime_t timeout)
{
#if QSERIALSOFT_USE_TRANSMITTER
    qserialsoftDriver* qserialsoftdp = (qserialsoftDriver*) ip;
    return chOQPutTimeout(&qserialsoftdp->oqueue, b, timeout);
#else
    return Q_TIMEOUT;
#endif
}

static msg_t gett(void* ip, systime_t timeout)
{
#if QSERIALSOFT_USE_RECEIVER
    qserialsoftDriver* qserialsoftdp = (qserialsoftDriver*) ip;
    return chIQGetTimeout(&qserialsoftdp->iqueue, timeout);
#else
    return Q_TIMEOUT;
#endif
}

static size_t writet(void* ip, const uint8_t* bp, size_t n, systime_t timeout)
{
#if QSERIALSOFT_USE_TRANSMITTER
    qserialsoftDriver* qserialsoftdp = (qserialsoftDriver*) ip;
    size_t result = chOQWriteTimeout(&qserialsoftdp->oqueue, bp, n, TIME_INFINITE);
    return result;
#else
    return Q_TIMEOUT;
#endif
}

static size_t readt(void* ip, uint8_t* bp, size_t n, systime_t timeout)
{
#if QSERIALSOFT_USE_RECEIVER
    qserialsoftDriver* qserialsoftdp = (qserialsoftDriver*) ip;
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

static const struct QSerialSoftDriverVMT vmt =
{
    write, read, put, get,
    putt, gett, writet, readt
};

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes @p qserialsoftDriver structure.
 *
 * @param[out] qsvip    pointer to the @p qserialsoftDriver object
 *
 * @init
 */
void qserialsoftObjectInit(qserialsoftDriver *qsvip)
{
    chDbgCheck(NULL != qsvip, "qserialsoftObjectInit");

    qsvip->vmt = &vmt;
    chEvtInit(&qsvip->event);

    qsvip->config = NULL;
    qsvip->state = QSERIALSOFT_STOP;

#if QSERIALSOFT_USE_TRANSMITTER
    chOQInit(&qsvip->oqueue, qsvip->ob, sizeof(qsvip->ob), NULL, qsvip);
    qsvip->obit = 0;
#endif

#if QSERIALSOFT_USE_RECEIVER
    chIQInit(&qsvip->iqueue, qsvip->ib, sizeof(qsvip->ib), NULL, qsvip);
    qsvip->ibit = 0;
#endif
}

/**
 * @brief   Configures and activates the qserialsoft driver.
 *
 * @param[in] qsvip     pointer to the @p qserialsoftDriver object
 * @param[in] config    pointer to the @p qserialsoftConfig object
 *
 * @api
 */
void qserialsoftStart(qserialsoftDriver *qsvip, qserialsoftConfig *config)
{
    chDbgCheck((NULL != qsvip) && (NULL != config), "qserialsoftStart");

    chSysLock();
    chDbgAssert((qsvip->state == QSERIALSOFT_STOP) || (qsvip->state == QSERIALSOFT_READY),
            "qserialsoftStart(), #1",
            "invalid state");

    chDbgAssert(GPT_STOP == config->gptd->state,
    "qserialsoftStart(), #2", "GPT will be started by qserialsoft driver internally.");

    qsvip->config = config;

    qsvip->state = QSERIALSOFT_READY;

    chSysUnlock();

#if QSERIALSOFT_USE_TRANSMITTER
    /* ToDo: Setup tx environment */

#endif

#if QSERIALSOFT_USE_RECEIVER
    /* start GPT driver to obtain gpt clock frequency */
    qsvip->config->gptcfg->frequency = 10000;
    qsvip->config->gptcfg->callback = qserialsoft_read_bit_cb;
    gptStart(qsvip->config->gptd, qsvip->config->gptcfg);
    gptStop(qsvip->config->gptd);

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
}

/**
 * @brief   Deactivates the qserialsoft driver.
 *
 * @param[in] qsvip     pointer to the @p qserialsoftDriver object
 *
 * @api
 */
void qserialsoftStop(qserialsoftDriver *qsvip)
{
    chDbgCheck(NULL != qsvip, "qserialsoftStop");

#if QSERIALSOFT_USE_TRANSMITTER
    /* ToDo: Stop tx environment */
#endif
#if QSERIALSOFT_USE_RECEIVER
    extChannelDisable(qsvip->config->extd, qsvip->config->rx_pad);
    gptStopTimer(qsvip->config->gptd);
    gptStop(qsvip->config->gptd);
#endif

    chSysLock();
    chDbgAssert((qsvip->state == QSERIALSOFT_STOP) || (qsvip->state == QSERIALSOFT_READY),
            "qserialsoftStop(), #1",
            "invalid state");
    qsvip->state = QSERIALSOFT_STOP;
    qsvip->config = NULL;

#if QSERIALSOFT_USE_RECEIVER
    chIQResetI(&qsvip->iqueue);
#endif

#if QSERIALSOFT_USE_TRANSMITTER
    chOQResetI(&qsvip->oqueue);
#endif

    chSysUnlock();
}

#endif /* HAL_USE_QSERIALSOFT */

/** @} */
