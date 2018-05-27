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
 * @file    qserial_fdx.c
 * @brief   Serial full duplex driver code.
 *
 * @addtogroup SERIAL_FDX
 * @{
 */

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

/**
 * @brief   Character escape function.
 * @details If required, function escapes a single character.
 *
 * @param[in] c         character to escape
 * @param[out] buffer   pointer to message buffer
 *
 * @return              byte count used to escape the character
 *
 * @notapi
 */
static uint8_t sfdxd_escape(uint8_t c, uint8_t* buffer)
{
    uint8_t idx = 0;
    if (c == SFDX_FRAME_BEGIN || c == SFDX_FRAME_END || c == SFDX_BYTE_ESC)
    {
        buffer[idx++] = SFDX_BYTE_ESC;
    }
    buffer[idx++] = c;
    return idx;
}

/**
 * @brief   Send function.
 * @details Called from pump thread function to send a frame.
 *
 * @param[in] sfdxdp    pointer to a @p SerialFdxDriver object
 *
 * @notapi
 */
static void sfdxd_send(SerialFdxDriver* sfdxdp)
{
    uint8_t idx = 0;
    sfdxdp->sendbuffer[idx++] = SFDX_FRAME_BEGIN;

    osalSysLock();
    while ((chSymQIsEmptyI(&sfdxdp->oqueue) == FALSE) &&
            (idx < (SERIAL_FDX_MTU - 3)))
    {
        osalSysUnlock();
        idx += sfdxd_escape((uint8_t)chSymQGet(&sfdxdp->oqueue),
                sfdxdp->sendbuffer + idx);
        osalSysLock();
    }
    osalSysUnlock();

    sfdxdp->sendbuffer[idx++] = SFDX_FRAME_END;
    streamWrite(sfdxdp->configp->farp, sfdxdp->sendbuffer, idx);

    osalSysLock();
    if ((sfdxdp->connected == TRUE) && (chSymQIsEmptyI(&sfdxdp->oqueue) == TRUE))
        chnAddFlagsI(sfdxdp, CHN_OUTPUT_EMPTY);
    osalOsRescheduleS();
    osalSysUnlock();
}

/**
 * @brief   Receive function.
 * @details Called from pump thread function to receive a frame.
 *
 * @param[in] sfdxdp    pointer to a @p SerialFdxDriver object
 * @param[in] timeout   timeout for waiting for new data
 *
 * @return              count of received bytes
 * @retval Q_TIMEOUT    Specified timeout expired.
 * @retval Q_RESET      Queue associated with the channel has been reset.
 *
 * @notapi
 */
static msg_t sfdxd_receive(SerialFdxDriver* sfdxdp, systime_t timeout)
{
    bool foundFrameBegin = FALSE;
    bool foundEsc = FALSE;
    msg_t byteCount = 0;
    msg_t c;
    while ((c = chnGetTimeout((BaseAsynchronousChannel*)sfdxdp->configp->farp,
            timeout)) >= 0)
    {
        if (c == SFDX_FRAME_BEGIN && foundFrameBegin == FALSE &&
                foundEsc == FALSE)
        {
            foundFrameBegin = TRUE;
        }
        else if (c == SFDX_FRAME_END &&
                foundFrameBegin == TRUE &&
                foundEsc == FALSE)
        {
            return byteCount;
        }
        else
        {
            if (c == SFDX_BYTE_ESC && foundEsc == FALSE)
            {
                foundEsc = TRUE;
            }
            else
            {
                if (sfdxdp->connected == TRUE)
                {
                    osalSysLock();
                    byteCount++;
                    if (chSymQIsEmptyI(&sfdxdp->iqueue) == TRUE)
                    {
                        chnAddFlagsI(sfdxdp, CHN_INPUT_AVAILABLE);
                    }
                    if (chSymQPutI(&sfdxdp->iqueue, (uint8_t)c) == Q_FULL)
                    {
                        chnAddFlagsI(sfdxdp, SFDX_OVERRUN_ERROR);
                    }
                    osalOsRescheduleS();
                    osalSysUnlock();
                }
                foundEsc = FALSE;
            }
        }
    }

    /* Raise an event if end of frame was not read */
    if (foundFrameBegin)
    {
        osalSysLock();
        chnAddFlagsI(sfdxdp, SFDX_FRAMING_ERROR);
        osalOsRescheduleS();
        osalSysUnlock();
    }
    return c;
}

/**
 * @brief   Drivers pump thread function.
 *
 * @param[in] parameters    pointer to a @p SerialFdxDriver object
 *
 * @notapi
 */
static void sfdxd_pump(void* parameters)
{
    SerialFdxDriver* sfdxdp = (SerialFdxDriver*)parameters;

#if defined(_CHIBIOS_RT_)
    chRegSetThreadName("sfdxd_pump");
#endif

    msg_t receiveResult = 0;
    while (true)
    {
        /* Nothing to do, going to sleep.*/
        osalSysLock();
        if (sfdxdp->state == SFDXD_STOP)
            chThdSuspendS(&sfdxdp->wait);
        osalSysUnlock();

        if (sfdxdp->configp->type == SFDXD_MASTER)
        {
            sfdxd_send(sfdxdp);
            receiveResult = sfdxd_receive(sfdxdp,
                    TIME_MS2I(SFDX_MASTER_RECEIVE_TIMEOUT_MS));
        }
        else
        {
            receiveResult = sfdxd_receive(sfdxdp,
                    TIME_MS2I(SFDX_SLAVE_RECEIVE_TIMEOUT_MS));
            if (receiveResult >= 0)
                sfdxd_send(sfdxdp);
        }

        /* Send connect or disconnect event. */
        if ((receiveResult >= 0) && (sfdxdp->connected == FALSE) &&
                (sfdxdp->state == SFDXD_READY))
        {
            osalSysLock();

            chSymQResetI(&sfdxdp->oqueue);
            chSymQResetI(&sfdxdp->iqueue);
            sfdxdp->connected = TRUE;
            chnAddFlagsI(sfdxdp, CHN_CONNECTED);

            osalOsRescheduleS();
            osalSysUnlock();
        }
        else if ((receiveResult == Q_TIMEOUT) &&
                (sfdxdp->connected == TRUE))
        {
            osalSysLock();

            sfdxdp->connected = FALSE;
            chnAddFlagsI(sfdxdp, CHN_DISCONNECTED);
            chSymQResetI(&sfdxdp->oqueue);
            chSymQResetI(&sfdxdp->iqueue);

            osalOsRescheduleS();
            osalSysUnlock();
        }
    }
}

/*
 * Interface implementation, the following functions just invoke the equivalent
 * queue-level function or macro.
 */
static msg_t putt(void* ip, uint8_t b, systime_t timeout)
{
    SerialFdxDriver* sfdxdp = (SerialFdxDriver*)ip;
    size_t result = chSymQPutTimeout(&sfdxdp->oqueue, b, timeout);
    return result;
}

static msg_t gett(void* ip, systime_t timeout)
{
    SerialFdxDriver* sfdxdp = (SerialFdxDriver*)ip;
    size_t result = chSymQGetTimeout(&sfdxdp->iqueue, timeout);
    return result;
}

static size_t writet(void* ip, const uint8_t* bp, size_t n, systime_t timeout)
{
    SerialFdxDriver* sfdxdp = (SerialFdxDriver*)ip;
    size_t result = chSymQWriteTimeout(&sfdxdp->oqueue, bp, n, TIME_INFINITE);
    return result;
}

static size_t readt(void* ip, uint8_t* bp, size_t n, systime_t timeout)
{
    SerialFdxDriver* sfdxdp = (SerialFdxDriver*)ip;
    size_t result = chSymQReadTimeout(&sfdxdp->iqueue, bp, n, timeout);
    return result;
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

static const struct SerialFdxDriverVMT vmt =
{
    (size_t)0,
    write, read, put, get,
    putt, gett, writet, readt
};

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Serial full duplex driver initialization.
 * @note    This function is implicitly invoked by @p qhalInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void sfdxdInit(void)
{
}

/**
 * @brief   Initializes a generic serial full duplex driver object.
 * @details The HW dependent part of the initialization has to be performed
 *          outside, usually in the hardware initialization code.
 *
 * @param[out] sfdxdp   pointer to a @p SerialFdxDriver structure
 *
 * @init
 */
void sfdxdObjectInit(SerialFdxDriver* sfdxdp)
{
    sfdxdp->vmt = &vmt;
    osalEventObjectInit(&sfdxdp->event);
    sfdxdp->state = SFDXD_STOP;
    sfdxdp->connected = FALSE;

    chSymQObjectInit(&sfdxdp->iqueue, sfdxdp->ib,
            sizeof(sfdxdp->ib));

    chSymQObjectInit(&sfdxdp->oqueue, sfdxdp->ob,
            sizeof(sfdxdp->ob));

#if defined(_CHIBIOS_RT_)
    sfdxdp->tr = NULL;
    sfdxdp->wait = NULL;

    /* Filling the thread working area here because the function
       @p chThdCreateI() does not do it.*/
#if CH_DBG_FILL_THREADS
    {
        void *wsp = sfdxdp->wa_pump;
        _thread_memfill((uint8_t *)wsp,
                (uint8_t *)wsp + sizeof(thread_t),
                CH_DBG_THREAD_FILL_VALUE);
        _thread_memfill((uint8_t *)wsp + sizeof(thread_t),
                (uint8_t *)wsp + sizeof(sfdxdp->wa_pump),
                CH_DBG_STACK_FILL_VALUE);
    }
#endif /* CH_DBG_FILL_THREADS */
#endif /* defined(_CHIBIOS_RT_) */
}

/**
 * @brief   Configures and starts the driver.
 *
 * @param[in] sfdxdp    pointer to a @p SerialFdxDriver object
 * @param[in] config    the architecture-dependent serial full duplex driver
 *                      configuration.
 *
 * @api
 */
void sfdxdStart(SerialFdxDriver* sfdxdp, const SerialFdxConfig *configp)
{
    osalDbgCheck(sfdxdp != NULL);

    osalSysLock();
    osalDbgAssert((sfdxdp->state == SFDXD_STOP) || (sfdxdp->state == SFDXD_READY),
            "invalid state");
    sfdxdp->configp = configp;
    sfdxdp->state = SFDXD_READY;
    sfdxdp->connected = FALSE;

#if defined(_CHIBIOS_RT_)
    /* Creates the data pump thread. Note, it is created only once.*/
    if (sfdxdp->tr == NULL)
    {
        sfdxdp->tr = chThdCreateI(sfdxdp->wa_pump, sizeof sfdxdp->wa_pump,
                SERIAL_FDX_THREAD_PRIO, sfdxd_pump, sfdxdp);
        chThdStartI(sfdxdp->tr);
    }
    else if (sfdxdp->wait != NULL)
    {
        chThdResumeS(&sfdxdp->wait, MSG_OK);
    }
    chSchRescheduleS();
#endif

    osalSysUnlock();
}

/**
 * @brief   Stops the driver.
 * @details Any thread waiting on the driver's queues will be awakened with
 *          the message @p Q_RESET.
 *
 * @param[in] sfdxdp    pointer to a @p SerialFdxDriver object
 *
 * @api
 */
void sfdxdStop(SerialFdxDriver* sfdxdp)
{
    osalDbgCheck(sfdxdp != NULL);

    osalSysLock();
    osalDbgAssert((sfdxdp->state == SFDXD_STOP) || (sfdxdp->state == SFDXD_READY),
            "invalid state");
    sfdxdp->state = SFDXD_STOP;

    if (sfdxdp->connected == TRUE)
        chnAddFlagsI(sfdxdp, CHN_DISCONNECTED);

    sfdxdp->connected = FALSE;

    chSymQResetI(&sfdxdp->oqueue);
    chSymQResetI(&sfdxdp->iqueue);
    osalOsRescheduleS();
    osalSysUnlock();
}
#endif /* HAL_USE_SERIAL_FDX */

/** @} */
