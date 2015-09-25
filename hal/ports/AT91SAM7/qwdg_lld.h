/**
 * @file    AT91SAM7/qwdg_lld.c
 * @brief   AT91SAM7 low level WDG driver header.
 *
 * @addtogroup WDG
 * @{
 */

#ifndef _QWDG_LLD_H_
#define _QWDG_LLD_H_

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
 * @brief   WDG internal driver configuration structure.
 */
typedef struct
{
    uint32_t wdt_mr;

} WDGConfig;

/**
 * @brief   Structure representing a WDG driver.
 */
typedef struct
{
    /**
     * @brief Driver state.
     */
    wdgstate_t state;
    /**
    * @brief Current configuration data.
    */
    const WDGConfig* config;
    /**
     * @brief Pointer to the WDG registers block.
     */
    AT91S_WDTC* wdg;
} WDGDriver;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

extern WDGDriver WDGD;

#ifdef __cplusplus
extern "C" {
#endif
    void wdg_lld_init(void);
    void wdg_lld_start(WDGDriver* wdgp);
    void wdg_lld_stop(WDGDriver* wdgp);
    void wdg_lld_reload(WDGDriver* wdgp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_WDG */

#endif /* _QWDG_LLD_H_ */

/** @} */
