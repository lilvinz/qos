/**
 * @file    qserial_fdx.h
 * @brief   Serial full duplex driver macros and structures.
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
/**
 * @name    Serial status flags
 * @{
 */
#define SFDX_OVERRUN_ERROR      32  /**< @brief Overflow happened.          */
#define SFDX_FRAMING_ERROR      64  /**< @brief Framing error happened.     */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Serial full duplex driver configuration options
 * @{
 */

/**
 * @brief   Serial full duplex driver buffer size.
 * @note    The default is 256 bytes for both the transmit and receive
 *          buffers.
 */
#if !defined(SERIAL_FDX_BUFFER_SIZE) || defined(__DOXYGEN__)
#define SERIAL_FDX_BUFFER_SIZE 256
#endif

/**
 * @brief   Serial full duplex driver maximum transfer unit.
 * @details MTU includes frame overhead and payload bytes. Increasing MTU value
 *          leads to higher memory allocation but less frame overhead bytes.
 *          Smallest MTU should be 4.
 * @note    The default is 32 bytes.
 */
#if !defined(SERIAL_FDX_MTU) || defined(__DOXYGEN__)
#define SERIAL_FDX_MTU 32
#endif

/**
 * @brief   Serial full duplex driver frame begin character.
 * @note    The default is 0x12.
 */
#if !defined(SFDX_FRAME_BEGIN) || defined(__DOXYGEN__)
#define SFDX_FRAME_BEGIN 0x12
#endif

/**
 * @brief   Serial full duplex driver frame end character.
 * @note    The default is 0x13.
 */
#if !defined(SFDX_FRAME_END) || defined(__DOXYGEN__)
#define SFDX_FRAME_END  0x13
#endif

/**
 * @brief   Serial full duplex driver escape character.
 * @note    The default is 0x7D.
 */
#if !defined(SFDX_BYTE_ESC) || defined(__DOXYGEN__)
#define SFDX_BYTE_ESC 0x7D
#endif

/**
 * @brief   Slave response timeout in ms.
 * @note    The default is 1000.
 */
#if !defined(SFDX_MASTER_RECEIVE_TIMEOUT_MS) || defined(__DOXYGEN__)
#define SFDX_MASTER_RECEIVE_TIMEOUT_MS 1000
#endif

/**
 * @brief   Master request timeout in ms.
 * @note    The default is 2000.
 */
#if !defined(SFDX_SLAVE_RECEIVE_TIMEOUT_MS) || defined(__DOXYGEN__)
#define SFDX_SLAVE_RECEIVE_TIMEOUT_MS 2000
#endif

/**
 * @brief   Dedicated data pump thread stack size.
 */
#if !defined(SERIAL_FDX_THREAD_STACK_SIZE) || defined(__DOXYGEN__)
#define SERIAL_FDX_THREAD_STACK_SIZE 256
#endif

/**
 * @brief   Dedicated data pump thread priority.
 */
#if !defined(SERIAL_FDX_THREAD_PRIO) || defined(__DOXYGEN__)
#define SERIAL_FDX_THREAD_PRIO LOWPRIO
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !CH_CFG_USE_QUEUES || !CH_CFG_USE_EVENTS
#error "Serial fdx requires CH_CFG_USE_QUEUES and CH_CFG_USE_EVENTS"
#endif

#if SERIAL_FDX_MTU < 4
#error "SERIAL_FDX_MTU must be >= 4"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief Driver state machine possible states.
 */
typedef enum
{
    SFDXD_UNINIT = 0, /**< Not initialized.                */
    SFDXD_STOP = 1,   /**< Stopped.                        */
    SFDXD_READY = 2   /**< Ready.                          */
} sfdxdstate_t;

/**
 * @brief Possible driver operation types.
 */
typedef enum
{
    SFDXD_MASTER = 0, /**< Master.                         */
    SFDXD_SLAVE = 1,  /**< Slave.                          */
} sfdxdtype_t;

/**
 * @brief   Structure representing a serial full duplex driver.
 */
typedef struct SerialFdxDriver SerialFdxDriver;

/**
 * @brief   Serial full duplex driver configuration structure.
 * @details An instance of this structure must be passed to @p sfdxddStart()
 *          in order to configure and start the driver operations.
 */
typedef struct
{
    /* Pointer to the far end. */
    BaseAsynchronousChannel* farp;
    /* Type of this driver instance. */
    sfdxdtype_t type;
} SerialFdxConfig;

/**
 * @brief   @p SerialFdxDriver specific data.
 */
#define _serial_fdx_driver_data                                           \
    _base_asynchronous_channel_data                                       \
    /* Driver state. */                                                   \
    sfdxdstate_t state;                                                   \
    /* Input queue.*/                                                     \
    SymmetricQueue iqueue;                                                \
    /* Output queue.*/                                                    \
    SymmetricQueue oqueue;                                                \
    /* Input circular buffer.*/                                           \
    uint8_t ib[SERIAL_FDX_BUFFER_SIZE];                                   \
    /* Output circular buffer.*/                                          \
    uint8_t ob[SERIAL_FDX_BUFFER_SIZE];                                   \
    /* Buffer for outgoing message composing.*/                           \
    uint8_t sendbuffer[SERIAL_FDX_MTU];                                   \
    /* Connection status.*/                                               \
    bool connected;                                                       \

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
 * @brief   Serial full duplex driver class.
 * @details This class extends @p BaseAsynchronousChannel.
 */
struct SerialFdxDriver
{
    /** @brief Virtual Methods Table.*/
    const struct SerialFdxDriverVMT* vmt;
    _serial_fdx_driver_data
    const SerialFdxConfig* configp;
    /**
     * @brief   Pointer to the thread.
     */
    thread_t* thd_ptr;
    /**
     * @brief   Pointer to the thread when it is sleeping or @p NULL.
     */
    thread_t* thd_wait;
    /**
     * @brief   Working area for the dedicated data pump thread;
     */
    WORKING_AREA(wa_pump, SERIAL_FDX_THREAD_STACK_SIZE);
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Macro Functions (SerialFdxDriver)
 * @{
 */

/**
 * @brief   Returns connection status.
 * @details Connection status changes if a connection event occurs.
 *
 * @param[in] ip        pointer to a @p SerialFdxDriver object
 *
 * @return              connection status
 * @retval TRUE         Driver receives responses from remote driver.
 * @retval FALSE        Driver receives no responses from remote driver.
 *
 * @api
 */
#define sfdxdConnected(ip) ((ip)->connected)

/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif
    void sfdxdInit(void);
    void sfdxdObjectInit(SerialFdxDriver* sfdxdp);
    void sfdxdStart(SerialFdxDriver* sfdxdp,
            const SerialFdxConfig* configp);
    void sfdxdStop(SerialFdxDriver* sfdxdp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SERIAL_FDX */

#endif /* _QSERIAL_FDX_H_ */

/** @} */
