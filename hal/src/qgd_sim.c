/**
 * @file    qgd_sim.c
 * @brief   Graphics display simulation code.
 *
 * @addtogroup GD_SIM
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_GD_SIM || defined(__DOXYGEN__)

/*
 * @todo    -
 *
 */

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Virtual methods table.
 */
static const struct GDSimDriverVMT gd_sim_vmt =
{
    .pixel_set = (void (*)(void*, coord_t, coord_t, color_t))gdsimPixelSet,
    .get_info = (bool_t (*)(void*, GDDeviceInfo*))gdsimGetInfo,
    .acquire = (void (*)(void*))gdsimAcquireBus,
    .release = (void (*)(void*))gdsimReleaseBus,
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Grahics display simulation via window.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void gdsimInit(void)
{
    gdsim_lld_init();
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] gdsimp   pointer to the @p GDSimDriver object
 *
 * @init
 */
void gdsimObjectInit(GDSimDriver* gdsimp)
{
    gdsimp->vmt = &gd_sim_vmt;
    gdsimp->state = GD_STOP;
    gdsimp->config = NULL;
#if GD_SIM_USE_MUTUAL_EXCLUSION
#if CH_USE_MUTEXES
    chMtxInit(&gdsimp->mutex);
#else
    chSemInit(&gdsimp->semaphore, 1);
#endif
#endif /* GD_SIM_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the graphics display simulation.
 *
 * @param[in] gdsimp    pointer to the @p GDSimDriver object
 * @param[in] config    pointer to the @p GDSimConfig object.
 *
 * @api
 */
void gdsimStart(GDSimDriver* gdsimp, const GDSimConfig* config)
{
    chDbgCheck((gdsimp != NULL) && (config != NULL), "gdsimStart");
    /* Verify device status. */
    chDbgAssert((gdsimp->state == GD_STOP) || (gdsimp->state == GD_READY),
            "gdsimStart(), #1", "invalid state");

    if (gdsimp->state == GD_READY)
        gdsimStop(gdsimp);

    gdsimp->config = config;

    chSysLock();
    gdsim_lld_start(gdsimp);
    chSysUnlock();

    gdsimp->state = GD_READY;
}

/**
 * @brief   Disables the graphics display peripheral.
 *
 * @param[in] gdsimp    pointer to the @p GDSimDriver object
 *
 * @api
 */
void gdsimStop(GDSimDriver* gdsimp)
{
    chDbgCheck(gdsimp != NULL, "gdsimStop");
    /* Verify device status. */
    chDbgAssert((gdsimp->state == GD_STOP) || (gdsimp->state == GD_READY),
            "gdsimStop(), #1", "invalid state");

    chSysLock();
    gdsim_lld_stop(gdsimp);
    chSysUnlock();

    gdsimp->state = GD_STOP;
}

/**
 * @brief   Sets pixel color.
 *
 * @param[in] gdsimp        pointer to the @p GDSimDriver object
 * @param[in] gddip        pointer to a @p GDDeviceInfo structure
 *
 * @api
 */
void gdsimPixelSet(GDSimDriver* gdsimp, coord_t x, coord_t y, color_t color)
{
    chDbgCheck(gdsimp != NULL, "gdPixelSet");
    /* Verify device status. */
    chDbgAssert(gdsimp->state >= GD_READY, "gdPixelSet(), #1",
            "invalid state");

    gdsim_lld_pixel_set(gdsimp, x, y, color);
}

/**
 * @brief   Returns device info.
 *
 * @param[in] gdsimp        pointer to the @p GDSimDriver object
 * @param[out] gddip        pointer to a @p GDDeviceInfo structure
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t gdsimGetInfo(GDSimDriver* gdsimp, GDDeviceInfo* gddip)
{
    chDbgCheck(gdsimp != NULL, "gdsimGetInfo");
    /* Verify device status. */
    chDbgAssert(gdsimp->state >= GD_READY, "gdsimGetInfo(), #1",
            "invalid state");

    chSysLock();
    gdsim_lld_get_info(gdsimp, gddip);
    chSysUnlock();

    return CH_SUCCESS;
}

/**
 * @brief   Gains exclusive access to the graphics display device.
 * @details This function tries to gain ownership to the graphics display
 *          device, if the device is already being used then the invoking
 *          thread is queued.
 * @pre     In order to use this function the option
 *          @p GD_SIM_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] gdsimp    pointer to the @p GDSimDriver object
 *
 * @api
 */
void gdsimAcquireBus(GDSimDriver* gdsimp)
{
    chDbgCheck(gdsimp != NULL, "gdsimAcquireBus");

#if GD_SIM_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    chMtxLock(&gdsimp->mutex);
#elif CH_USE_SEMAPHORES
    chSemWait(&gdsimp->semaphore);
#endif
#endif /* GD_SIM_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the graphics display device.
 * @pre     In order to use this function the option
 *          @p GD_SIM_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] gdsimp    pointer to the @p GDSimDriver object
 *
 * @api
 */
void gdsimReleaseBus(GDSimDriver* gdsimp)
{
    chDbgCheck(gdsimp != NULL, "gdsimReleaseBus");

#if GD_SIM_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    (void)gdsimp;
    chMtxUnlock();
#elif CH_USE_SEMAPHORES
    chSemSignal(&gdsimp->semaphore);
#endif
#endif /* GD_SIM_USE_MUTUAL_EXCLUSION */
}

#endif /* HAL_USE_GD_SIM */

/** @} */
