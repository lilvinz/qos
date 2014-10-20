/**
 * @file    qserial_fdx.c
 * @brief   Virtual serial driver code.
 *
 * @addtogroup SERIAL_FDX
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_SERIAL_FDX || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*
 * Interface implementation, the following functions just invoke the equivalent
 * queue-level function or macro.
 */

static size_t write(void *ip, const uint8_t *bp, size_t n)
{
    SerialFdxDriver *sfdp = (SerialFdxDriver*)ip;

    size_t result = chSymQWriteTimeout(&sfdp->configp->farp->queue, bp, n, TIME_INFINITE);

    chSysLock();
    if (chSymQIsEmptyI(&sfdp->configp->farp->queue) == FALSE)
        chnAddFlagsI(sfdp->configp->farp, CHN_INPUT_AVAILABLE);
    chSysUnlock();

    return result;
}

static size_t read(void *ip, uint8_t *bp, size_t n)
{
    SerialFdxDriver *sfdp = (SerialFdxDriver*)ip;

    size_t result = chSymQReadTimeout(&sfdp->queue, bp, n, TIME_INFINITE);

    chSysLock();
    if (chSymQIsEmptyI(&sfdp->queue) == TRUE)
        chnAddFlagsI(sfdp->configp->farp, CHN_OUTPUT_EMPTY);
    chSysUnlock();

    return result;
}

static msg_t put(void *ip, uint8_t b)
{
    SerialFdxDriver *sfdp = (SerialFdxDriver*)ip;

    size_t result = chSymQPutTimeout(&sfdp->configp->farp->queue, b, TIME_INFINITE);

    chSysLock();
    if (chSymQIsEmptyI(&sfdp->configp->farp->queue) == FALSE)
        chnAddFlagsI(sfdp->configp->farp, CHN_INPUT_AVAILABLE);
    chSysUnlock();

    return result;
}

static msg_t get(void *ip)
{
    SerialFdxDriver *sfdp = (SerialFdxDriver*)ip;

    size_t result = chSymQGetTimeout(&sfdp->queue, TIME_INFINITE);

    chSysLock();
    if (chSymQIsEmptyI(&sfdp->queue) == TRUE)
        chnAddFlagsI(sfdp->configp->farp, CHN_OUTPUT_EMPTY);
    chSysUnlock();

    return result;
}

static msg_t putt(void *ip, uint8_t b, systime_t timeout)
{
    SerialFdxDriver *sfdp = (SerialFdxDriver*)ip;

    size_t result = chSymQPutTimeout(&sfdp->configp->farp->queue, b, timeout);

    chSysLock();
    if (chSymQIsEmptyI(&sfdp->configp->farp->queue) == FALSE)
        chnAddFlagsI(sfdp->configp->farp, CHN_INPUT_AVAILABLE);
    chSysUnlock();

    return result;
}

static msg_t gett(void *ip, systime_t timeout)
{
    SerialFdxDriver *sfdp = (SerialFdxDriver*)ip;

    size_t result = chSymQGetTimeout(&sfdp->queue, timeout);

    chSysLock();
    if (chSymQIsEmptyI(&sfdp->queue) == TRUE)
        chnAddFlagsI(sfdp->configp->farp, CHN_OUTPUT_EMPTY);
    chSysUnlock();

    return result;
}

static size_t writet(void *ip, const uint8_t *bp, size_t n, systime_t time)
{
    SerialFdxDriver *sfdp = (SerialFdxDriver*)ip;

    size_t result = chSymQWriteTimeout(&sfdp->configp->farp->queue, bp, n, time);

    chSysLock();
    if (chSymQIsEmptyI(&sfdp->configp->farp->queue) == FALSE)
        chnAddFlagsI(sfdp->configp->farp, CHN_INPUT_AVAILABLE);
    chSysUnlock();

    return result;
}

static size_t readt(void *ip, uint8_t *bp, size_t n, systime_t time)
{
    SerialFdxDriver *sfdp = (SerialFdxDriver*)ip;

    size_t result = chSymQReadTimeout(&sfdp->queue, bp, n, time);

    chSysLock();
    if (chSymQIsEmptyI(&sfdp->queue) == TRUE)
        chnAddFlagsI(sfdp->configp->farp, CHN_OUTPUT_EMPTY);
    chSysUnlock();

    return result;
}

static const struct SerialFdxDriverVMT vmt =
{
    write, read, put, get,
    putt, gett, writet, readt
};

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Virtual serial driver initialization.
 * @note    This function is implicitly invoked by @p qhalInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void sdfdxInit(void)
{
}

/**
 * @brief   Initializes a generic full duplex driver object.
 * @details The HW dependent part of the initialization has to be performed
 *          outside, usually in the hardware initialization code.
 *
 * @param[out] sdfdxp   pointer to a @p SerialFdxDriver structure
 *
 * @init
 */
void sdfdxObjectInit(SerialFdxDriver *sdfdxp)
{
    sdfdxp->vmt = &vmt;
    chEvtInit(&sdfdxp->event);
    sdfdxp->state = SDFDX_STOP;
    chSymQInit(&sdfdxp->queue, sdfdxp->queuebuf,
            sizeof(sdfdxp->queuebuf));
}

/**
 * @brief   Configures and starts the driver.
 *
 * @param[in] sdfdxp    pointer to a @p SerialFdxDriver object
 * @param[in] config        the architecture-dependent serial driver configuration.
 *                          If this parameter is set to @p NULL then a default
 *                          configuration is used.
 *
 * @api
 */
void sdfdxStart(SerialFdxDriver *sdfdxp, const SerialFdxConfig *configp)
{
    chDbgCheck(sdfdxp != NULL, "sdfdxStart");

    chSysLock();
    chDbgAssert((sdfdxp->state == SDFDX_STOP) || (sdfdxp->state == SDFDX_STOP),
            "sdfdxStart(), #1",
            "invalid state");
    sdfdxp->configp = configp;
    sdfdxp->state = SDFDX_READY;
    chnAddFlagsI(sdfdxp, CHN_CONNECTED);
    chSysUnlock();
}

/**
 * @brief   Stops the driver.
 * @details Any thread waiting on the driver's queues will be awakened with
 *          the message @p Q_RESET.
 *
 * @param[in] sdfdxp    pointer to a @p SerialFdxDriver object
 *
 * @api
 */
void sdfdxStop(SerialFdxDriver *sdfdxp)
{
    chDbgCheck(sdfdxp != NULL, "sdfdxStop");

    chSysLock();
    chDbgAssert((sdfdxp->state == SDFDX_STOP) || (sdfdxp->state == SDFDX_READY),
                "sdfdxStop(), #1",
                "invalid state");
    chnAddFlagsI(sdfdxp, CHN_DISCONNECTED);
    chSymQResetI(&sdfdxp->queue);
    chSchRescheduleS();
    chSysUnlock();
}

#endif /* HAL_USE_SERIAL_FDX */

/** @} */
