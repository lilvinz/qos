/**
 * @file    qserialsoft.h
 * @brief   SVInfo Driver macros and structures.
 *
 * @addtogroup SVINFO
 * @{
 */

#ifndef _SERIALSOFT_H_
#define _SERIALSOFT_H_

#if (HAL_USE_SERIAL_SOFT == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants                                                          */
/*===========================================================================*/

/**
 * @name    Serial status flags
 * @{
 */
#define SERIALSOFT_PARITY_ERROR    32   /**< @brief Parity error happened   */
#define SERIALSOFT_FRAMING_ERROR   64   /**< @brief Framing error happened  */
#define SERIALSOFT_OVERRUN_ERROR   128  /**< @brief Overflow happened       */
#define SERIALSOFT_NOISE_ERROR     256  /**< @brief Noise on the line       */
#define SERIALSOFT_BREAK_DETECTED  512  /**< @brief Break detected          */
#define SERIALSOFT_CONTINUOUS_HIGH 1024 /**< @brief Continuous high detected */


/*===========================================================================*/
/* Driver pre-compile time settings                                          */
/*===========================================================================*/
/**
 * @brief   QSerialSoft driver buffer size.
 * @note    The default is 32 bytes for both the transmit and receive
 *          buffers.
 */
#if !defined(SERIALSOFT_BUFFER_SIZE) || defined(__DOXYGEN__)
#define SERIALSOFT_BUFFER_SIZE 32
#endif

/*===========================================================================*/
/* Derived constants and error checks                                        */
/*===========================================================================*/
#if !SERIALSOFT_USE_TRANSMITTER && !SERIALSOFT_USE_RECEIVER
#error "qserialsoft driver requires transmitter and/or receiver mode enabled."
#endif

#if SERIALSOFT_USE_TRANSMITTER
#error "qserialsoft transmitter not implemented yet."
#endif

#if SERIALSOFT_USE_TRANSMITTER && !HAL_USE_GPT
#error "qserialsoft driver TX requires HAL_USE_GPT."
#endif

#if SERIALSOFT_USE_RECEIVER && (!HAL_USE_EXT || !HAL_USE_GPT)
#error "qserialsoft driver RX requires HAL_USE_EXT and HAL_USE_GPT."
#endif

#if !HAL_USE_PAL
#error "qserialsoft driver requires HAL_USE_PAL."
#endif

/*===========================================================================*/
/* Driver data structures and types                                          */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum
{
    SERIALSOFT_UNINIT = 0, /**< Not initialized.                           */
    SERIALSOFT_STOP = 1,   /**< Stopped.                                   */
    SERIALSOFT_READY = 2,  /**< Ready.                                     */
} qserialsoft_state_t;

/**
 * @brief   @p QSerialSoft Driver specific methods.
 */
#define _qserialsoft_driver_methods                                        \
    _base_asynchronous_channel_methods

/**
 * @extends BaseAsynchronousChannelVMT
 *
 * @brief   @p QSerialSoft Driver virtual methods table.
 */
struct SerialSoftDriverVMT
{
    _qserialsoft_driver_methods
};

/**
 * @brief   Driver configuration structure.
 */
typedef struct
{
#if SERIALSOFT_USE_TRANSMITTER
    /* ToDo: Implement transmitter config */
#endif /* SERIALSOFT_USE_TRANSMITTER */

#if SERIALSOFT_USE_RECEIVER
    /**
     * @brief Pointer to @p ext driver.
     */
    EXTDriver *extd;

    /**
     * @brief Pointer to @p ext driver configuration.
     * @note  ext driver configuration must not be const
     */
    EXTConfig *extcfg;

    /**
     * @brief Pointer to @p gpt driver.
     */
    GPTDriver *gptd;

    /**
     * @brief Pointer to @p gpt driver configuration.
     * @note  gpt driver configuration must not be const
     */
    GPTConfig *gptcfg;

    /**
     * @bried Port to use for communication.
     */
    ioportid_t rx_port;

    /**
     * @bried Pad to use for communication.
     */
    ioportmask_t rx_pad;

    /**
     * @bried Invert serial polarity.
     */
    bool_t rx_invert;
#endif

    /**
     * @brief Serial baud rate.
     */
    uint32_t speed;

    /**
     * @brief Count of data bits (7 or 8)
     */
    uint8_t databits;

    /**
     * @brief Type of parity (0 = none, 1 = even, 2 = odd).
     */
    uint8_t parity;

    /**
     * @brief Number of stop bits (1, 2).
     */
    uint8_t stopbits;

} SerialSoftConfig;

/**
 * @extends BaseAsynchronousChannel
 *
 * @brief   QSerialSoft driver class
 * @details This class extends @p BaseAsynchronousChannel.
 */
typedef struct
{
    /* Virtual Methods Table */
    const struct SerialSoftDriverVMT* vmt;
    _base_asynchronous_channel_data

    /* Driver config */
    SerialSoftConfig *config;
    /* Driver state */
    qserialsoft_state_t state;
    /* Bit timing - data bit interval */
    uint32_t timing_bit_interval;
    /* Bit timing - center of 1st data bit */
    uint32_t timing_first_bit;
#if SERIALSOFT_USE_TRANSMITTER
    /* Output bit number */
    uint8_t obit;
    /* Output byte buffer */
    uint8_t obyte;
    /* Output queue */
    OutputQueue oqueue;
    /* Output circular buffer */
    uint8_t ob[SERIALSOFT_BUFFER_SIZE];
#endif
#if SERIALSOFT_USE_RECEIVER
    /* Input bit number */
    uint8_t ibit;
    /* Input byte buffer */
    uint16_t ibyte;
    /* Input queue */
    InputQueue iqueue;
    /* Input circular buffer */
    uint8_t ib[SERIALSOFT_BUFFER_SIZE];
#endif
} SerialSoftDriver;

/*===========================================================================*/
/* Driver macros                                                             */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations                                                     */
/*===========================================================================*/

extern SerialSoftDriver QSERIALSOFTD1;

#ifdef __cplusplus
extern "C"
{
#endif
void serialsoftInit(void);
void serialsoftObjectInit(SerialSoftDriver *owp);
void serialsoftStart(SerialSoftDriver *owp, SerialSoftConfig *config);
void serialsoftStop(SerialSoftDriver *owp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SERIAL_SOFT */

#endif /* _SERIALSOFT_H_ */

/** @} */

