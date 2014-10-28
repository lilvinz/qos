/**
 * @file    qserial_fdx.c
 * @brief   Serial full duplex driver code.
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
static msg_t sfdxd_pump(void* parameters) __attribute__((noreturn));
static void sfdx_send(SerialFdxDriver* sfdxdp);
static msg_t sfdx_receive(SerialFdxDriver* sfdxdp, systime_t timeout);
static uint8_t sfdxd_escape(uint8_t c, uint8_t* buffer);

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
    chEvtInit(&sfdxdp->event);
    sfdxdp->state = SFDXD_STOP;
    sfdxdp->connected = FALSE;
    sfdxdp->thd_ptr = NULL;
    sfdxdp->thd_wait = NULL;

    chSymQInit(&sfdxdp->iqueue, sfdxdp->ib,
            sizeof(sfdxdp->ib));

    chSymQInit(&sfdxdp->oqueue, sfdxdp->ob,
            sizeof(sfdxdp->ob));

    /* Filling the thread working area here because the function
     @p chThdCreateI() does not do it. */
#if CH_DBG_FILL_THREADS
    {
        void *wsp = sfdxdp->wa_pump;
        _thread_memfill((uint8_t*)wsp,
                (uint8_t*)wsp + sizeof(Thread),
                CH_THREAD_FILL_VALUE);
        _thread_memfill((uint8_t*)wsp + sizeof(Thread),
                (uint8_t*)wsp + sizeof(sfdxdp->wa_pump) - sizeof(Thread),
                CH_STACK_FILL_VALUE);
    }
#endif
}

/**
 * @brief   Configures and starts the driver.
 *
 * @param[in] sfdxdp    pointer to a @p SerialFdxDriver object
 * @param[in] config    the architecture-dependent serial full duplex driver configuration.
 *
 * @api
 */
void sfdxdStart(SerialFdxDriver* sfdxdp, const SerialFdxConfig *configp)
{
    chDbgCheck(sfdxdp != NULL, "sfdxdStart");

    chSysLock();
    chDbgAssert((sfdxdp->state == SFDXD_STOP) || (sfdxdp->state == SFDXD_READY),
            "sfdxdStart(), #1",
            "invalid state");
    sfdxdp->configp = configp;
    sfdxdp->state = SFDXD_READY;
    sfdxdp->connected = FALSE;

    if (sfdxdp->thd_ptr == NULL)
    sfdxdp->thd_ptr = sfdxdp->thd_wait = chThdCreateI(sfdxdp->wa_pump,
            sizeof sfdxdp->wa_pump,
            SERIAL_FDX_THREAD_PRIO,
            sfdxd_pump,
            sfdxdp);
    if (sfdxdp->thd_wait != NULL)
    {
        chThdResumeI(sfdxdp->thd_wait);
        sfdxdp->thd_wait = NULL;
    }

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
    chDbgCheck(sfdxdp != NULL, "sfdxdStop");

    chSysLock();
    chDbgAssert((sfdxdp->state == SFDXD_STOP) || (sfdxdp->state == SFDXD_READY),
            "sfdxdStop(), #1",
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

/**
 * @brief   Drivers pump thread function.
 *
 * @param[in] parameters    pointer to a @p SerialFdxDriver object
 *
 */
static msg_t sfdxd_pump(void* parameters)
{
    SerialFdxDriver* sfdxdp = (SerialFdxDriver*)parameters;
    chRegSetThreadName("sfdxd_pump");

    msg_t receiveResult = 0;
    while (TRUE)
    {
        if (sfdxdp->state == SFDXD_READY)
        {
            if (sfdxdp->configp->type == SFDXD_MASTER)
            {
                sfdx_send(sfdxdp);
                receiveResult = sfdx_receive(sfdxdp, MS2ST(SFDX_MASTER_RECEIVE_TIMEOUT_MS));
            }
            else
            {
                receiveResult = sfdx_receive(sfdxdp, MS2ST(SFDX_SLAVE_RECEIVE_TIMEOUT_MS));
                if (receiveResult >= 0)
                sfdx_send(sfdxdp);
            }

            /* send connect or disconnect event */
            if ((receiveResult >= 0) && (sfdxdp->connected == FALSE) && (sfdxdp->state == SFDXD_READY))
            {
                chSysLock();
                sfdxdp->connected = TRUE;
                chnAddFlagsI(sfdxdp, CHN_CONNECTED);
                chSysUnlock();
            }
            else if ((receiveResult == Q_TIMEOUT) && (sfdxdp->connected == TRUE))
            {
                chSysLock();
                sfdxdp->connected = FALSE;
                chnAddFlagsI(sfdxdp, CHN_DISCONNECTED);
                chSysUnlock();
            }
        }
        else
        {
            /* nothing to do. going to sleep */
            chSysLock();
            sfdxdp->thd_wait = chThdSelf();
            chSchGoSleepS(THD_STATE_SUSPENDED);
            chSysUnlock();
        }
    }
}

/**
 * @brief   Send function
 * @details Called from pump thread function to send a frame.
 *
 * @param[in] sfdxdp    pointer to a @p SerialFdxDriver object
 */
static void sfdx_send(SerialFdxDriver* sfdxdp)
{
    uint8_t idx = 0;
    sfdxdp->sendbuffer[idx++] = SFDX_FRAME_BEGIN;

    chSysLock();
    while ((chSymQIsEmptyI(&sfdxdp->oqueue) == FALSE) && (idx < SERIAL_FDX_MTU - 2))
    {
        chSysUnlock();
        idx += sfdxd_escape((uint8_t)chSymQGet(&sfdxdp->oqueue), sfdxdp->sendbuffer + idx);
        chSysLock();
    }
    chSysUnlock();

    sfdxdp->sendbuffer[idx++] = SFDX_FRAME_END;
    chSequentialStreamWrite(sfdxdp->configp->farp, sfdxdp->sendbuffer, idx);

    chSysLock();
    if (chSymQIsEmptyI(&sfdxdp->oqueue) == TRUE)
    chnAddFlagsI(sfdxdp, CHN_OUTPUT_EMPTY);
    chSysUnlock();
}

/**
 * @brief   Receive function
 * @details Called from pump thread function to receive a frame.
 *
 * @param[in] sfdxdp    pointer to a @p SerialFdxDriver object
 * @param[in] timeout   timeout for waiting for new data
 *
 * @return              count of received bytes.
 * @retval Q_TIMEOUT    if the specified time expired.
 * @retval Q_RESET      if the channel associated queue (if any) has been
 *                      reset.
 */
static msg_t sfdx_receive(SerialFdxDriver* sfdxdp, systime_t timeout)
{
    bool foundFrameBegin = FALSE;
    bool foundEsc = FALSE;
    msg_t byteCount = 0;
    msg_t c;
    while ((c = chnGetTimeout((BaseAsynchronousChannel*)sfdxdp->configp->farp, timeout)) >= 0)
    {
        if (c == SFDX_FRAME_BEGIN && foundFrameBegin == FALSE && foundEsc == FALSE)
        {
            foundFrameBegin = TRUE;
        }
        else if (c == SFDX_FRAME_END && foundFrameBegin == TRUE && foundEsc == FALSE)
        {
            return byteCount;
        }
        else if (c == SFDX_FRAME_END && foundFrameBegin == TRUE && foundEsc == FALSE)
        {
            foundEsc = TRUE;
        }
        else
        {
            if (c == SFDX_BYTE_ESC && foundEsc == FALSE)
            {
                foundEsc = TRUE;
            }
            else
            {
                byteCount++;
                if (chSymQPut(&sfdxdp->iqueue, (uint8_t)c) == Q_OK)
                {
                    chSysLock();
                    chnAddFlagsI(sfdxdp, CHN_INPUT_AVAILABLE);
                    chSysUnlock();
                }
                foundEsc = FALSE;
            }
        }
    }

    return c;
}

/**
 * @brief   Escaping characters
 * @details If needed function escapes a single character.
 *
 * @param[in] c         character to escape
 * @param[out] buffer   pointer to message buffer
 *
 * @return              byte count used to escape the character.
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
#endif /* HAL_USE_SERIAL_FDX */

/** @} */
