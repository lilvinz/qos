/**
 * @file    qserialsoft.h
 * @brief   SVInfo Driver macros and structures.
 *
 * @addtogroup SVINFO
 * @{
 */

#ifndef _QSERIALSOFT_H_
#define _QSERIALSOFT_H_

#if (HAL_USE_QSERIALSOFT == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants                                                          */
/*===========================================================================*/

/**
 * @name    Serial status flags
 * @{
 */
#define QSERIALSOFT_PARITY_ERROR    32   /**< @brief Parity error happened   */
#define QSERIALSOFT_FRAMING_ERROR   64   /**< @brief Framing error happened  */
#define QSERIALSOFT_OVERRUN_ERROR   128  /**< @brief Overflow happened       */
#define QSERIALSOFT_NOISE_ERROR     256  /**< @brief Noise on the line       */
#define QSERIALSOFT_BREAK_DETECTED  512  /**< @brief Break detected          */
#define QSERIALSOFT_CONTINUOUS_HIGH 1024 /**< @brief Continuous high detected */


/*===========================================================================*/
/* Driver pre-compile time settings                                          */
/*===========================================================================*/
/**
 * @brief   QSerialSoft driver buffer size.
 * @note    The default is 32 bytes for both the transmit and receive
 *          buffers.
 */
#if !defined(QSERIALSOFT_BUFFER_SIZE) || defined(__DOXYGEN__)
#define QSERIALSOFT_BUFFER_SIZE 32
#endif

/*===========================================================================*/
/* Derived constants and error checks                                        */
/*===========================================================================*/
#if !QSERIALSOFT_USE_TRANSMITTER && !QSERIALSOFT_USE_RECEIVER
#error "qserialsoft driver requires transmitter and/or receiver mode enabled."
#endif

#if QSERIALSOFT_USE_TRANSMITTER && !HAL_USE_GPT
#error "qserialsoft driver TX requires HAL_USE_GPT."
#endif

#if QSERIALSOFT_USE_RECEIVER && !HAL_USE_EXT && !HAL_USE_GPT
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
    QSERIALSOFT_UNINIT = 0, /**< Not initialized.                           */
    QSERIALSOFT_STOP = 1,   /**< Stopped.                                   */
    QSERIALSOFT_READY = 2,  /**< Ready.                                     */
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
struct QSerialSoftDriverVMT
{
    _qserialsoft_driver_methods
};

/**
 * @brief   Driver configuration structure.
 */
typedef struct
{
#if QSERIALSOFT_USE_TRANSMITTER
    /**
     * @brief Pointer to @p PWM driver used for communication.
     */
    PWMDriver *pwmd;
    /**
     * @brief Pointer to configuration structure for underlying PWM driver.
     * @note  It is NOT constant because driver needs to change them
     *        during normal functioning.
     */
    PWMConfig *pwmcfg;
    /**
     * @brief   Active logic level for master channel.
     * @details Just set it to @p PWM_OUTPUT_ACTIVE_HIGH when svinfo is
     *          connected to direct (not complementary) output of the timer.
     *          In opposite case you need to check documentation to choose
     *          correct value.
     */
    pwmmode_t pwmmode;
    /**
     * @brief Number of PWM channel used as pulse generator.
     */
    pwmchannel_t pwmchannel;
#endif /* QSERIALSOFT_USE_TRANSMITTER */

#if QSERIALSOFT_USE_RECEIVER
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
    bool rx_invert;
#endif

    /**
     * @brief Serial baud rate.
     */
    uint32_t speed;

    /**
     * @brief Count of data bits.
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

} qserialsoftConfig;

/**
 * @extends BaseAsynchronousChannel
 *
 * @brief   QSerialSoft driver class
 * @details This class extends @p BaseAsynchronousChannel.
 */
typedef struct
{
    /* Virtual Methods Table */
    const struct QSerialSoftDriverVMT* vmt;
    _base_asynchronous_channel_data

    /* Driver config */
    qserialsoftConfig *config;
    /* Driver state */
    qserialsoft_state_t state;
#if QSERIALSOFT_USE_TRANSMITTER
    /* Output bit number */
    uint8_t obit;
    /* Output byte buffer */
    uint8_t obyte;
    /* Output queue */
    OutputQueue oqueue;
    /* Output circular buffer */
    uint8_t ob[QSERIALSOFT_BUFFER_SIZE];
#endif
#if QSERIALSOFT_USE_RECEIVER
    /* Input bit number */
    uint8_t ibit;
    /* Input byte buffer */
    uint16_t ibyte;
    /* Input queue */
    InputQueue iqueue;
    /* Input circular buffer */
    uint8_t ib[QSERIALSOFT_BUFFER_SIZE];
#endif
} qserialsoftDriver;

/*===========================================================================*/
/* Driver macros                                                             */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations                                                     */
/*===========================================================================*/

extern qserialsoftDriver QSERIALSOFTD1;

#ifdef __cplusplus
extern "C"
{
#endif
void qserialsoftObjectInit(qserialsoftDriver *owp);
void qserialsoftStart(qserialsoftDriver *owp, qserialsoftConfig *config);
void qserialsoftStop(qserialsoftDriver *owp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_QSERIALSOFT */

#endif /* _QSERIALSOFT_H_ */

/** @} */

