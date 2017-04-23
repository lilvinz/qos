/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

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
    .stream_start = (void (*)(void*, coord_t, coord_t, coord_t, coord_t))
            gdsimStreamStart,
    .stream_write =
            (void (*)(void*, const color_t[], size_t))gdsimStreamWrite,
    .stream_end = (void (*)(void*))gdsimStreamEnd,
    .rect_fill = (void (*)(void*, coord_t, coord_t, coord_t, coord_t, color_t))
            gdsimRectFill,
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
    gdsimp->state = GD_READY;
    chSysUnlock();
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
    gdsimp->state = GD_STOP;
    chSysUnlock();
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
    gdsim_lld_flush(gdsimp, x, y, 1, 1);
}

/**
 * @brief   Starts streamed writing into a window.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 * @param[in] left      left window border coordinate
 * @param[in] top       top window border coordinate
 * @param[in] width     height of the window
 * @param[in] height    width of the window
 *
 * @api
 */
void gdsimStreamStart(GDSimDriver* gdsimp, coord_t left, coord_t top,
        coord_t width, coord_t height)
{
    chDbgCheck(gdsimp != NULL, "gdsimStreamStart");
    /* Verify device status. */
    chDbgAssert(gdsimp->state >= GD_READY, "gdsimStreamStart(), #1",
            "invalid state");

    gdsimp->state = GD_ACTIVE;

    gdsimp->stream_left = left;
    gdsimp->stream_top = top;
    gdsimp->stream_width = width;
    gdsimp->stream_height = height;
    gdsimp->stream_pos = 0;
}

/**
 * @brief   Write a chunk of data in stream mode.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 * @param[in] data[]    array of color_t data
 * @param[in] n         number of array elements
 *
 * @api
 */
void gdsimStreamWrite(GDSimDriver* gdsimp, const color_t data[], size_t n)
{
    chDbgCheck(gdsimp != NULL, "gdsimStreamWrite");
    /* Verify device status. */
    chDbgAssert(gdsimp->state >= GD_ACTIVE, "gdsimStreamWrite(), #1",
            "invalid state");

    for (size_t i = 0; i < n; ++i)
    {
        gdsim_lld_pixel_set(gdsimp,
                gdsimp->stream_left + gdsimp->stream_pos % gdsimp->stream_width,
                gdsimp->stream_top + gdsimp->stream_pos / gdsimp->stream_width,
                data[i]);

        ++gdsimp->stream_pos;
    }
}

/**
 * @brief   End stream mode writing.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 *
 * @api
 */
void gdsimStreamEnd(GDSimDriver* gdsimp)
{
    chDbgCheck(gdsimp != NULL, "gdsimStreamEnd");
    /* Verify device status. */
    chDbgAssert(gdsimp->state >= GD_ACTIVE, "gdsimStreamEnd(), #1",
            "invalid state");

    gdsim_lld_flush(gdsimp, gdsimp->stream_left, gdsimp->stream_top,
            gdsimp->stream_width, gdsimp->stream_height);

    gdsimp->state = GD_READY;
}

/**
 * @brief   Fills a rectangle with a color.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 * @param[in] left      left rectangle border coordinate
 * @param[in] top       top rectangle border coordinate
 * @param[in] width     height of the rectangle
 * @param[in] height    width of the rectangle
 * @param[in] color     color to fill rectangle with
 *
 * @api
 */
void gdsimRectFill(GDSimDriver* gdsimp, coord_t left, coord_t top,
        coord_t width, coord_t height, color_t color)
{
    chDbgCheck(gdsimp != NULL, "gdsimRectFill");
    /* Verify device status. */
    chDbgAssert(gdsimp->state >= GD_READY, "gdsimRectFill(), #1",
            "invalid state");

    gdsim_lld_rect_fill(gdsimp, left, top, width, height, color);
    gdsim_lld_flush(gdsimp, left, top, width, height);
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
