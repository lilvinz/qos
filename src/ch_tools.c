
#include "ch_tools.h"

#include <stdbool.h>

/*===========================================================================*/
/* Local definitions                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Imported variables                                                        */
/*===========================================================================*/

/*===========================================================================*/
/* Exported variables                                                        */
/*===========================================================================*/

/*===========================================================================*/
/* Local types                                                               */
/*===========================================================================*/

/*===========================================================================*/
/* Local constants                                                           */
/*===========================================================================*/

/*===========================================================================*/
/* Local variables                                                           */
/*===========================================================================*/

/*===========================================================================*/
/* Local functions                                                           */
/*===========================================================================*/

/*===========================================================================*/
/* Exported functions                                                        */
/*===========================================================================*/

/**
 * @brief   Suspends the invoking thread to match the specified time interval.
 *
 * @param[in] previous  pointer to the previous systime_t
 * @param[in] period    time period to match
 *                      - @a TIME_INFINITE the thread enters an infinite sleep
 *                        state.
 *                      - @a TIME_IMMEDIATE this value is not allowed.
 *
 * @api
 */
void chThdSleepPeriod(systime_t *previous, systime_t period)
{
    chDbgCheck(period != TIME_INFINITE &&
            previous != NULL, "chThdSleepPeriod");

    systime_t future = *previous + period;

    chSysLock();

    systime_t now = chTimeNow();

    bool mustDelay =
        now < *previous ?
        (now < future && future < *previous) :
        (now < future || future < *previous);

    if (mustDelay)
        chThdSleepS(future - now);

    chSysUnlock();

    *previous = future;
}

/**
 * @brief   Returns remaining time of the specified time interval.
 *          The advancement of previous has to be done by the caller only
 *          after time has passed.
 *
 * @param[in] previous  pointer to the previous systime_t
 * @param[in] period    time period to match
 *                      - @a TIME_INFINITE the thread enters an infinite sleep
 *                        state.
 *                      - @a TIME_IMMEDIATE this value is not allowed.
 *
 * @returns             time to sleep in order to match period
 *
 * @api
 */
systime_t chThdRemainingPeriod(systime_t *previous, systime_t period)
{
    chDbgCheck(period != TIME_INFINITE &&
            previous != NULL, "chThdRemainingPeriod");

    systime_t future = *previous + period;
    systime_t now = chTimeNow();

    bool mustDelay =
        now < *previous ?
        (now < future && future < *previous) :
        (now < future || future < *previous);

    if (mustDelay)
        return future - now;
    else
        return TIME_IMMEDIATE;
}

#if CH_USE_EVENTS || defined(__DOXYGEN__)

/**
 * @brief   Waits for any of the specified events.
 * @details The function waits for any event among those specified in
 *          @p mask to become pending then the events are cleared and
 *          returned.
 *
 * @param[in] mask      mask of the event flags that the function should wait
 *                      for, @p ALL_EVENTS enables all the events
 * @param[in] time      the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE this value is not allowed.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The mask of the served and cleared events.
 * @retval 0            if the operation has timed out.
 *
 * @api
 */
eventmask_t chEvtWaitAnyPeriod(eventmask_t mask, systime_t *previous,
        systime_t period)
{
    chDbgCheck(period != TIME_INFINITE &&
            previous != NULL, "chEvtWaitAnyPeriod");

    Thread *ctp = currp;
    eventmask_t m;
    systime_t future = *previous + period;

    chSysLock();

    /* Check if event is already pending. */
    if ((m = (ctp->p_epending & mask)) != 0)
    {
        ctp->p_epending &= ~m;
        chSysUnlock();
        /* If we are woken because of an event, do not update previous time. */
        return m;
    }

    systime_t now = chTimeNow();

    bool mustDelay =
        now < *previous ?
        (now < future && future < *previous) :
        (now < future || future < *previous);

    if (mustDelay)
    {
        ctp->p_u.ewmask = mask;
        if (chSchGoSleepTimeoutS(THD_STATE_WTOREVT, future - now) < RDY_OK)
        {
            chSysUnlock();
            /* Update previous time because of timeout. */
            *previous = future;
            return (eventmask_t)0;
        }
        m = ctp->p_epending & mask;
        ctp->p_epending &= ~m;

        chSysUnlock();
        /* If we are woken because of an event, do not update previous time. */
        return m;
    }

    chSysUnlock();
    /* Update previous time because of timeout. */
    *previous = future;
    return (eventmask_t)0;
}

#endif /* CH_USE_EVENTS || defined(__DOXYGEN__) */
