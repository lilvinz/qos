/**
 * @file    qserial_virtual.c
 * @brief   Virtual serial driver code.
 *
 * @addtogroup SERIAL_VIRTUAL
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_SERIAL_VIRTUAL || defined(__DOXYGEN__)

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
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    size_t result = chSymQWriteTimeout(&svdp->configp->farp->queue, bp, n, TIME_INFINITE);

    chSysLock();
    if (chSymQIsEmptyI(&svdp->configp->farp->queue) == FALSE)
        chnAddFlagsI(svdp->configp->farp, CHN_INPUT_AVAILABLE);
    chSysUnlock();

    return result;
}

static size_t read(void *ip, uint8_t *bp, size_t n)
{
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    size_t result = chSymQReadTimeout(&svdp->queue, bp, n, TIME_INFINITE);

    chSysLock();
    if (chSymQIsEmptyI(&svdp->queue) == TRUE)
        chnAddFlagsI(svdp->configp->farp, CHN_OUTPUT_EMPTY);
    chSysUnlock();

    return result;
}

static msg_t put(void *ip, uint8_t b)
{
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    size_t result = chSymQPutTimeout(&svdp->configp->farp->queue, b, TIME_INFINITE);

    chSysLock();
    if (chSymQIsEmptyI(&svdp->configp->farp->queue) == FALSE)
        chnAddFlagsI(svdp->configp->farp, CHN_INPUT_AVAILABLE);
    chSysUnlock();

    return result;
}

static msg_t get(void *ip)
{
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    size_t result = chSymQGetTimeout(&svdp->queue, TIME_INFINITE);

    chSysLock();
    if (chSymQIsEmptyI(&svdp->queue) == TRUE)
        chnAddFlagsI(svdp->configp->farp, CHN_OUTPUT_EMPTY);
    chSysUnlock();

    return result;
}

static msg_t putt(void *ip, uint8_t b, systime_t timeout)
{
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    size_t result = chSymQPutTimeout(&svdp->configp->farp->queue, b, timeout);

    chSysLock();
    if (chSymQIsEmptyI(&svdp->configp->farp->queue) == FALSE)
        chnAddFlagsI(svdp->configp->farp, CHN_INPUT_AVAILABLE);
    chSysUnlock();

    return result;
}

static msg_t gett(void *ip, systime_t timeout)
{
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    size_t result = chSymQGetTimeout(&svdp->queue, timeout);

    chSysLock();
    if (chSymQIsEmptyI(&svdp->queue) == TRUE)
        chnAddFlagsI(svdp->configp->farp, CHN_OUTPUT_EMPTY);
    chSysUnlock();

    return result;
}

static size_t writet(void *ip, const uint8_t *bp, size_t n, systime_t time)
{
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    size_t result = chSymQWriteTimeout(&svdp->configp->farp->queue, bp, n, time);

    chSysLock();
    if (chSymQIsEmptyI(&svdp->configp->farp->queue) == FALSE)
        chnAddFlagsI(svdp->configp->farp, CHN_INPUT_AVAILABLE);
    chSysUnlock();

    return result;
}

static size_t readt(void *ip, uint8_t *bp, size_t n, systime_t time)
{
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    size_t result = chSymQReadTimeout(&svdp->queue, bp, n, time);

    chSysLock();
    if (chSymQIsEmptyI(&svdp->queue) == TRUE)
        chnAddFlagsI(svdp->configp->farp, CHN_OUTPUT_EMPTY);
    chSysUnlock();

    return result;
}

static const struct SerialVirtualDriverVMT vmt =
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
void sdvirtualInit(void)
{
}

/**
 * @brief   Initializes a generic full duplex driver object.
 * @details The HW dependent part of the initialization has to be performed
 *          outside, usually in the hardware initialization code.
 *
 * @param[out] sdvirtualp   pointer to a @p SerialVirtualDriver structure
 *
 * @init
 */
void sdvirtualObjectInit(SerialVirtualDriver *sdvirtualp)
{
    sdvirtualp->vmt = &vmt;
    chEvtInit(&sdvirtualp->event);
    sdvirtualp->state = SDVIRTUAL_STOP;
    chSymQInit(&sdvirtualp->queue, sdvirtualp->queuebuf,
            sizeof(sdvirtualp->queuebuf));
}

/**
 * @brief   Configures and starts the driver.
 *
 * @param[in] sdvirtualp    pointer to a @p SerialVirtualDriver object
 * @param[in] config        the architecture-dependent serial driver configuration.
 *                          If this parameter is set to @p NULL then a default
 *                          configuration is used.
 *
 * @api
 */
void sdvirtualStart(SerialVirtualDriver *sdvirtualp, const SerialVirtualConfig *configp)
{
    chDbgCheck(sdvirtualp != NULL, "sdvirtualStart");

    chSysLock();
    chDbgAssert((sdvirtualp->state == SDVIRTUAL_STOP) || (sdvirtualp->state == SDVIRTUAL_READY),
            "sdvirtualStart(), #1",
            "invalid state");
    sdvirtualp->configp = configp;
    sdvirtualp->state = SDVIRTUAL_READY;
    chnAddFlagsI(sdvirtualp, CHN_CONNECTED);
    chSysUnlock();
}

/**
 * @brief   Stops the driver.
 * @details Any thread waiting on the driver's queues will be awakened with
 *          the message @p Q_RESET.
 *
 * @param[in] sdvirtualp    pointer to a @p SerialVirtualDriver object
 *
 * @api
 */
void sdvirtualStop(SerialVirtualDriver *sdvirtualp)
{
    chDbgCheck(sdvirtualp != NULL, "sdvirtualStop");

    chSysLock();
    chDbgAssert((sdvirtualp->state == SDVIRTUAL_STOP) || (sdvirtualp->state == SDVIRTUAL_READY),
                "sdvirtualStop(), #1",
                "invalid state");
    chnAddFlagsI(sdvirtualp, CHN_DISCONNECTED);
    chSymQResetI(&sdvirtualp->queue);
    chSchRescheduleS();
    chSysUnlock();
}

#endif /* HAL_USE_SERIAL_VIRTUAL */

/** @} */
