/**
 * @file    qgd_hx8347.h
 * @brief   Graphics display HX8347 header.
 *
 * @addtogroup GD_HX8347
 * @{
 */

#ifndef _QGD_HX8347_H_
#define _QGD_HX8347_H_

#if HAL_USE_GD_HX8347 || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    GD_HX8347 registers
 * @{
 */
#define GD_HX8347_MODE_CTL                  0x01    /**< Display mode control.*/
#define GD_HX8347_COL_ADDR_START_2          0x02    /**< Column address start 2.*/
#define GD_HX8347_COL_ADDR_START_1          0x03    /**< Column address start 1.*/
#define GD_HX8347_COL_ADDR_END_2            0x04    /**< Column address end 2.*/
#define GD_HX8347_COL_ADDR_END_1            0x05    /**< Column address end 1.*/
#define GD_HX8347_ROW_ADDR_START_2          0x06    /**< Row address start 2.*/
#define GD_HX8347_ROW_ADDR_START_1          0x07    /**< Row address start 1.*/
#define GD_HX8347_ROW_ADDR_END_2            0x08    /**< Row address end 2.*/
#define GD_HX8347_ROW_ADDR_END_1            0x09    /**< Row address end 1.*/
#define GD_HX8347_AREA_ROW_START_2          0x0a    /**< Partial area start row 2.*/
#define GD_HX8347_AREA_ROW_START_1          0x0b    /**< Partial area start row 1.*/
#define GD_HX8347_AREA_ROW_END_2            0x0c    /**< Partial area end row 2.*/
#define GD_HX8347_AREA_ROW_END_1            0x0d    /**< Partial area end row 1.*/
#define GD_HX8347_VSCROLL_AREA_2_TOP        0x0e    /**< Vertical scroll top fixed area 2.*/
#define GD_HX8347_VSCROLL_AREA_1_TOP        0x0f    /**< Vertical scroll top fixed area 1.*/
#define GD_HX8347_VSCROLL_AREA_2_HEIGHT     0x10    /**< Vertical scroll height area 2.*/
#define GD_HX8347_VSCROLL_AREA_1_HEIGHT     0x11    /**< Vertical scroll height area 1.*/
#define GD_HX8347_VSCROLL_AREA_2_BUTTON     0x12    /**< Vertical scroll button area 2.*/
#define GD_HX8347_VSCROLL_AREA_1_BUTTON     0x13    /**< Vertical scroll button area 1.*/
#define GD_HX8347_VSCROLL_ADDR_START_2      0x14    /**< Vertical scroll start address 2.*/
#define GD_HX8347_VSCROLL_ADDR_START_1      0x15    /**< Vertical scroll start address 1.*/
#define GD_HX8347_WRITE_DATA                0x22    /**< Write data.*/
#define GD_HX8347_ID_1                      0x61    /**< ID setting 1.*/
#define GD_HX8347_ID_2                      0x62    /**< ID setting 2.*/
#define GD_HX8347_ID_3                      0x63    /**< ID setting 3.*/

/**
 * @name    GD_HX8347 interface modes
 * @{
 */
#define GD_HX8347_IM_16LPI_2                0x02     /**< 16-line parallel, mode 2.*/
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    GD_HX8347 configuration options
 * @{
 */

/**
 * @brief   Enables the @p gdhx8347AcquireBus() and @p gdhx8347ReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(GD_HX8347_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define GD_HX8347_USE_MUTUAL_EXCLUSION         TRUE
#endif

/**
 * @brief   GD_HX8347 Interface Mode
 */
#if !defined(GD_HX8347_IM) || defined(__DOXYGEN__)
#define GD_HX8347_IM                           GD_HX8347_IM_16LPI_2
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if GD_HX8347_USE_MUTUAL_EXCLUSION &&                                        \
    (!CH_USE_MUTEXES && !CH_USE_SEMAPHORES)
#error "GD_HX8347_USE_MUTUAL_EXCLUSION requires "                            \
    "CH_USE_MUTEXES and/or CH_USE_SEMAPHORES"
#endif

#if GD_HX8347_IM != GD_HX8347_IM_16LPI_2
#error "Unsupported interface mode selected"
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
     * @brief <tt>D/!C</tt> signal port
     */
    ioportid_t dcx_port;
    /**
     * @brief <tt>D/!C</tt> signal pad
     */
    uint16_t dcx_pad;
    /**
     * @brief <tt>!CS</tt> signal port
     */
    ioportid_t csx_port;
    /**
     * @brief <tt>!CS</tt> signal pad
     */
    uint16_t csx_pad;
    /**
     * @brief <tt>!RD</tt> signal port
     */
    ioportid_t rdx_port;
    /**
     * @brief <tt>!RD</tt> signal pad
     */
    uint16_t rdx_pad;
    /**
     * @brief <tt>!WR</tt> signal port
     */
    ioportid_t wrx_port;
    /**
     * @brief <tt>!WR</tt> signal pad
     */
    uint16_t wrx_pad;
    /**
     * @brief <tt>DATA</tt> bus
     */
    IOBus* db_bus;
    /**
     * @brief Configuration callback
     */
    void (*config_cb)(BaseGDDevice* gdp);
} GDHX8347Config;

/**
 * @brief   @p GDHX8347Driver specific methods.
 */
#define _gd_hx8347_driver_methods                                            \
    _base_gd_device_methods

/**
 * @extends BaseGDDeviceVMT
 *
 * @brief   @p GDHX8347Driver virtual methods table.
 */
struct GDHX8347DriverVMT
{
    _gd_hx8347_driver_methods
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
    const struct GDHX8347DriverVMT* vmt;
    _base_gd_device_data
    /**
    * @brief Current configuration data.
    */
    const GDHX8347Config* config;
#if GD_HX8347_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief Mutex protecting the device.
     */
    Mutex mutex;
#elif CH_USE_SEMAPHORES
    Semaphore semaphore;
#endif
#endif /* GD_HX8347_USE_MUTUAL_EXCLUSION */
    /**
    * @brief Cached device info
    */
    GDDeviceInfo gddi;
    /**
     * @brief Non-stacked value, for SPI with CCM.
     */
    uint8_t value;
} GDHX8347Driver;

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
    void gdhx8347Init(void);
    void gdhx8347ObjectInit(GDHX8347Driver* gdhx8347p);
    void gdhx8347Start(GDHX8347Driver* gdhx8347p,
            const GDHX8347Config* config);
    void gdhx8347Stop(GDHX8347Driver* gdhx8347p);
    void gdhx8347PixelSet(GDHX8347Driver* gdhx8347p, coord_t x, coord_t y,
            color_t color);
    void gdhx8347StreamStart(GDHX8347Driver* gdhx8347p, coord_t left, coord_t top,
            coord_t width, coord_t height);
    void gdhx8347StreamWrite(GDHX8347Driver* gdhx8347p, const color_t data[], size_t n);
    void gdhx8347StreamEnd(GDHX8347Driver* gdhx8347p);
    void gdhx8347RectFill(GDHX8347Driver* gdhx8347p, coord_t left, coord_t top,
            coord_t width, coord_t height, color_t color);
    bool_t gdhx8347GetInfo(GDHX8347Driver* gdhx8347p, GDDeviceInfo* gddip);
    void gdhx8347AcquireBus(GDHX8347Driver* gdhx8347p);
    void gdhx8347ReleaseBus(GDHX8347Driver* gdhx8347p);
    void gdhx8347Select(GDHX8347Driver* gdhx8347p);
    void gdhx8347Unselect(GDHX8347Driver* gdhx8347p);
    void gdhx8347WriteCommand(GDHX8347Driver* gdhx8347p, uint8_t cmd);
    void gdhx8347WriteByte(GDHX8347Driver* gdhx8347p, uint8_t value);
    uint8_t gdhx8347ReadByte(GDHX8347Driver* gdhx8347p);
    void gdhx8347WriteChunk(GDHX8347Driver* gdhx8347p,
            const uint8_t chunk[], size_t length);
    void gdhx8347ReadChunk(GDHX8347Driver* gdhx8347p,
            uint8_t chunk[], size_t length);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_GD_HX8347 */

#endif /* _QGD_HX8347_H_ */

/** @} */
