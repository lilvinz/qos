/**
 * @file    qgraphics_display.h
 * @brief   Generic display devices access.
 * @details This header defines an abstract interface useful to access generic
 *          graphics display devices in a standardized way.
 *
 * @addtogroup GRAPHICS_DISPLAY
 * @details This module defines an abstract interface for accessing generic
 *          graphics display devices devices.
 *          Note that no code is present, just abstract interfaces-like
 *          structures, you should look at the system as to a set of
 *          abstract C++ classes (even if written in C). This system
 *          has then advantage to make the access to display devices
 *          independent from the implementation logic.
 * @{
 */

#ifndef _QGPRAHICS_DISPLAY_H_
#define _QGPRAHICS_DISPLAY_H_

#include <stdint.h>

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Supported pixel color formats
 */
#define GD_COLORFORMAT_ARGB8888 1
#define GD_COLORFORMAT_RGB888 2
#define GD_COLORFORMAT_RGB666 3
#define GD_COLORFORMAT_RGB565 4

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    GD configuration options
 * @{
 */

#if !defined(GD_COLORFORMAT)
#define GD_COLORFORMAT GD_COLORFORMAT_RGB565
#endif /* !defined(GD_COLORFORMAT) */

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   The type for a coordinate.
 */
typedef uint16_t coord_t;

/**
 * @brief   The type for a color.
 */
#if (GD_COLORFORMAT == GD_COLORFORMAT_ARGB8888) || \
        (GD_COLORFORMAT == GD_COLORFORMAT_RGB888) || \
        (GD_COLORFORMAT == GD_COLORFORMAT_RGB666)
typedef uint32_t color_t;
#elif GD_COLORFORMAT == GD_COLORFORMAT_RGB565
typedef uint16_t color_t;
#else
#error "Unsupported pixel color format"
#endif

/**
 * @brief   Driver state machine possible states.
 */
typedef enum
{
    GD_UNINIT = 0,                  /**< Not initialized.                   */
    GD_STOP = 1,                    /**< Stopped.                           */
    GD_SLEEP = 2,                   /**< Device in power saving mode.       */
    GD_READY = 3,                   /**< Device ready.                      */
    GD_ACTIVE= 4,                   /**< Device transferring data.          */
} gdstate_t;

/**
 * @brief   Graphics display device info.
 */
typedef struct
{
    coord_t size_x;
    coord_t size_y;
    uint8_t id[3];
} GDDeviceInfo;

/**
 * @brief   @p BaseGDDevice specific methods.
 */
#define _base_gd_device_methods                                               \
    void (*pixel_set)(void *instance, coord_t x, coord_t y, color_t color);   \
    void (*stream_start)(void *instance, coord_t left, coord_t top,           \
            coord_t width, coord_t height);                                   \
    void (*stream_write)(void *instance, const color_t data[], size_t n);     \
    void (*stream_color)(void *instance, const color_t color, uint16_t);      \
    void (*stream_end)(void *instance);                                       \
    void (*rect_fill)(void *instance, coord_t left, coord_t top,              \
            coord_t width, coord_t height, color_t color);                    \
    bool (*get_info)(void *instance, GDDeviceInfo *gddip);                    \
    /* End of mandatory functions. */                                         \
    /* Acquire device if supported by underlying driver.*/                    \
    void (*acquire)(void *instance);                                          \
    /* Release device if supported by underlying driver.*/                    \
    void (*release)(void *instance);

/**
 * @brief   @p BaseGDDevice specific data.
 */
#define _base_gd_device_data                                                  \
    /* Driver state.*/                                                        \
    gdstate_t state;

/**
 * @brief   @p BaseGDDevice virtual methods table.
 */
struct BaseGDDeviceVMT
{
    _base_gd_device_methods
};

/**
 * @brief   Base gd device class.
 * @details This class represents a generic, block-accessible, device.
 */
typedef struct
{
    /** @brief Virtual Methods Table.*/
    const struct BaseGDDeviceVMT* vmt;
    _base_gd_device_data
} BaseGDDevice;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Macro Functions (BaseGDDevice)
 * @{
 */

/**
 * @brief   Returns the driver state.
 * @note    Can be called in ISR context.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 *
 * @return              The driver state.
 *
 * @special
 */
#define gdGetDriverState(ip) ((ip)->state)

/**
 * @brief   Sets a pixel to a color.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 * @param[in] x         x coordinates
 * @param[in] y         y coordinates
 * @param[in] color     desired pixel color
 *
 * @api
 */
#define gdPixelSet(ip, x, y, color) ((ip)->vmt->pixel_set(ip, x, y, color))

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
#define gdStreamStart(ip, left, top, width, height) \
    ((ip)->vmt->stream_start(ip, left, top, width, height))

/**
 * @brief   Write a chunk of data in stream mode.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 * @param[in] data[]    array of color_t data
 * @param[in] n         number of array elements
 *
 * @api
 */
#define gdStreamWrite(ip, data, n) ((ip)->vmt->stream_write(ip, data, n))

/**
 * @brief   Write a color n times in stream mode.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 * @param[in] color     color to write
 * @param[in] n         number of times to write color
 *
 * @api
 */
#define gdStreamColor(ip, color, n) ((ip)->vmt->stream_color(ip, color, n))

/**
 * @brief   End stream mode writing.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 *
 * @api
 */
#define gdStreamEnd(ip) ((ip)->vmt->stream_end(ip))


/**
 * @brief   Fills a rectangle with a color.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 * @param[in] left      left rectangle border coordinate
 * @param[in] top       top rectangle border coordinate
 * @param[in] width     height of the rectangle
 * @param[in] height    width of the rectangle
 * @param[in] color     color to fill the rect with
 *
 * @api
 */
#define gdRectFill(ip, left, top, width, height, color) \
    ((ip)->vmt->rect_fill(ip, left, top, width, height, color))

/**
 * @brief   Returns a media information structure.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 * @param[out] gddip    pointer to a @p GDDeviceInfo structure
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  operation succeeded.
 * @retval HAL_FAILED   operation failed.
 *
 * @api
 */
#define gdGetInfo(ip, gddip) ((ip)->vmt->get_info(ip, gddip))

/**
 * @brief   Acquires device for exclusive access if implemented.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 *
 * @api
 */
#define gdAcquire(ip) ((ip)->vmt->acquire)(ip)

/**
 * @brief   Releases exclusive access from device if implemented.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 *
 * @api
 */
#define gdRelease(ip) ((ip)->vmt->release)(ip)

/** @} */

#endif /* _QGPRAHICS_DISPLAY_H_ */
