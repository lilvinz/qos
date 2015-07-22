/**
 * @file    Posix/qgd_sim.c
 * @brief   Posix low level graphics display simulation driver header.
 *
 * @addtogroup GD_SIM
 * @{
 */

#ifndef _QGD_SIM_LLD_H_
#define _QGD_SIM_LLD_H_

#if HAL_USE_GD_SIM || defined(__DOXYGEN__)

#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Dedicated data pump threads priority.
 */
#if !defined(STM32_USB_OTG_THREAD_PRIO) || defined(__DOXYGEN__)
#define GD_SIM_THREAD_PRIO                  LOWPRIO
#endif

/**
 * @brief   Dedicated data pump threads stack size.
 */
#if !defined(STM32_USB_OTG_THREAD_STACK_SIZE) || defined(__DOXYGEN__)
#define GD_SIM_THREAD_STACK_SIZE            2048
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !CH_USE_WAITEXIT
#error "GD_SIM_USE_MUTUAL_EXCLUSION requires CH_USE_WAITEXIT"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Non volatile memory emulation driver configuration structure.
 */
typedef struct
{
    /**
     * @brief Width of the simulated display.
     */
    coord_t size_x;
    /**
     * @brief Height of the simulated display.
     */
    coord_t size_y;
} GDSimConfig;

/**
 * @brief   @p GDSimDriver specific methods.
 */
#define _gd_sim_driver_methods                                                \
    _base_gd_device_methods

/**
 * @extends BaseGDDeviceVMT
 *
 * @brief   @p GDSimDriver virtual methods table.
 */
struct GDSimDriverVMT
{
    _gd_sim_driver_methods
};

/**
 * @extends BaseGDDevice
 *
 * @brief   Structure representing a simulated graphics display driver.
 */
typedef struct
{
    /**
    * @brief Virtual Methods Table.
    */
    const struct GDSimDriverVMT* vmt;
    _base_gd_device_data
    /**
    * @brief Current configuration data.
    */
    const GDSimConfig* config;
#if GD_SIM_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief Mutex protecting the device.
     */
    Mutex mutex;
#elif CH_USE_SEMAPHORES
    Semaphore semaphore;
#endif
#endif /* GD_SIM_USE_MUTUAL_EXCLUSION */
    /**
     * @brief   Pointer to the thread.
     */
    Thread* thd_ptr;
    Thread* thd_wait;
    /**
     * @brief   Working area for the dedicated data pump thread;
     */
    WORKING_AREA(wa_pump, GD_SIM_THREAD_STACK_SIZE);
    /**
     * @brief   xcb stuff
     */
    xcb_connection_t* xcb_connection;
    xcb_screen_t* xcb_screen;
    xcb_window_t xcb_window;
    xcb_gcontext_t xcb_gcontext;
    xcb_pixmap_t xcb_pixmap;
    xcb_image_t* xcb_image;
    /**
     * @brief   Stream write state.
     */
    coord_t stream_left;
    coord_t stream_top;
    coord_t stream_width;
    coord_t stream_height;
    size_t stream_pos;
} GDSimDriver;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
    void gdsim_lld_init(void);
    void gdsim_lld_object_init(GDSimDriver* gdsimp);
    void gdsim_lld_start(GDSimDriver* gdsimp);
    void gdsim_lld_stop(GDSimDriver* gdsimp);
    void gdsim_lld_pixel_set(GDSimDriver* gdsimp, coord_t x, coord_t y,
            color_t color);
    void gdsim_lld_rect_fill(GDSimDriver* gdsimp, coord_t left, coord_t top,
            coord_t width, coord_t height, color_t color);
    bool_t gdsim_lld_get_info(GDSimDriver* gdsimp, GDDeviceInfo* gddip);
    void gdsim_lld_flush(GDSimDriver* gdsimp, coord_t left, coord_t top,
            coord_t width, coord_t height);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_GD_SIM */

#endif /* _QGD_SIM_LLD_H_ */

/** @} */
