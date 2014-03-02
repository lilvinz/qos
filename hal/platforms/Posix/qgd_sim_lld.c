/**
 * @file    Posix/qwdg.c
 * @brief   Posix low level graphics display simulation driver code.
 *
 * @addtogroup GD_SIM
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_GD_SIM || defined(__DOXYGEN__)

#include <xcb/xcb_icccm.h>
#include <xcb/xcb_atom.h>

#include <string.h>

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers and threads.                                    */
/*===========================================================================*/

static msg_t gdsim_lld_pump(void* p)
{
    GDSimDriver* gdsimp = (GDSimDriver*)p;
    chRegSetThreadName("gdsim_lld_pump");

    while (gdsimp->exit_pump == false)
    {
        xcb_generic_event_t* e;

        while ((e = xcb_poll_for_event(gdsimp->xcb_connection)) != NULL)
        {
            switch (e->response_type & ~0x80)
            {
                case XCB_EXPOSE:
                {
                    break;
                }
                default:
                {
                    /* Unknown event type, ignore it. */
                    break;
                }
            }
            /* Free the generic event. */
            free(e);
        }

        chThdSleepMilliseconds(1);
    }

    return 0;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level graphics display simulation driver initialization.
 *
 * @notapi
 */
void gdsim_lld_init(void)
{
}

/**
 * @brief   Low level graphics display simulation driver initialization.
 *
 * @notapi
 */
void gdsim_lld_object_init(GDSimDriver* gdsimp)
{
    gdsimp->thd_ptr = NULL;
    gdsimp->exit_pump = true;

    /* Filling the thread working area here because the function
       @p chThdCreateI() does not do it. */
#if CH_DBG_FILL_THREADS
    {
        void *wsp = gdsimp->wa_pump;
        _thread_memfill((uint8_t*)wsp,
                (uint8_t*)wsp + sizeof(Thread),
                CH_THREAD_FILL_VALUE);
        _thread_memfill((uint8_t*)wsp + sizeof(Thread),
                (uint8_t*)wsp + sizeof(gdsimp->wa_pump) - sizeof(Thread),
                CH_STACK_FILL_VALUE);
    }
#endif
}

/**
 * @brief   Configures and activates the graphics display simulation peripheral.
 *
 * @param[in] gdsimp    pointer to the @p GDSimDriver object
 *
 * @notapi
 */
void gdsim_lld_start(GDSimDriver* gdsimp)
{
    if (gdsimp->state == GD_STOP)
    {
        gdsimp->xcb_connection = xcb_connect(NULL, NULL);

        /* Get the first screen. */
        gdsimp->xcb_screen = xcb_setup_roots_iterator(
                xcb_get_setup(gdsimp->xcb_connection)).data;

        /* Ask for our window's id. */
        gdsimp->xcb_window = xcb_generate_id(gdsimp->xcb_connection);

        /* Create the window. */
        xcb_create_window(gdsimp->xcb_connection,
                XCB_COPY_FROM_PARENT,               /* depth (same as root)  */
                gdsimp->xcb_window,                 /* window Id             */
                gdsimp->xcb_screen->root,           /* parent window         */
                0, 0,                               /* x, y                  */
                gdsimp->config->size_x,             /* width                 */
                gdsimp->config->size_y,             /* height                */
                10,                                 /* border_width          */
                XCB_WINDOW_CLASS_INPUT_OUTPUT,      /* class                 */
                gdsimp->xcb_screen->root_visual,    /* visual                */
                0, NULL);                           /* masks, not used yet   */

        /* Set title on window. */
        xcb_icccm_set_wm_name(gdsimp->xcb_connection,
                gdsimp->xcb_window,
                XCB_ATOM_STRING, 8, strlen("gdsim"), "gdsim");

        /* Set size hints on window. */
        xcb_size_hints_t hints;
        xcb_icccm_size_hints_set_max_size(&hints, gdsimp->config->size_x,
                gdsimp->config->size_y);
        xcb_icccm_size_hints_set_min_size(&hints, gdsimp->config->size_x,
                gdsimp->config->size_y);
        xcb_icccm_set_wm_size_hints(gdsimp->xcb_connection, gdsimp->xcb_window,
                XCB_ATOM_WM_NORMAL_HINTS, &hints);

        /* Ask for our graphical context id. */
        gdsimp->xcb_gcontext = xcb_generate_id(gdsimp->xcb_connection);

        /* Create a black graphic context for drawing in the foreground. */
        xcb_create_gc(gdsimp->xcb_connection,
                gdsimp->xcb_gcontext,
                gdsimp->xcb_window,
                XCB_GC_FOREGROUND,
                (uint32_t[]){ gdsimp->xcb_screen->black_pixel });

        /* Map the window on the screen. */
        xcb_map_window(gdsimp->xcb_connection, gdsimp->xcb_window);

        /* Make sure commands are sent before we pause, so window is shown. */
        xcb_flush(gdsimp->xcb_connection);

        /* Creates the data pump threads in a suspended state. Note, it is
         * created only once, the first time @p gdsimStart() is invoked. */
        gdsimp->exit_pump = false;
        gdsimp->thd_ptr = chThdCreateI(gdsimp->wa_pump, sizeof(gdsimp->wa_pump),
                GD_SIM_THREAD_PRIO, gdsim_lld_pump, gdsimp);
        chThdResumeI(gdsimp->thd_ptr);
    }
}

/**
 * @brief   Deactivates the graphics display simulation peripheral.
 *
 * @param[in] gdsimp    pointer to the @p GDSimDriver object
 *
 * @notapi
 */
void gdsim_lld_stop(GDSimDriver* gdsimp)
{
    if (gdsimp->state == GD_READY)
    {
        gdsimp->exit_pump = true;

        /* Wait for pump thread to exit. */
        chThdWait(gdsimp->thd_ptr);

        xcb_disconnect(gdsimp->xcb_connection);
    }
}

/**
 * @brief   Sets a pixel to a color.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 * @param[in] x         x coordinates
 * @param[in] y         y coordinates
 * @param[in] color     desired pixel color
 *
 * @notapi
 */
void gdsim_lld_pixel_set(GDSimDriver* gdsimp, coord_t x, coord_t y,
        color_t color)
{
    xcb_change_gc(gdsimp->xcb_connection, gdsimp->xcb_gcontext,
            XCB_GC_FOREGROUND, (uint32_t[]){ color });

    const xcb_point_t point =
    {
        .x = x,
        .y = y,
    };

    xcb_poly_point(gdsimp->xcb_connection,
            XCB_COORD_MODE_ORIGIN,
            gdsimp->xcb_window,
            gdsimp->xcb_gcontext,
            1,
            &point);

    xcb_flush(gdsimp->xcb_connection);
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
bool_t gdsim_lld_get_info(GDSimDriver* gdsimp, GDDeviceInfo* gddip)
{
    gddip->size_x = gdsimp->config->size_x;
    gddip->size_y = gdsimp->config->size_y;

    return CH_SUCCESS;
}

#endif /* HAL_USE_GD_SIM */

/** @} */
