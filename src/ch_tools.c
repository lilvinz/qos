
#include "ch_tools.h"

/**
 * @brief   Suspends the invoking thread to match the specified time interval.
 *
 * @param[in] previous  pointer to the previous systime_t
 * @param[in] perios    sleep period to match
 *                      - @a TIME_INFINITE the thread enters an infinite sleep
 *                        state.
 *                      - @a TIME_IMMEDIATE this value is not allowed.
 *
 * @api
 */
void chThdSleepPeriod(systime_t *previous, systime_t period)
{
    systime_t future = *previous + period;
    chSysLock();
    systime_t now = chTimeNow();
    int mustDelay =
        now < *previous ?
        (now < future && future < *previous) :
        (now < future || future < *previous);
    if (mustDelay)
        chThdSleepS(future - now);
    chSysUnlock();
    *previous = future;
}
