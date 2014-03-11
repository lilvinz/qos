/**
 * @file    qgraphics_display_sim.h
 * @brief   Graphics display simulation header.
 *
 * @addtogroup GD_SIM
 * @{
 */

#ifndef _QGD_SIM_H_
#define _QGD_SIM_H_

#if HAL_USE_GD_SIM || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    GD_SIM configuration options
 * @{
 */

/**
 * @brief   Enables the @p gdsimAcquireBus() and @p gdsimReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(GD_SIM_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define GD_SIM_USE_MUTUAL_EXCLUSION       TRUE
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if GD_SIM_USE_MUTUAL_EXCLUSION &&                                            \
    (!CH_USE_MUTEXES && !CH_USE_SEMAPHORES)
#error "GD_SIM_USE_MUTUAL_EXCLUSION requires "                                \
    "CH_USE_MUTEXES and/or CH_USE_SEMAPHORES"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

#include "qgd_sim_lld.h"

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
    void gdsimInit(void);
    void gdsimObjectInit(GDSimDriver* gdsimp);
    void gdsimStart(GDSimDriver* gdsimp, const GDSimConfig* config);
    void gdsimStop(GDSimDriver* gdsimp);
    void gdsimPixelSet(GDSimDriver* gdsimp, coord_t x, coord_t y, color_t color);
    void gdsimStreamStart(GDSimDriver* gdsimp, coord_t left, coord_t top,
            coord_t width, coord_t height);
    void gdsimStreamWrite(GDSimDriver* gdsimp, const color_t data[], size_t n);
    void gdsimStreamEnd(GDSimDriver* gdsimp);
    void gdsimRectFill(GDSimDriver* gdsimp, coord_t left, coord_t top,
            coord_t width, coord_t height, color_t color);
    bool_t gdsimGetInfo(GDSimDriver* gdsimp, GDDeviceInfo* gddip);
    void gdsimAcquireBus(GDSimDriver* gdsimp);
    void gdsimReleaseBus(GDSimDriver* gdsimp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_GD_SIM */

#endif /* _QGD_SIM_H_ */

/** @} */
