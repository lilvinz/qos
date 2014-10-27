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
#define SFDX_BYTE_BEGIN 0x12
#define SFDX_BYTE_END 0x13
#define SFDX_BYTE_ESC 0x7D

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/
static msg_t sfdxd_pump(void* parameters) __attribute__ ((noreturn));
static void sfdx_send(SerialFdxDriver *sfdxdp);
static void sfdx_receive(SerialFdxDriver *sfdxdp, systime_t timeout);
static uint8_t sfdxd_escape(uint8_t c, uint8_t* buffer, uint8_t size);

/*
 * Interface implementation, the following functions just invoke the equivalent
 * queue-level function or macro.
 */

static msg_t putt(void *ip, uint8_t b, systime_t timeout)
{
	SerialFdxDriver	*sfdxdp = (SerialFdxDriver*)ip;
	size_t result = chSymQPutTimeout(&sfdxdp->oqueue, b, timeout);
	return result;
}

static msg_t gett(void *ip, systime_t timeout)
{
	SerialFdxDriver	*sfdxdp = (SerialFdxDriver*)ip;
	size_t result = chSymQGetTimeout(&sfdxdp->iqueue, timeout);
	return result;
}

static size_t writet(void *ip, const uint8_t *bp, size_t n, systime_t timeout)
{
	SerialFdxDriver	*sfdxdp = (SerialFdxDriver*)ip;
	size_t result = chSymQWriteTimeout(&sfdxdp->oqueue, bp, n, TIME_INFINITE);
	return result;
}

static size_t readt(void *ip, uint8_t *bp, size_t n, systime_t timeout)
{
	SerialFdxDriver	*sfdxdp = (SerialFdxDriver*)ip;
	size_t result = chSymQReadTimeout(&sfdxdp->iqueue, bp, n, timeout);
	return result;
}

static size_t write(void *ip, const uint8_t *bp, size_t n)
{

	return writet(ip, bp, n, TIME_INFINITE);
}

static size_t read(void *ip, uint8_t *bp, size_t n)
{
	return readt(ip, bp, n, TIME_INFINITE);
}

static msg_t put(void *ip, uint8_t b)
{
	return putt(ip, b, TIME_INFINITE);
}

static msg_t get(void *ip)
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
 * @brief   Virtual serial driver initialization.
 * @note    This function is implicitly invoked by @p qhalInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void sfdxdInit(void)
{
}

/**
 * @brief   Initializes a generic full duplex driver object.
 * @details The HW dependent part of the initialization has to be performed
 *          outside, usually in the hardware initialization code.
 *
 * @param[out] sfdxdp   pointer to a @p SerialFdxDriver structure
 *
 * @init
 */
void sfdxdObjectInit(SerialFdxDriver *sfdxdp)
{
    sfdxdp->vmt = &vmt;
    chEvtInit(&sfdxdp->event);
    sfdxdp->state = SFDXD_STOP;
    sfdxdp->thd_ptr = NULL;

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
 * @param[in] config        the architecture-dependent serial driver configuration.
 *                          If this parameter is set to @p NULL then a default
 *                          configuration is used.
 *
 * @api
 */
void sfdxdStart(SerialFdxDriver *sfdxdp, const SerialFdxConfig *configp)
{
    chDbgCheck(sfdxdp != NULL, "sfdxdStart");

    chSysLock();
    chDbgAssert((sfdxdp->state == SFDXD_STOP) || (sfdxdp->state == SFDXD_READY),
            "sfdxdStart(), #1",
            "invalid state");
    sfdxdp->configp = configp;
    sfdxdp->state = SFDXD_READY;
    chnAddFlagsI(sfdxdp, CHN_CONNECTED);

    if (sfdxdp->thd_ptr == NULL)
    	sfdxdp->thd_ptr = sfdxdp->thd_wait = chThdCreateI(sfdxdp->wa_pump,
                                                        sizeof sfdxdp->wa_pump,
                                                        SERIAL_FDX_THREAD_PRIO,
                                                        sfdxd_pump,
                                                        sfdxdp);
    chThdResumeI(sfdxdp->thd_ptr);
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
void sfdxdStop(SerialFdxDriver *sfdxdp)
{
    chDbgCheck(sfdxdp != NULL, "sfdxdStop");

    chSysLock();
    chDbgAssert((sfdxdp->state == SFDXD_STOP) || (sfdxdp->state == SFDXD_READY),
                "sfdxdStop(), #1",
                "invalid state");
    sfdxdp->state = SFDXD_STOP;

    chnAddFlagsI(sfdxdp, CHN_DISCONNECTED);
	chSymQResetI(&sfdxdp->oqueue);
	chSymQResetI(&sfdxdp->iqueue);
    chSchRescheduleS();
    chSysUnlock();
}

static msg_t sfdxd_pump(void* parameters)
{
	SerialFdxDriver *sfdxdp = (SerialFdxDriver*)parameters;
	chRegSetThreadName("sfdxd_pump");

	while(TRUE)
	{
		if (sfdxdp->state == SFDXD_READY)
		{
			if(sfdxdp->configp->type == SFDXD_MASTER)
			{
				sfdx_send(sfdxdp);
				sfdx_receive(sfdxdp, S2ST(1));
			}
			else
			{
				sfdx_receive(sfdxdp, TIME_INFINITE);
				sfdx_send(sfdxdp);
			}
		}
	}
}

static void sfdx_send(SerialFdxDriver *sfdxdp)
{
	uint8_t buffer[2];
	uint8_t charCount = 0;

	chSequentialStreamPut(sfdxdp->configp->farp, SFDX_BYTE_BEGIN);
	chSysLock();
	while(chSymQIsEmptyI(&sfdxdp->oqueue) == FALSE)
	{
		chSysUnlock();
		buffer[0] = (uint8_t)chSymQGet(&sfdxdp->oqueue);

		charCount = sfdxd_escape(buffer[0], buffer, sizeof(buffer));

		if (chSequentialStreamWrite(sfdxdp->configp->farp, buffer, charCount) != charCount)
		{

		}
		chSysLock();
	}
	chSysUnlock();


	chSequentialStreamPut(sfdxdp->configp->farp, SFDX_BYTE_END);

	chSysLock();
	if (chSymQIsEmptyI(&sfdxdp->oqueue) == TRUE)
		chnAddFlagsI(sfdxdp, CHN_OUTPUT_EMPTY);
	chSysUnlock();


}

static void sfdx_receive(SerialFdxDriver *sfdxdp, systime_t timeout)
{
	bool foundFrameBegin = FALSE;
	bool foundEsc = FALSE;
	uint8_t c;
	while((c = (uint8_t)chnGetTimeout((BaseAsynchronousChannel*)sfdxdp->configp->farp, timeout)) != Q_RESET)
	{
		if (c == SFDX_BYTE_BEGIN && foundFrameBegin == FALSE && foundEsc == FALSE)
		{
			foundFrameBegin = TRUE;
		}
		else if(c == SFDX_BYTE_END && foundFrameBegin == TRUE && foundEsc == FALSE)
		{
			return;
		}
		else if(c == SFDX_BYTE_END && foundFrameBegin == TRUE && foundEsc == FALSE)
		{
			foundEsc = TRUE;
		}
		else
		{
			if(c == SFDX_BYTE_ESC && foundEsc == FALSE)
			{
				foundEsc = TRUE;
			}
			else
			{
				if (chSymQPut(&sfdxdp->iqueue, c) == Q_OK)
				{
					chSysLock();
						chnAddFlagsI(sfdxdp, CHN_INPUT_AVAILABLE);
					chSysUnlock();
				}
				foundEsc = FALSE;
			}
		}
	}
}

static uint8_t sfdxd_escape(uint8_t c, uint8_t* buffer, uint8_t size)
{
	uint8_t idx = 0;
	if (c == SFDX_BYTE_BEGIN || c == SFDX_BYTE_END || c == SFDX_BYTE_ESC)
	{
		buffer[idx++] = SFDX_BYTE_ESC;
	}
	buffer[idx++] = c;
	return idx;
}
#endif /* HAL_USE_SERIAL_FDX */

/** @} */
