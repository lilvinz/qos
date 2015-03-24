
#ifndef CH_TOOLS_H_
#define CH_TOOLS_H_

#include "ch.h"

/**
* @name Time conversion utilities
* @{
*/

/**
* @brief System ticks to seconds.
* @details Converts from system ticks number to seconds.
* @note The result is rounded up to the next second boundary.
*
* @param[in] n number of system ticks
* @return The number of seconds.
*
* @api
*/
#define ST2S(n) (((((n) - 1UL) * 1UL) / CH_FREQUENCY) + 1UL)

/**
* @brief System ticks to milliseconds.
* @details Converts from system ticks number to milliseconds.
* @note The result is rounded up to the next millisecond boundary.
*
* @param[in] n number of system ticks
* @return The number of milliseconds.
*
* @api
*/
#define ST2MS(n) (((((n) - 1UL) * 1000UL) / CH_FREQUENCY) + 1UL)

/**
* @brief System ticks to microseconds.
* @details Converts from system ticks number to microseconds.
* @note The result is rounded up to the next microsecond boundary.
*
* @param[in] n number of system ticks
* @return The number of microseconds.
*
* @api
*/
#define ST2US(n) (((((n) - 1UL) * 1000000UL) / CH_FREQUENCY) + 1UL)

/** @} */

void chThdSleepPeriod(systime_t *previous, systime_t period);
systime_t chThdRemainingPeriod(systime_t *previous, systime_t period);

#endif /* CH_TOOLS_H_ */
