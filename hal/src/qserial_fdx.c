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

    chSysLock();
    while ((chSymQIsEmptyI(&sfdxdp->oqueue) == FALSE) &&
            (idx < (SERIAL_FDX_MTU - 3)))
    {
        chSysUnlock();
        idx += sfdxd_escape((uint8_t)chSymQGet(&sfdxdp->oqueue),
                sfdxdp->sendbuffer + idx);
        chSysLock();
    }
    chSysUnlock();

    sfdxdp->sendbuffer[idx++] = SFDX_FRAME_END;
    chSequentialStreamWrite(sfdxdp->configp->farp, sfdxdp->sendbuffer, idx);

    chSysLock();
    if ((sfdxdp->connected == TRUE) && (chSymQIsEmptyI(&sfdxdp->oqueue) == TRUE))
        chnAddFlagsI(sfdxdp, CHN_OUTPUT_EMPTY);
    chSysUnlock();
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
                    chSysLock();
                    byteCount++;
                    if (chSymQIsEmptyI(&sfdxdp->iqueue) == TRUE)
                    {
                        chnAddFlagsI(sfdxdp, CHN_INPUT_AVAILABLE);
                    }
                    if (chSymQPutI(&sfdxdp->iqueue, (uint8_t)c) == Q_FULL)
                    {
                        chnAddFlagsI(sfdxdp, SFDX_OVERRUN_ERROR);
                    }
                    chSysUnlock();
                }
                foundEsc = FALSE;
            }
        }
    }

    /* Raise an event if end of frame was not read */
    if (foundFrameBegin)
    {
        chSysLock();
        chnAddFlagsI(sfdxdp, SFDX_FRAMING_ERROR);
        chSysUnlock();
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
__attribute__((noreturn))
static void sfdxd_pump(void* parameters)
{
    SerialFdxDriver* sfdxdp = (SerialFdxDriver*)parameters;
    chRegSetThreadName("sfdxd_pump");

    msg_t receiveResult = 0;
    while (true)
    {
        if (sfdxdp->state == SFDXD_READY)
        {
            if (sfdxdp->configp->type == SFDXD_MASTER)
            {
                sfdxd_send(sfdxdp);
                receiveResult = sfdxd_receive(sfdxdp,
                        MS2ST(SFDX_MASTER_RECEIVE_TIMEOUT_MS));
            }
            else
            {
                receiveResult = sfdxd_receive(sfdxdp,
                        MS2ST(SFDX_SLAVE_RECEIVE_TIMEOUT_MS));
                if (receiveResult >= 0)
                    sfdxd_send(sfdxdp);
            }

            /* Send connect or disconnect event. */
            if ((receiveResult >= 0) && (sfdxdp->connected == FALSE) &&
                    (sfdxdp->state == SFDXD_READY))
            {
                chSysLock();

                chSymQResetI(&sfdxdp->oqueue);
                chSymQResetI(&sfdxdp->iqueue);
                sfdxdp->connected = TRUE;
                chnAddFlagsI(sfdxdp, CHN_CONNECTED);

                chSysUnlock();
            }
            else if ((receiveResult == Q_TIMEOUT) &&
                    (sfdxdp->connected == TRUE))
            {
                chSysLock();

                sfdxdp->connected = FALSE;
                chnAddFlagsI(sfdxdp, CHN_DISCONNECTED);
                chSymQResetI(&sfdxdp->oqueue);
                chSymQResetI(&sfdxdp->iqueue);

                chSysUnlock();
            }
        }
        else
        {
            /* Nothing to do. Going to sleep. */
            chSysLock();
            chThdSuspendS(&sfdxdp->wait);
            chSysUnlock();
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
    chEvtObjectInit(&sfdxdp->event);
    sfdxdp->state = SFDXD_STOP;
    sfdxdp->connected = FALSE;
    sfdxdp->wait = NULL;

    chSymQObjectInit(&sfdxdp->iqueue, sfdxdp->ib,
            sizeof(sfdxdp->ib));

    chSymQObjectInit(&sfdxdp->oqueue, sfdxdp->ob,
            sizeof(sfdxdp->ob));

#if defined(_CHIBIOS_RT_)
    sfdxdp->tr = NULL;
    /* Filling the thread working area here because the function
       @p chThdCreateI() does not do it.*/
#if CH_DBG_FILL_THREADS
    {
        void *wsp = sfdxdp->wa_pump;
        _thread_memfill((uint8_t*)wsp,
                (uint8_t*)wsp + sizeof(thread_t),
                CH_THREAD_FILL_VALUE);
        _thread_memfill((uint8_t*)wsp + sizeof(thread_t),
                (uint8_t*)wsp + sizeof(sfdxdp->wa_pump),
                CH_STACK_FILL_VALUE);
    }
#endif
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
    chDbgCheck(sfdxdp != NULL);

    chSysLock();
    chDbgAssert((sfdxdp->state == SFDXD_STOP) || (sfdxdp->state == SFDXD_READY),
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
        chSchRescheduleS();
    }
#endif

    chSysUnlock();
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
    chDbgCheck(sfdxdp != NULL);

    chSysLock();
    chDbgAssert((sfdxdp->state == SFDXD_STOP) || (sfdxdp->state == SFDXD_READY),
            "invalid state");
    sfdxdp->state = SFDXD_STOP;

    if (sfdxdp->connected == TRUE)
        chnAddFlagsI(sfdxdp, CHN_DISCONNECTED);

    sfdxdp->connected = FALSE;

    chSymQResetI(&sfdxdp->oqueue);
    chSymQResetI(&sfdxdp->iqueue);
    chSchRescheduleS();
    chSysUnlock();
}
#endif /* HAL_USE_SERIAL_FDX */

/** @} */
