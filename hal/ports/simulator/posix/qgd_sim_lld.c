/**
 * @file    Posix/qwdg.c
 * @brief   Posix low level graphics display simulation driver code.
 *
 * @addtogroup GD_SIM
 * @{
 */

#include "qhal.h"

#if HAL_USE_GD_SIM || defined(__DOXYGEN__)

#include <xcb/xcb_icccm.h>
#include <xcb/xcb_atom.h>

#include <string.h>
#include <stdlib.h>

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

static uint32_t convert_color(color_t color)
{
#if (GD_COLORFORMAT == GD_COLORFORMAT_ARGB8888)
    return (uint32_t)color;
#elif GD_COLORFORMAT == GD_COLORFORMAT_RGB565
    return ((((color >> 11) << 3) & 0xff) << 16) |
            ((((color >> 5) << 2) & 0xff) << 8) |
            ((((color >> 0) << 3) & 0xff) << 0);
#else
#error "Unsupported pixel color format"
#endif
}

/*===========================================================================*/
/* Driver interrupt handlers and threads.                                    */
/*===========================================================================*/

/**
 * @brief   Drivers pump thread function.
 *
 * @param[in] parameters    pointer to a @p GDSimDriver object
 *
 * @notapi
 */
__attribute__((noreturn))
static void gdsim_lld_pump(void* p)
{
    GDSimDriver* gdsimp = (GDSimDriver*)p;

#if defined(_CHIBIOS_RT_)
    chRegSetThreadName("gdsim_lld_pump");
#endif

    while (true)
    {
        /* Nothing to do, going to sleep.*/
        osalSysLock();
        if (gdsimp->state == GD_STOP)
            chThdSuspendS(&gdsimp->wait);

        xcb_generic_event_t* e;
        while ((e = xcb_poll_for_event(gdsimp->xcb_connection)) != NULL)
        {
            switch (e->response_type)
            {
                case XCB_EXPOSE:
                {
                    xcb_expose_event_t* ee = (xcb_expose_event_t*)e;
                    xcb_copy_area(gdsimp->xcb_connection, gdsimp->xcb_pixmap,
                            gdsimp->xcb_window, gdsimp->xcb_gcontext,
                            ee->x, ee->y,
                            ee->x, ee->y,
                            ee->width, ee->height);
                    xcb_flush(gdsimp->xcb_connection);
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
        osalSysUnlock();

        osalThreadSleepMilliseconds(10);
    }
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
#if defined(_CHIBIOS_RT_)
    gdsimp->tr = NULL;
    gdsimp->wait = NULL;

    /* Filling the thread working area here because the function
       @p chThdCreateI() does not do it.*/
#if CH_DBG_FILL_THREADS
    {
        void *wsp = gdsimp->wa_pump;
        _thread_memfill((uint8_t *)wsp,
                (uint8_t *)wsp + sizeof(thread_t),
                CH_DBG_THREAD_FILL_VALUE);
        _thread_memfill((uint8_t *)wsp + sizeof(thread_t),
                (uint8_t *)wsp + sizeof(gdsimp->wa_pump),
                CH_DBG_STACK_FILL_VALUE);
    }
#endif /* CH_DBG_FILL_THREADS */
#endif /* defined(_CHIBIOS_RT_) */
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
                gdsimp->xcb_screen->root_depth,     /* depth (same as root)  */
                gdsimp->xcb_window,                 /* window id             */
                gdsimp->xcb_screen->root,           /* parent window         */
                0, 0,                               /* x, y                  */
                gdsimp->config->size_x,             /* width                 */
                gdsimp->config->size_y,             /* height                */
                10,                                 /* border_width          */
                XCB_WINDOW_CLASS_INPUT_OUTPUT,      /* class                 */
                gdsimp->xcb_screen->root_visual,    /* visual                */
                XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
                (uint32_t[]){ gdsimp->xcb_screen->black_pixel, XCB_EVENT_MASK_EXPOSURE });

        /* Set title on window. */
        xcb_icccm_set_wm_name(gdsimp->xcb_connection,
                gdsimp->xcb_window,
                XCB_ATOM_STRING, 8, strlen("gdsim"), "gdsim");

        /* Set size hints on window. */
        xcb_size_hints_t hints;
        xcb_icccm_size_hints_set_min_size(&hints, gdsimp->config->size_x,
                gdsimp->config->size_y);
        xcb_icccm_size_hints_set_max_size(&hints, gdsimp->config->size_x,
                gdsimp->config->size_y);
        xcb_icccm_set_wm_size_hints(gdsimp->xcb_connection, gdsimp->xcb_window,
                XCB_ATOM_WM_NORMAL_HINTS, &hints);

        /* Ask an id for our to be created pixmap. */
        gdsimp->xcb_pixmap = xcb_generate_id(gdsimp->xcb_connection);

        /* Create the pixmap. */
        xcb_create_pixmap(gdsimp->xcb_connection,
                gdsimp->xcb_screen->root_depth,     /* depth (same as root)  */
                gdsimp->xcb_pixmap,                 /* id of the pixmap      */
                gdsimp->xcb_window,
                gdsimp->config->size_x,
                gdsimp->config->size_y);

        /* Ask for our graphical context id. */
        gdsimp->xcb_gcontext = xcb_generate_id(gdsimp->xcb_connection);

        /* Create a black graphic context for drawing in the foreground. */
        xcb_create_gc(gdsimp->xcb_connection,
                gdsimp->xcb_gcontext,
                gdsimp->xcb_pixmap,
                XCB_GC_BACKGROUND,
                (uint32_t[]){ gdsimp->xcb_screen->black_pixel });

        /* Map the window on the screen. */
        xcb_map_window(gdsimp->xcb_connection, gdsimp->xcb_window);

        /* Make sure commands are sent before we pause, so window is shown. */
        xcb_flush(gdsimp->xcb_connection);

        /* Create image. */
        gdsimp->xcb_image = xcb_image_create(
                gdsimp->config->size_x,
                gdsimp->config->size_y,
                XCB_IMAGE_FORMAT_Z_PIXMAP,
                8,
                24,
                32,
                32,
                XCB_IMAGE_ORDER_MSB_FIRST,
                XCB_IMAGE_ORDER_MSB_FIRST,
                NULL,
                0,
                NULL);

#if defined(_CHIBIOS_RT_)
        /* Creates the data pump thread. Note, it is created only once.*/
        if (gdsimp->tr == NULL)
        {
            gdsimp->tr = chThdCreateI(gdsimp->wa_pump, sizeof gdsimp->wa_pump,
                    GD_SIM_THREAD_PRIO, gdsim_lld_pump, gdsimp);
            chThdStartI(gdsimp->tr);
        }
        else if (gdsimp->wait != NULL)
        {
            chThdResumeS(&gdsimp->wait, MSG_OK);
        }
        chSchRescheduleS();
#endif
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
    osalSysLock();

    xcb_image_put_pixel(gdsimp->xcb_image, x, y, convert_color(color));

    osalSysUnlock();
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
 * @notapi
 */
void gdsim_lld_rect_fill(GDSimDriver* gdsimp, coord_t left, coord_t top,
        coord_t width, coord_t height, color_t color)
{
    osalSysLock();

    for (coord_t y = top; y < top + height; ++y)
        for (size_t x = left; x < left + width; ++x)
            xcb_image_put_pixel(gdsimp->xcb_image, x, y, convert_color(color));

    osalSysUnlock();
}

/**
 * @brief   Returns device info.
 *
 * @param[in] gdsimp        pointer to the @p GDSimDriver object
 * @param[out] gddip        pointer to a @p GDDeviceInfo structure
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool gdsim_lld_get_info(GDSimDriver* gdsimp, GDDeviceInfo* gddip)
{
    gddip->size_x = gdsimp->config->size_x;
    gddip->size_y = gdsimp->config->size_y;

    return HAL_SUCCESS;
}

/**
 * @brief   Flushes the the x client buffer.
 *
 * @param[in] gdsimp    pointer to the @p GDSimDriver object
 *
 * @notapi
 */
void gdsim_lld_flush(GDSimDriver* gdsimp, coord_t left, coord_t top,
        coord_t width, coord_t height)
{
    osalSysLock();

    /* Create subimage for to be flushed area. */
    xcb_image_t* sub_image = xcb_image_subimage(
            gdsimp->xcb_image,
            left,
            top,
            width,
            height,
            0,
            0,
            0);

    /* Convert subimage to connection native format. */
    xcb_image_t* native_image =
            xcb_image_native(gdsimp->xcb_connection,
                    sub_image,
                    1);

    /* Draw it into the window. */
    xcb_image_put(gdsimp->xcb_connection,
            gdsimp->xcb_window,
            gdsimp->xcb_gcontext,
            native_image,
            left,
            top,
            0);

    /* Draw it into the pixmap (for restoring the window). */
    xcb_image_put(gdsimp->xcb_connection,
            gdsimp->xcb_pixmap,
            gdsimp->xcb_gcontext,
            native_image,
            left,
            top,
            0);

    /* Flush out all requests. */
    xcb_flush(gdsimp->xcb_connection);

    xcb_image_destroy(sub_image);
    xcb_image_destroy(native_image);

    osalSysUnlock();
}

#endif /* HAL_USE_GD_SIM */

/** @} */
