/**
 * @file    qserial_fdx.h
 * @brief   serial full duplex driver macros and structures.
 *
 * @addtogroup SERIAL_FDX
 * @{
 */

#ifndef _QSERIAL_FDX_H_
#define _QSERIAL_FDX_H_

#if HAL_USE_SERIAL_FDX || defined(__DOXYGEN__)

#include "qsymqueue.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Virtual serial configuration options
 * @{
 */

/**
 * @brief   Virtual serial buffer size.
 * @note    The default is 256 bytes for both the transmit and receive
 *          buffers.
 */
#if !defined(SERIAL_FDX_BUFFER_SIZE) || defined(__DOXYGEN__)
#define SERIAL_FDX_BUFFER_SIZE  256
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !CH_USE_QUEUES && !CH_USE_EVENTS
#error "Virtual serial fdx requires CH_USE_QUEUES and CH_USE_EVENTS"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief Driver state machine possible states.
 */
typedef enum
{
    SDFDX_UNINIT = 0,               /**< Not initialized.                */
    SDFDX_STOP = 1,                 /**< Stopped.                        */
    SDFDX_READY = 2                 /**< Ready.                          */
} sdfdxstate_t;

/**
 * @brief   Structure representing a serial driver.
 */
typedef struct SerialFdxDriver SerialFdxDriver;

/**
 * @brief   Virtual serial driver configuration structure.
 * @details An instance of this structure must be passed to @p svirtualdStart()
 *          in order to configure and start the driver operations.
 */
typedef struct
{
    /* Pointer to the far end. */
    SerialFdxDriver *farp;
} SerialFdxConfig;

/**
 * @brief   @p SerialFdxDriver specific data.
 */
#define _serial_fdx_driver_data                                           \
    _base_asynchronous_channel_data                                           \
    /* Driver state. */                                                       \
	sdfdxstate_t state;                                                   \
    /* Incoming data queue.*/                                                 \
    SymmetricQueue queue;                                                     \
    /* Input buffer.*/                                                        \
    uint8_t queuebuf[SERIAL_FDX_BUFFER_SIZE];

/**
 * @brief   @p SerialFdxDriver specific methods.
 */
#define _serial_fdx_driver_methods                                        \
    _base_asynchronous_channel_methods

/**
 * @extends BaseAsynchronousChannelVMT
 *
 * @brief   @p SerialFdxDriver virtual methods table.
 */
struct SerialFdxDriverVMT
{
    _serial_fdx_driver_methods
};

/**
 * @extends BaseAsynchronousChannel
 *
 * @brief   Full duplex virtual serial driver class.
 * @details This class extends @p BaseAsynchronousChannel by adding virtual
 *          I/O queues.
 */
struct SerialFdxDriver
{
    /** @brief Virtual Methods Table.*/
    const struct SerialFdxDriverVMT *vmt;
    _serial_fdx_driver_data
    const SerialFdxConfig *configp;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
    void sdfdxInit(void);
    void sdfdxObjectInit(SerialFdxDriver *sdfdxp);
    void sdfdxStart(SerialFdxDriver *sdfdxp,
            const SerialFdxConfig *configp);
    void sdfdxStop(SerialFdxDriver *sdfdxp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SERIAL_FDXL */

#endif /* _QSERIAL_FDX_H_ */

/** @} */
