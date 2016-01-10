/**
 * @file    posix/wdg_lld.c
 * @brief   Posix low level WDG driver header.
 *
 * @addtogroup WDG
 * @{
 */

#ifndef _WDG_LLD_H_
#define _WDG_LLD_H_

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
 * @brief   Type of a structure representing an WDG driver.
 */
typedef struct WDGDriver WDGDriver;

/**
 * @brief   Driver configuration structure.
 * @note    It could be empty on some architectures.
 */
typedef struct
{
} WDGConfig;

/**
 * @brief   Structure representing an WDG driver.
 */
struct WDGDriver
{
    /**
     * @brief Driver state.
     */
    wdgstate_t state;
    /**
    * @brief Current configuration data.
    */
    const WDGConfig* config;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

extern WDGDriver WDGD1;

#ifdef __cplusplus
extern "C" {
#endif
    void wdg_lld_init(void);
    void wdg_lld_start(WDGDriver* wdgp);
    void wdg_lld_stop(WDGDriver* wdgp);
    void wdg_lld_reset(WDGDriver* wdgp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_WDG */

#endif /* _WDG_LLD_H_ */

/** @} */
