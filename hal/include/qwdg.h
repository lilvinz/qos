/**
 * @file    qwdg.h
 * @brief   MCU internal watchdog driver header.
 *
 * @addtogroup WDG
 * @{
 */

#ifndef _QWDG_H_
#define _QWDG_H_

#if HAL_USE_WDG || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum
{
    WDG_UNINIT = 0,                  /**< Not initialized.                   */
    WDG_STOP = 1,                    /**< Stopped.                           */
    WDG_READY = 2,                   /**< Ready.                             */
} wdgstate_t;

#include "qwdg_lld.h"

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
    void wdgInit(void);
    void wdgObjectInit(WDGDriver* wdgp);
    void wdgStart(WDGDriver* wdgp, const WDGConfig* config);
    void wdgStop(WDGDriver* wdgp);
    void wdgReload(WDGDriver* wdgp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_WDG */

#endif /* _QWDG_H_ */

/** @} */
