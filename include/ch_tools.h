
#ifndef CH_TOOLS_H_
#define CH_TOOLS_H_

#include "qhal.h"

/*===========================================================================*/
/* Constants                                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Pre-compile time settings                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks                                        */
/*===========================================================================*/

#if CH_KERNEL_MAJOR != 2 || CH_KERNEL_MINOR != 6
#error "ch_tools requires ChibiOS 2.6 and requires rework for other versions."
#endif

/*===========================================================================*/
/* Data structures and types                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Macros                                                                    */
/*===========================================================================*/

/**
 * @name    Time conversion utilities
 * @{
 */

/**
 * @brief   System ticks to seconds.
 * @details Converts from system ticks number to seconds.
 * @note    The result is rounded up to the next second boundary.
 *
 * @param[in] n         number of system ticks
 * @return              The number of seconds.
 *
 * @api
 */
#define ST2S(n) (((n) + CH_FREQUENCY - 1UL) / CH_FREQUENCY)

/**
 * @brief   System ticks to milliseconds.
 * @details Converts from system ticks number to milliseconds.
 * @note    The result is rounded up to the next millisecond boundary.
 *
 * @param[in] n         number of system ticks
 * @return              The number of milliseconds.
 *
 * @api
 */
#define ST2MS(n) (((n) * 1000UL + CH_FREQUENCY - 1UL) /              \
                  CH_FREQUENCY)

/**
 * @brief   System ticks to microseconds.
 * @details Converts from system ticks number to microseconds.
 * @note    The result is rounded up to the next microsecond boundary.
 *
 * @param[in] n         number of system ticks
 * @return              The number of microseconds.
 *
 * @api
 */
#define ST2US(n) (((n) * 1000000UL + CH_FREQUENCY - 1UL) /           \
                  CH_FREQUENCY)

/** @} */

/*===========================================================================*/
/* External declarations                                                     */
/*===========================================================================*/

#ifdef __cplusplus
extern "C"
{
#endif
    void chThdSleepPeriod(systime_t *previous, systime_t period);
#if CH_CFG_USE_EVENTS || defined(__DOXYGEN__)
    eventmask_t chEvtWaitAnyPeriod(eventmask_t mask, systime_t *previous,
            systime_t period);
#endif /* CH_CFG_USE_EVENTS || defined(__DOXYGEN__) */
#ifdef __cplusplus
}
#endif

#endif /* CH_TOOLS_H_ */
