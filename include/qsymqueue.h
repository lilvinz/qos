/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

                                      ---

    A special exception to the GPL can be applied should you wish to distribute
    a combined work that includes ChibiOS/RT, without being obliged to provide
    the source code for any proprietary components. See the file exception.txt
    for full details of how and when the exception can be applied.
*/

/**
 * @file    chsymqueue.h
 * @brief   Symmetric queue macros and structures.
 *
 * @addtogroup io_queues
 * @{
 */

#ifndef _QSYMQUEUES_H_
#define _QSYMQUEUES_H_

/**
 * @brief   Type of a generic I/O queue structure.
 */
typedef struct symmetric_queue symmetric_queue_t;

/**
 * @brief   Symmetric queue structure.
 * @details This structure represents a symmetric queue.
 */
struct symmetric_queue {
  threads_queue_t q_readers;   /**< @brief Queue of threads waiting to read. */
  threads_queue_t q_writers;   /**< @brief Queue of threads waiting to write.*/
  size_t q_counter;         /**< @brief Resources counter.                   */
  uint8_t *q_buffer;        /**< @brief Pointer to the queue buffer.         */
  uint8_t *q_top;           /**< @brief Pointer to the first location
                                        after the buffer.                    */
  uint8_t *q_wrptr;         /**< @brief Write pointer.                       */
  uint8_t *q_rdptr;         /**< @brief Read pointer.                        */
};

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Returns the queue's buffer size.
 *
 * @param[in] qp        pointer to a @p symmetric_queue_t structure.
 * @return              The buffer size.
 *
 * @iclass
 */
#define chSymQSizeI(sqp) ((size_t)((sqp)->q_top - (sqp)->q_buffer))

/**
 * @brief   Queue space.
 * @details Returns the used space.
 *
 * @param[in] qp        pointer to a @p symmetric_queue_t structure.
 * @return              The used buffer space.
 *
 * @iclass
 */
#define chSymQSpaceI(sqp) ((sqp)->q_counter)

/**
 * @brief   Returns the filled space into a symmetric queue.
 *
 * @param[in] sqp       pointer to a @p symmetric_queue_t structure
 * @return              The number of full bytes in the queue.
 * @retval 0            if the queue is empty.
 *
 * @iclass
 */
#define chSymQGetFullI(sqp) chSymQSpaceI(sqp)

/**
 * @brief   Returns the empty space into a symmetric queue.
 *
 * @param[in] sqp       pointer to a @p symmetric_queue_t structure
 * @return              The number of empty bytes in the queue.
 * @retval 0            if the queue is full.
 *
 * @iclass
 */
#define chSymQGetEmptyI(sqp) (chSymQSizeI(sqp) - chSymQSpaceI(sqp))

/**
 * @brief   Evaluates to @p TRUE if the specified symmetric queue is empty.
 *
 * @param[in] sqp       pointer to a @p symmetric_queue_t structure.
 * @return              The queue status.
 * @retval FALSE        if the queue is not empty.
 * @retval TRUE         if the queue is empty.
 *
 * @iclass
 */
#define chSymQIsEmptyI(sqp) ((bool)(chSymQSpaceI(sqp) == 0))

/**
 * @brief   Evaluates to @p TRUE if the specified symmetric queue is full.
 *
 * @param[in] sqp       pointer to a @p symmetric_queue_t structure.
 * @return              The queue status.
 * @retval FALSE        if the queue is not full.
 * @retval TRUE         if the queue is full.
 *
 * @iclass
 */
#define chSymQIsFullI(sqp) ((bool)(chSymQGetEmptyI(sqp) == 0))

/**
 * @brief   Symmetric queue read.
 * @details This function reads a byte value from a symmetric queue. If the queue
 *          is empty then the calling thread is suspended until a byte arrives
 *          in the queue.
 *
 * @param[in] sqp       pointer to a @p symmetric_queue_t structure
 * @return              A byte value from the queue.
 * @retval Q_RESET      if the queue has been reset.
 *
 * @api
 */
#define chSymQGet(sqp) chSymQGetTimeout(sqp, TIME_INFINITE)

/**
 * @brief   Symmetric queue write.
 * @details This function writes a byte value to a symmetric queue. If the queue
 *          is full then the calling thread is suspended until there is space
 *          in the queue.
 *
 * @param[in] sqp       pointer to a @p symmetric_queue_t structure
 * @param[in] b         the byte value to be written in the queue
 * @return              The operation status.
 * @retval Q_OK         if the operation succeeded.
 * @retval Q_RESET      if the queue has been reset.
 *
 * @api
 */
#define chSymQPut(sqp, b) chSymQPutTimeout(sqp, b, TIME_INFINITE)
 /** @} */

/**
 * @brief   Data part of a static symmetric queue initializer.
 * @details This macro should be used when statically initializing a
 *          symmetric queue that is part of a bigger structure.
 *
 * @param[in] name      the name of the symmetric queue variable
 * @param[in] buffer    pointer to the queue buffer area
 * @param[in] size      size of the queue buffer area
 */
#define _SYMMETRICQUEUE_DATA(name, buffer, size) {                          \
  _THREADSQUEUE_DATA(name),                                                 \
  0,                                                                        \
  (uint8_t *)(buffer),                                                      \
  (uint8_t *)(buffer) + (size),                                             \
  (uint8_t *)(buffer),                                                      \
  (uint8_t *)(buffer),                                                      \
}

/**
 * @brief   Static symmetric queue initializer.
 * @details Statically initialized symmetric queues require no explicit
 *          initialization using @p chSymQObjectInit().
 *
 * @param[in] name      the name of the input queue variable
 * @param[in] buffer    pointer to the queue buffer area
 * @param[in] size      size of the queue buffer area
 */
#define SYMMETRICQUEUE_DECL(name, buffer, size)                             \
  symmetric_queue_t name = _SYMMETRICQUEUE_DATA(name, buffer, size)

#ifdef __cplusplus
extern "C" {
#endif
  void chSymQObjectInit(symmetric_queue_t *sqp, uint8_t *bp, size_t size);
  void chSymQResetI(symmetric_queue_t *sqp);
  msg_t chSymQGetI(symmetric_queue_t *sqp);
  msg_t chSymQGetTimeoutS(symmetric_queue_t *sqp, sysinterval_t timeout);
  msg_t chSymQGetTimeout(symmetric_queue_t *sqp, sysinterval_t timeout);
  size_t chSymQReadTimeoutS(symmetric_queue_t *sqp, uint8_t *bp,
                         size_t n, sysinterval_t timeout);
  size_t chSymQReadTimeout(symmetric_queue_t *sqp, uint8_t *bp,
                         size_t n, sysinterval_t timeout);
  msg_t chSymQPutI(symmetric_queue_t *sqp, uint8_t b);
  msg_t chSymQPutTimeoutS(symmetric_queue_t *sqp, uint8_t b, sysinterval_t timeout);
  msg_t chSymQPutTimeout(symmetric_queue_t *sqp, uint8_t b, sysinterval_t timeout);
  size_t chSymQWriteTimeoutS(symmetric_queue_t *sqp, const uint8_t *bp,
                          size_t n, sysinterval_t timeout);
  size_t chSymQWriteTimeout(symmetric_queue_t *sqp, const uint8_t *bp,
                          size_t n, sysinterval_t timeout);
#ifdef __cplusplus
}
#endif

#endif /* _QSYMQUEUES_H_ */

/** @} */
