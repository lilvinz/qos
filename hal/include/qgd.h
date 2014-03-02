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

/**
 * @brief   Driver state machine possible states.
 */
typedef enum
{
    GD_UNINIT = 0,                 /**< Not initialized.                   */
    GD_STOP = 1,                   /**< Stopped.                           */
    GD_READY = 2,                  /**< Device ready.                      */
    GD_POWERSAVE = 3,              /**< Device in power saving mode.       */
} gdstate_t;

/**
 * @brief   The type for a coordinate.
 */
typedef uint16_t coord_t;

/**
 * @brief   The type for a color.
 */
typedef uint32_t color_t;

/**
 * @brief   Graphics display device info.
 */
typedef struct
{
    coord_t size_x;
    coord_t size_y;
} GDDeviceInfo;

/**
 * @brief   @p BaseGDDevice specific methods.
 */
#define _base_gd_device_methods                                               \
    void (*pixel_set)(void *instance, coord_t x, coord_t y, color_t color);   \
    bool_t (*get_info)(void *instance, GDDeviceInfo *gddip);                  \
    /* End of mandatory functions. */                                         \
    /* Acquire device if supported by underlying driver.*/                    \
    void (*acquire)(void *instance);                                          \
    /* Release device if supported by underlying driver.*/                    \
    void (*release)(void *instance);

/**
 * @brief   @p BaseGDDevice specific data.
 */
#define _base_gd_device_data                                                 \
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
 * @brief   Returns a media information structure.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 * @param[out] gddip    pointer to a @p GDDeviceInfo structure
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
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

/** @} */
