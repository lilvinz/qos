/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

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

    chSysLock();

    size_t w = 0;

    for (; w < n; ++w)
    {
        /* Try to write a byte to far queue. */
        msg_t result = chSymQPutTimeoutS(&svdp->configp->farp->queue, *bp++, TIME_INFINITE);
        if (result != Q_OK)
        {
            chSchRescheduleS();
            chSysUnlock();
            return w;
        }

        /* Check if far queue was empty and set flag. */
        if (chSymQSpaceI(&svdp->configp->farp->queue) == 1)
            chnAddFlagsI(svdp->configp->farp, CHN_INPUT_AVAILABLE);
    }

    chSchRescheduleS();
    chSysUnlock();

    return w;
}

static size_t read(void *ip, uint8_t *bp, size_t n)
{
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    chSysLock();

    size_t r = 0;

    for (; r < n; ++r)
    {
        /* Try to get a byte from near queue. */
        msg_t result = chSymQGetTimeoutS(&svdp->queue, TIME_INFINITE);
        if (result < Q_OK)
        {
            chSchRescheduleS();
            chSysUnlock();
            return r;
        }

        /* Store it to buffer if successful. */
        *bp++ = (uint8_t)result;

        /* Check if near queue is empty and set flags. */
        if (chSymQIsEmptyI(&svdp->queue) == TRUE)
            chnAddFlagsI(svdp->configp->farp, CHN_OUTPUT_EMPTY);
    }

    chSchRescheduleS();
    chSysUnlock();

    return r;
}

static msg_t put(void *ip, uint8_t b)
{
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    chSysLock();

    /* Try to write a byte to far queue. */
    msg_t result = chSymQPutTimeoutS(&svdp->configp->farp->queue, b, TIME_INFINITE);
    if (result != Q_OK)
    {
        chSchRescheduleS();
        chSysUnlock();
        return result;
    }

    /* Check if far queue was empty and set flag. */
    if (chSymQSpaceI(&svdp->configp->farp->queue) == 1)
        chnAddFlagsI(svdp->configp->farp, CHN_INPUT_AVAILABLE);

    chSchRescheduleS();
    chSysUnlock();

    return result;
}

static msg_t get(void *ip)
{
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    chSysLock();

    /* Try to get a byte from near queue. */
    msg_t result = chSymQGetTimeoutS(&svdp->queue, TIME_INFINITE);
    if (result < Q_OK)
    {
        chSchRescheduleS();
        chSysUnlock();
        return result;
    }

    /* Check if near queue is empty and set flags. */
    if (chSymQIsEmptyI(&svdp->queue) == TRUE)
        chnAddFlagsI(svdp->configp->farp, CHN_OUTPUT_EMPTY);

    chSchRescheduleS();
    chSysUnlock();

    return result;
}

static msg_t putt(void *ip, uint8_t b, systime_t timeout)
{
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    chSysLock();

    /* Try to write a byte to far queue. */
    msg_t result = chSymQPutTimeoutS(&svdp->configp->farp->queue, b, timeout);
    if (result != Q_OK)
    {
        chSchRescheduleS();
        chSysUnlock();
        return result;
    }

    /* Check if far queue was empty and set flag. */
    if (chSymQSpaceI(&svdp->configp->farp->queue) == 1)
        chnAddFlagsI(svdp->configp->farp, CHN_INPUT_AVAILABLE);

    chSchRescheduleS();
    chSysUnlock();

    return result;
}

static msg_t gett(void *ip, systime_t timeout)
{
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    chSysLock();

    /* Try to get a byte from near queue. */
    msg_t result = chSymQGetTimeoutS(&svdp->queue, timeout);
    if (result < Q_OK)
    {
        chSchRescheduleS();
        chSysUnlock();
        return result;
    }

    /* Check if near queue is empty and set flags. */
    if (chSymQIsEmptyI(&svdp->queue) == TRUE)
        chnAddFlagsI(svdp->configp->farp, CHN_OUTPUT_EMPTY);

    chSchRescheduleS();
    chSysUnlock();

    return result;
}

static size_t writet(void *ip, const uint8_t *bp, size_t n, systime_t timeout)
{
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    chSysLock();

    size_t w = 0;
    systime_t start = chTimeNowI();

    for (; w < n; ++w)
    {
        /* Calculate remaining timeout. */
        systime_t this_timeout = timeout;
        if (timeout != TIME_IMMEDIATE && timeout != TIME_INFINITE)
        {
            if (chTimeElapsedSinceI(start) >= timeout)
            {
                chSysUnlock();
                return w;
            }
            this_timeout = timeout - chTimeElapsedSinceI(start);
        }

        /* Try to write a byte to far queue. */
        msg_t result = chSymQPutTimeoutS(&svdp->configp->farp->queue, *bp++, this_timeout);
        if (result != Q_OK)
        {
            chSchRescheduleS();
            chSysUnlock();
            return w;
        }

        /* Check if far queue was empty and set flag. */
        if (chSymQSpaceI(&svdp->configp->farp->queue) == 1)
            chnAddFlagsI(svdp->configp->farp, CHN_INPUT_AVAILABLE);
    }

    chSchRescheduleS();
    chSysUnlock();

    return w;
}

static size_t readt(void *ip, uint8_t *bp, size_t n, systime_t timeout)
{
    SerialVirtualDriver *svdp = (SerialVirtualDriver*)ip;

    chSysLock();

    size_t r = 0;
    systime_t start = chTimeNowI();

    for (; r < n; ++r)
    {
        /* Calculate remaining timeout. */
        systime_t this_timeout = timeout;
        if (timeout != TIME_IMMEDIATE && timeout != TIME_INFINITE)
        {
            if (chTimeElapsedSinceI(start) >= timeout)
            {
                chSysUnlock();
                return r;
            }
            this_timeout = timeout - chTimeElapsedSinceI(start);
        }

        /* Try to get a byte from near queue. */
        msg_t result = chSymQGetTimeoutS(&svdp->queue, this_timeout);
        if (result < Q_OK)
        {
            chSchRescheduleS();
            chSysUnlock();
            return r;
        }

        /* Store it to buffer if successful. */
        *bp++ = (uint8_t)result;

        /* Check if near queue is empty and set flags. */
        if (chSymQIsEmptyI(&svdp->queue) == TRUE)
            chnAddFlagsI(svdp->configp->farp, CHN_OUTPUT_EMPTY);
    }

    chSchRescheduleS();
    chSysUnlock();

    return r;
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
    chSchRescheduleS();
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
