/**
 * @file    qgd_ili9341.h
 * @brief   Graphics display ILI9341 header.
 *
 * @addtogroup GD_ILI9341
 * @{
 */

#ifndef _QGD_ILI9341_H_
#define _QGD_ILI9341_H_

#if HAL_USE_GD_ILI9341 || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    GD_ILI9341 regulative commands
 * @{
 */
#define GD_ILI9341_CMD_NOP                 0x00    /**< No operation.*/
#define GD_ILI9341_CMD_RESET               0x01    /**< Software reset.*/
#define GD_ILI9341_GET_ID_INFO             0x04    /**< Get ID information.*/
#define GD_ILI9341_GET_STATUS              0x09    /**< Get status.*/
#define GD_ILI9341_GET_PWR_MODE            0x0A    /**< Get power mode.*/
#define GD_ILI9341_GET_MADCTL              0x0B    /**< Get MADCTL.*/
#define GD_ILI9341_GET_PIX_FMT             0x0C    /**< Get pixel format.*/
#define GD_ILI9341_GET_IMG_FMT             0x0D    /**< Get image format.*/
#define GD_ILI9341_GET_SIG_MODE            0x0E    /**< Get signal mode.*/
#define GD_ILI9341_GET_SELF_DIAG           0x0F    /**< Get self-diagnostics.*/
#define GD_ILI9341_CMD_SLEEP_ON            0x10    /**< Enter sleep mode.*/
#define GD_ILI9341_CMD_SLEEP_OFF           0x11    /**< Exit sleep mode.*/
#define GD_ILI9341_CMD_PARTIAL_ON          0x12    /**< Enter partial mode.*/
#define GD_ILI9341_CMD_PARTIAL_OFF         0x13    /**< Exit partial mode.*/
#define GD_ILI9341_CMD_INVERT_ON           0x20    /**< Enter inverted mode.*/
#define GD_ILI9341_CMD_INVERT_OFF          0x21    /**< Exit inverted mode.*/
#define GD_ILI9341_SET_GAMMA               0x26    /**< Set gamma params.*/
#define GD_ILI9341_CMD_DISPLAY_OFF         0x28    /**< Disable display.*/
#define GD_ILI9341_CMD_DISPLAY_ON          0x29    /**< Enable display.*/
#define GD_ILI9341_SET_COL_ADDR            0x2A    /**< Set column address.*/
#define GD_ILI9341_SET_PAGE_ADDR           0x2B    /**< Set page address.*/
#define GD_ILI9341_SET_MEM                 0x2C    /**< Set memory.*/
#define GD_ILI9341_SET_COLOR               0x2D    /**< Set color.*/
#define GD_ILI9341_GET_MEM                 0x2E    /**< Get memory.*/
#define GD_ILI9341_SET_PARTIAL_AREA        0x30    /**< Set partial area.*/
#define GD_ILI9341_SET_VSCROLL             0x33    /**< Set vertical scroll def.*/
#define GD_ILI9341_CMD_TEARING_ON          0x34    /**< Tearing line enabled.*/
#define GD_ILI9341_CMD_TEARING_OFF         0x35    /**< Tearing line disabled.*/
#define GD_ILI9341_SET_MEM_ACS_CTL         0x36    /**< Set mem access ctl.*/
#define GD_ILI9341_SET_VSCROLL_ADDR        0x37    /**< Set vscroll start addr.*/
#define GD_ILI9341_CMD_IDLE_OFF            0x38    /**< Exit idle mode.*/
#define GD_ILI9341_CMD_IDLE_ON             0x39    /**< Enter idle mode.*/
#define GD_ILI9341_SET_PIX_FMT             0x3A    /**< Set pixel format.*/
#define GD_ILI9341_SET_MEM_CONT            0x3C    /**< Set memory continue.*/
#define GD_ILI9341_GET_MEM_CONT            0x3E    /**< Get memory continue.*/
#define GD_ILI9341_SET_TEAR_SCANLINE       0x44    /**< Set tearing scanline.*/
#define GD_ILI9341_GET_TEAR_SCANLINE       0x45    /**< Get tearing scanline.*/
#define GD_ILI9341_SET_BRIGHTNESS          0x51    /**< Set brightness.*/
#define GD_ILI9341_GET_BRIGHTNESS          0x52    /**< Get brightness.*/
#define GD_ILI9341_SET_DISPLAY_CTL         0x53    /**< Set display ctl.*/
#define GD_ILI9341_GET_DISPLAY_CTL         0x54    /**< Get display ctl.*/
#define GD_ILI9341_SET_CABC                0x55    /**< Set CABC.*/
#define GD_ILI9341_GET_CABC                0x56    /**< Get CABC.*/
#define GD_ILI9341_SET_CABC_MIN            0x5E    /**< Set CABC min.*/
#define GD_ILI9341_GET_CABC_MIN            0x5F    /**< Set CABC max.*/
#define GD_ILI9341_GET_ID1                 0xDA    /**< Get ID1.*/
#define GD_ILI9341_GET_ID2                 0xDB    /**< Get ID2.*/
#define GD_ILI9341_GET_ID3                 0xDC    /**< Get ID3.*/
/** @} */

/**
 * @name    GD_ILI9341 extended commands
 * @{
 */
#define GD_ILI9341_SET_RGB_IF_SIG_CTL      0xB0    /**< RGB IF signal ctl.*/
#define GD_ILI9341_SET_FRAME_CTL_NORMAL    0xB1    /**< Set frame ctl (normal).*/
#define GD_ILI9341_SET_FRAME_CTL_IDLE      0xB2    /**< Set frame ctl (idle).*/
#define GD_ILI9341_SET_FRAME_CTL_PARTIAL   0xB3    /**< Set frame ctl (partial).*/
#define GD_ILI9341_SET_INVERSION_CTL       0xB4    /**< Set inversion ctl.*/
#define GD_ILI9341_SET_BLANKING_PORCH_CTL  0xB5    /**< Set blanking porch ctl.*/
#define GD_ILI9341_SET_FUNCTION_CTL        0xB6    /**< Set function ctl.*/
#define GD_ILI9341_SET_ENTRY_MODE          0xB7    /**< Set entry mode.*/
#define GD_ILI9341_SET_LIGHT_CTL_1         0xB8    /**< Set backlight ctl 1.*/
#define GD_ILI9341_SET_LIGHT_CTL_2         0xB9    /**< Set backlight ctl 2.*/
#define GD_ILI9341_SET_LIGHT_CTL_3         0xBA    /**< Set backlight ctl 3.*/
#define GD_ILI9341_SET_LIGHT_CTL_4         0xBB    /**< Set backlight ctl 4.*/
#define GD_ILI9341_SET_LIGHT_CTL_5         0xBC    /**< Set backlight ctl 5.*/
#define GD_ILI9341_SET_LIGHT_CTL_7         0xBE    /**< Set backlight ctl 7.*/
#define GD_ILI9341_SET_LIGHT_CTL_8         0xBF    /**< Set backlight ctl 8.*/
#define GD_ILI9341_SET_POWER_CTL_1         0xC0    /**< Set power ctl 1.*/
#define GD_ILI9341_SET_POWER_CTL_2         0xC1    /**< Set power ctl 2.*/
#define GD_ILI9341_SET_VCOM_CTL_1          0xC5    /**< Set VCOM ctl 1.*/
#define GD_ILI9341_SET_VCOM_CTL_2          0xC6    /**< Set VCOM ctl 2.*/
#define GD_ILI9341_SET_NVMEM               0xD0    /**< Set NVMEM data.*/
#define GD_ILI9341_GET_NVMEM_KEY           0xD1    /**< Get NVMEM protect key.*/
#define GD_ILI9341_GET_NVMEM_STATUS        0xD2    /**< Get NVMEM status.*/
#define GD_ILI9341_GET_ID4                 0xD3    /**< Get ID4.*/
#define GD_ILI9341_SET_PGAMMA              0xE0    /**< Set positive gamma.*/
#define GD_ILI9341_SET_NGAMMA              0xE1    /**< Set negative gamma.*/
#define GD_ILI9341_SET_DGAMMA_CTL_1        0xE2    /**< Set digital gamma ctl 1.*/
#define GD_ILI9341_SET_DGAMMA_CTL_2        0xE3    /**< Set digital gamma ctl 2.*/
#define GD_ILI9341_SET_IF_CTL              0xF6    /**< Set interface control.*/
/** @} */

/**
 * @name    GD_ILI9341 interface modes
 * @{
 */
#define GD_ILI9341_IM_3LSI_1               0x5     /**< 3-line serial, mode 1.*/
#define GD_ILI9341_IM_3LSI_2               0xD     /**< 3-line serial, mode 2.*/
#define GD_ILI9341_IM_4LSI_1               0x6     /**< 4-line serial, mode 1.*/
#define GD_ILI9341_IM_4LSI_2               0xE     /**< 4-line serial, mode 2.*/
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    GD_ILI9341 configuration options
 * @{
 */

/**
 * @brief   Enables the @p gdili9341AcquireBus() and @p gdili9341ReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(GD_ILI9341_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define GD_ILI9341_USE_MUTUAL_EXCLUSION         TRUE
#endif

/**
 * @brief   GD_ILI9341 Interface Mode
 */
#if !defined(GD_ILI9341_IM) || defined(__DOXYGEN__)
#define GD_ILI9341_IM                           GD_ILI9341_IM_4LSI_1
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if GD_ILI9341_USE_MUTUAL_EXCLUSION &&                                        \
    (!CH_USE_MUTEXES && !CH_USE_SEMAPHORES)
#error "GD_ILI9341_USE_MUTUAL_EXCLUSION requires "                            \
    "CH_USE_MUTEXES and/or CH_USE_SEMAPHORES"
#endif

#if GD_ILI9341_IM != GD_ILI9341_IM_4LSI_1 && \
    GD_ILI9341_IM != GD_ILI9341_IM_4LSI_2
#error "Only GD_ILI9341_IM_4LSI interface modes are supported currently"
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
     * @brief Width of the simulated display
     */
    coord_t size_x;
    /**
     * @brief Height of the simulated display
     */
    coord_t size_y;
    /**
     * @brief SPI driver used by ILI9341
     */
    SPIDriver *spip;
#if GD_ILI9341_IM == GD_ILI9341_IM_4LSI_1 || \
    GD_ILI9341_IM == GD_ILI9341_IM_4LSI_2
    /**
     * @brief <tt>D/!C</tt> signal port
     */
    ioportid_t dcx_port;
    /**
     * @brief <tt>D/!C</tt> signal pad
     */
    uint16_t dcx_pad;
#endif /* GD_ILI9341_IM */
} GDIL9341Config;

/**
 * @brief   @p GDIL9341Driver specific methods.
 */
#define _gd_ili9341_driver_methods                                            \
    _base_gd_device_methods

/**
 * @extends BaseGDDeviceVMT
 *
 * @brief   @p GDIL9341Driver virtual methods table.
 */
struct GDIL9341DriverVMT
{
    _gd_ili9341_driver_methods
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
    const struct GDIL9341DriverVMT* vmt;
    _base_gd_device_data
    /**
    * @brief Current configuration data.
    */
    const GDIL9341Config* config;
#if GD_ILI9341_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief Mutex protecting the device.
     */
    Mutex mutex;
#elif CH_USE_SEMAPHORES
    Semaphore semaphore;
#endif
#endif /* GD_ILI9341_USE_MUTUAL_EXCLUSION */
    /**
    * @brief Cached device info
    */
    GDDeviceInfo gddi;
    /**
     * @brief Non-stacked value, for SPI with CCM.
     */
    uint8_t value;
} GDIL9341Driver;

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
    void gdili9341Init(void);
    void gdili9341ObjectInit(GDIL9341Driver* gdili9341p);
    void gdili9341Start(GDIL9341Driver* gdili9341p,
            const GDIL9341Config* config);
    void gdili9341Stop(GDIL9341Driver* gdili9341p);
    void gdili9341PixelSet(GDIL9341Driver* gdili9341p, coord_t x, coord_t y,
            color_t color);
    void gdili9341StreamStart(GDIL9341Driver* gdili9341p, coord_t left, coord_t top,
            coord_t width, coord_t height);
    void gdili9341StreamWrite(GDIL9341Driver* gdili9341p, const color_t data[], size_t n);
    void gdili9341StreamEnd(GDIL9341Driver* gdili9341p);
    void gdili9341RectFill(GDIL9341Driver* gdili9341p, coord_t left, coord_t top,
            coord_t width, coord_t height, color_t color);
    bool_t gdili9341GetInfo(GDIL9341Driver* gdili9341p, GDDeviceInfo* gddip);
    void gdili9341AcquireBus(GDIL9341Driver* gdili9341p);
    void gdili9341ReleaseBus(GDIL9341Driver* gdili9341p);
    void gdili9341Select(GDIL9341Driver* gdili9341p);
    void gdili9341Unselect(GDIL9341Driver* gdili9341p);
    void gdili9341WriteCommand(GDIL9341Driver* gdili9341p, uint8_t cmd);
    void gdili9341WriteByte(GDIL9341Driver* gdili9341p, uint8_t value);
    uint8_t gdili9341ReadByte(GDIL9341Driver* gdili9341p);
    void gdili9341WriteChunk(GDIL9341Driver* gdili9341p,
            const uint8_t chunk[], size_t length);
    void gdili9341ReadChunk(GDIL9341Driver* gdili9341p,
            uint8_t chunk[], size_t length);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_GD_ILI9341 */

#endif /* _QGD_ILI9341_H_ */

/** @} */