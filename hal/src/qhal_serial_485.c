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
 * @file    qserial_485.c
 * @brief   Serial 485 driver code.
 *
 * @addtogroup SERIAL_485
 * @{
 */

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

static size_t _write(void *ip, const uint8_t *bp, size_t n) {

  return oqWriteTimeout(&((Serial485Driver *)ip)->oqueue, bp,
                          n, TIME_INFINITE);
}

static size_t _read(void *ip, uint8_t *bp, size_t n) {

  return iqReadTimeout(&((Serial485Driver *)ip)->iqueue, bp,
                         n, TIME_INFINITE);
}

static msg_t _put(void *ip, uint8_t b) {

  return oqPutTimeout(&((Serial485Driver *)ip)->oqueue, b, TIME_INFINITE);
}

static msg_t _get(void *ip) {

  return iqGetTimeout(&((Serial485Driver *)ip)->iqueue, TIME_INFINITE);
}

static msg_t _putt(void *ip, uint8_t b, systime_t timeout) {

  return oqPutTimeout(&((Serial485Driver *)ip)->oqueue, b, timeout);
}

static msg_t _gett(void *ip, systime_t timeout) {

  return iqGetTimeout(&((Serial485Driver *)ip)->iqueue, timeout);
}

static size_t _writet(void *ip, const uint8_t *bp, size_t n, systime_t time) {

  return oqWriteTimeout(&((Serial485Driver *)ip)->oqueue, bp, n, time);
}

static size_t _readt(void *ip, uint8_t *bp, size_t n, systime_t time) {

  return iqReadTimeout(&((Serial485Driver *)ip)->iqueue, bp, n, time);
}

static msg_t _ctl(void *ip, unsigned int operation, void *arg) {
  Serial485Driver *s485dp = (Serial485Driver *)ip;

  osalDbgCheck(s485dp != NULL);

  switch (operation) {
  case CHN_CTL_NOP:
    osalDbgCheck(arg == NULL);
    break;
  case CHN_CTL_INVALID:
    osalDbgAssert(false, "invalid CTL operation");
    break;
  default:
#if defined(SD_LLD_IMPLEMENTS_CTL)
    /* Delegating to the LLD if supported.*/
    return s485d_lld_control(s485dp, operation, arg);
#else
    break;
#endif
  }
  return MSG_OK;
}

static const struct Serial485DriverVMT vmt = {
  (size_t)0,
  _write, _read, _put, _get,
  _putt, _gett, _writet, _readt,
  _ctl
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
  osalEventObjectInit(&s485dp->event);
  s485dp->state = S485D_STOP;
  iqObjectInit(&s485dp->iqueue, s485dp->ib, SERIAL_BUFFERS_SIZE, inotify, s485dp);
  oqObjectInit(&s485dp->oqueue, s485dp->ob, SERIAL_BUFFERS_SIZE, onotify, s485dp);
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

  osalDbgCheck(s485dp != NULL);

  osalSysLock();
  osalDbgAssert((s485dp->state == S485D_STOP) || (s485dp->state == S485D_READY),
              "invalid state");
  s485dp->config = config;
  s485d_lld_start(s485dp, config);
  s485dp->state = S485D_READY;
  chnAddFlagsI(s485dp, CHN_CONNECTED);
  osalOsRescheduleS();
  osalSysUnlock();
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

  osalDbgCheck(s485dp != NULL);

  osalSysLock();
  osalDbgAssert((s485dp->state == S485D_STOP) || (s485dp->state == S485D_READY),
              "invalid state");
  chnAddFlagsI(s485dp, CHN_DISCONNECTED);
  s485d_lld_stop(s485dp);
  s485dp->state = S485D_STOP;
  oqResetI(&s485dp->oqueue);
  iqResetI(&s485dp->iqueue);
  osalOsRescheduleS();
  osalSysUnlock();
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

  osalDbgCheckClassI();
  osalDbgCheck(s485dp != NULL);

  if (iqIsEmptyI(&s485dp->iqueue))
    chnAddFlagsI(s485dp, CHN_INPUT_AVAILABLE);
  if (iqPutI(&s485dp->iqueue, b) < Q_OK)
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

  osalDbgCheckClassI();
  osalDbgCheck(s485dp != NULL);

  b = oqGetI(&s485dp->oqueue);
  if (b < Q_OK)
    chnAddFlagsI(s485dp, CHN_OUTPUT_EMPTY);
  return b;
}

/**
 * @brief   Direct output check on a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          checks directly the output queue. This is faster but cannot
 *          be used to check different channels implementations.
 *
 * @param[in] s485dp    pointer to a @p Serial485Driver object
 * @return              The queue status.
 * @retval false        if the next write operation would not block.
 * @retval true         if the next write operation would block.
 *
 * @deprecated
 *
 * @api
 */
bool sdPutWouldBlock(Serial485Driver *s485dp) {
  bool b;

  osalSysLock();
  b = oqIsFullI(&s485dp->oqueue);
  osalSysUnlock();

  return b;
}

/**
 * @brief   Direct input check on a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          checks directly the input queue. This is faster but cannot
 *          be used to check different channels implementations.
 *
 * @param[in] s485dp    pointer to a @p Serial485Driver object
 * @return              The queue status.
 * @retval false        if the next write operation would not block.
 * @retval true         if the next write operation would block.
 *
 * @deprecated
 *
 * @api
 */
bool sdGetWouldBlock(Serial485Driver *s485dp) {
  bool b;

  osalSysLock();
  b = iqIsEmptyI(&s485dp->iqueue);
  osalSysUnlock();

  return b;
}

/**
 * @brief   Control operation on a serial port.
 *
 * @param[in] s485dp    pointer to a @p Serial485Driver object
 * @param[in] operation control operation code
 * @param[in,out] arg   operation argument
 *
 * @return              The control operation status.
 * @retval MSG_OK       in case of success.
 * @retval MSG_TIMEOUT  in case of operation timeout.
 * @retval MSG_RESET    in case of operation reset.
 *
 * @api
 */
msg_t sdControl(Serial485Driver *s485dp, unsigned int operation, void *arg) {

  return _ctl((void *)s485dp, operation, arg);
}

#endif /* HAL_USE_SERIAL_485 */

/** @} */
