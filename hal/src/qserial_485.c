/**
 * @file    qserial_485.c
 * @brief   Serial 485 driver code.
 *
 * @addtogroup SERIAL_485
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_SERIAL_485 || defined(__DOXYGEN__)

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

static size_t write(void *ip, const uint8_t *bp, size_t n) {

  return chOQWriteTimeout(&((Serial485Driver *)ip)->oqueue, bp,
                          n, TIME_INFINITE);
}

static size_t read(void *ip, uint8_t *bp, size_t n) {

  return chIQReadTimeout(&((Serial485Driver *)ip)->iqueue, bp,
                         n, TIME_INFINITE);
}

static msg_t put(void *ip, uint8_t b) {

  return chOQPutTimeout(&((Serial485Driver *)ip)->oqueue, b, TIME_INFINITE);
}

static msg_t get(void *ip) {

  return chIQGetTimeout(&((Serial485Driver *)ip)->iqueue, TIME_INFINITE);
}

static msg_t putt(void *ip, uint8_t b, systime_t timeout) {

  return chOQPutTimeout(&((Serial485Driver *)ip)->oqueue, b, timeout);
}

static msg_t gett(void *ip, systime_t timeout) {

  return chIQGetTimeout(&((Serial485Driver *)ip)->iqueue, timeout);
}

static size_t writet(void *ip, const uint8_t *bp, size_t n, systime_t time) {

  return chOQWriteTimeout(&((Serial485Driver *)ip)->oqueue, bp, n, time);
}

static size_t readt(void *ip, uint8_t *bp, size_t n, systime_t time) {

  return chIQReadTimeout(&((Serial485Driver *)ip)->iqueue, bp, n, time);
}

static const struct Serial485DriverVMT vmt = {
  write, read, put, get,
  putt, gett, writet, readt
};

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Serial Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void s485dInit(void) {

  s485d_lld_init();
}

/**
 * @brief   Initializes a generic full duplex driver object.
 * @details The HW dependent part of the initialization has to be performed
 *          outside, usually in the hardware initialization code.
 *
 * @param[out] s485dp   pointer to a @p Serial485Driver structure
 * @param[in] inotify   pointer to a callback function that is invoked when
 *                      some data is read from the Queue. The value can be
 *                      @p NULL.
 * @param[in] onotify   pointer to a callback function that is invoked when
 *                      some data is written in the Queue. The value can be
 *                      @p NULL.
 *
 * @init
 */
void s485dObjectInit(Serial485Driver *s485dp, qnotify_t inotify, qnotify_t onotify) {

  s485dp->vmt = &vmt;
  chEvtInit(&s485dp->event);
  s485dp->state = S485D_STOP;
  chIQInit(&s485dp->iqueue, s485dp->ib, SERIAL_BUFFERS_SIZE, inotify, s485dp);
  chOQInit(&s485dp->oqueue, s485dp->ob, SERIAL_BUFFERS_SIZE, onotify, s485dp);
}

/**
 * @brief   Configures and starts the driver.
 *
 * @param[in] s485dp    pointer to a @p Serial485Driver object
 * @param[in] config    the architecture-dependent serial driver configuration.
 *                      If this parameter is set to @p NULL then a default
 *                      configuration is used.
 *
 * @api
 */
void s485dStart(Serial485Driver *s485dp, const Serial485Config *config) {

  chDbgCheck(s485dp != NULL, "s485d485Start");

  chSysLock();
  chDbgAssert((s485dp->state == S485D_STOP) || (s485dp->state == S485D_READY),
              "s485d485Start(), #1",
              "invalid state");
  s485dp->config = config;
  s485d_lld_start(s485dp, config);
  s485dp->state = S485D_READY;
  chnAddFlagsI(s485dp, CHN_CONNECTED);
  chSysUnlock();
}

/**
 * @brief   Stops the driver.
 * @details Any thread waiting on the driver's queues will be awakened with
 *          the message @p Q_RESET.
 *
 * @param[in] s485dp    pointer to a @p Serial485Driver object
 *
 * @api
 */
void s485dStop(Serial485Driver *s485dp) {

  chDbgCheck(s485dp != NULL, "s485d485Stop");

  chSysLock();
  chDbgAssert((s485dp->state == S485D_STOP) || (s485dp->state == S485D_READY),
              "s485d485Stop(), #1",
              "invalid state");
  chnAddFlagsI(s485dp, CHN_DISCONNECTED);
  s485d_lld_stop(s485dp);
  s485dp->state = S485D_STOP;
  chOQResetI(&s485dp->oqueue);
  chIQResetI(&s485dp->iqueue);
  chSchRescheduleS();
  chSysUnlock();
}

/**
 * @brief   Handles incoming data.
 * @details This function must be called from the input interrupt service
 *          routine in order to enqueue incoming data and generate the
 *          related events.
 * @note    The incoming data event is only generated when the input queue
 *          becomes non-empty.
 * @note    In order to gain some performance it is suggested to not use
 *          this function directly but copy this code directly into the
 *          interrupt service routine.
 *
 * @param[in] s485dp    pointer to a @p Serial485Driver structure
 * @param[in] b         the byte to be written in the driver's Input Queue
 *
 * @iclass
 */
void s485dIncomingDataI(Serial485Driver *s485dp, uint8_t b) {

  chDbgCheckClassI();
  chDbgCheck(s485dp != NULL, "s485dIncomingDataI");

  if (chIQIsEmptyI(&s485dp->iqueue))
    chnAddFlagsI(s485dp, CHN_INPUT_AVAILABLE);
  if (chIQPutI(&s485dp->iqueue, b) < Q_OK)
    chnAddFlagsI(s485dp, S485D_OVERRUN_ERROR);
}

/**
 * @brief   Handles outgoing data.
 * @details Must be called from the output interrupt service routine in order
 *          to get the next byte to be transmitted.
 * @note    In order to gain some performance it is suggested to not use
 *          this function directly but copy this code directly into the
 *          interrupt service routine.
 *
 * @param[in] s485dp    pointer to a @p Serial485Driver structure
 * @return              The byte value read from the driver's output queue.
 * @retval Q_EMPTY      if the queue is empty (the lower driver usually
 *                      disables the interrupt source when this happens).
 *
 * @iclass
 */
msg_t s485dRequestDataI(Serial485Driver *s485dp) {
  msg_t  b;

  chDbgCheckClassI();
  chDbgCheck(s485dp != NULL, "s485dRequestDataI");

  b = chOQGetI(&s485dp->oqueue);
  if (b < Q_OK)
    chnAddFlagsI(s485dp, CHN_OUTPUT_EMPTY);
  return b;
}

#endif /* HAL_USE_SERIAL_485 */

/** @} */
