/**
 * @file    qgd_ili9341.c
 * @brief   Graphics display ILI9341 code.
 *
 * @addtogroup GD_ILI9341
 * @{
 */

#include "qhal.h"

#if HAL_USE_GD_ILI9341 || defined(__DOXYGEN__)

#include "nelems.h"

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

/**
 * @brief   Virtual methods table.
 */
static const struct GDILI9341DriverVMT gd_sim_vmt =
{
    .pixel_set = (void (*)(void*, coord_t, coord_t, color_t))gdili9341PixelSet,
    .stream_start = (void (*)(void*, coord_t, coord_t, coord_t, coord_t))
            gdili9341StreamStart,
    .stream_write =
            (void (*)(void*, const color_t[], size_t))gdili9341StreamWrite,
    .stream_end = (void (*)(void*))gdili9341StreamEnd,
    .rect_fill = (void (*)(void*, coord_t, coord_t, coord_t, coord_t, color_t))
            gdili9341RectFill,
    .get_info = (bool (*)(void*, GDDeviceInfo*))gdili9341GetInfo,
    .acquire = (void (*)(void*))gdili9341AcquireBus,
    .release = (void (*)(void*))gdili9341ReleaseBus,
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Graphics display ILI9341.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void gdili9341Init(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] gdili9341p   pointer to the @p GDILI9341Driver object
 *
 * @init
 */
void gdili9341ObjectInit(GDILI9341Driver* gdili9341p)
{
    gdili9341p->vmt = &gd_sim_vmt;
    gdili9341p->state = GD_STOP;
    gdili9341p->config = NULL;
#if GD_ILI9341_USE_MUTUAL_EXCLUSION
#if CH_CFG_USE_MUTEXES
    chMtxInit(&gdili9341p->mutex);
#else
    chSemInit(&gdili9341p->semaphore, 1);
#endif
#endif /* GD_ILI9341_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the graphics display ILI9341.
 *
 * @param[in] gdili9341p    pointer to the @p GDILI9341Driver object
 * @param[in] config        pointer to the @p GDILI9341Config object.
 *
 * @api
 */
void gdili9341Start(GDILI9341Driver* gdili9341p, const GDILI9341Config* config)
{
    chDbgCheck((gdili9341p != NULL) && (config != NULL));
    /* Verify device status. */
    chDbgAssert((gdili9341p->state == GD_STOP) || (gdili9341p->state == GD_READY),
            "invalid state");

    if (gdili9341p->state == GD_READY)
        gdili9341Stop(gdili9341p);

    gdili9341p->config = config;
    gdili9341p->state = GD_READY;

    /* Cache device info. */
    gdili9341p->gddi.size_x = gdili9341p->config->size_x;
    gdili9341p->gddi.size_y = gdili9341p->config->size_y;

    /* Lock device. */
    gdili9341AcquireBus(gdili9341p);

    /* Exit deep standby mode sequence according to datasheet. */
    for (uint8_t i = 0; i < 6; ++i)
    {
        gdili9341Select(gdili9341p);
        gdili9341Unselect(gdili9341p);
        chThdSleepMilliseconds(1);
    }

    /* Wait for reset delay according to datasheet. */
    chThdSleepMilliseconds(5);

    /* Soft reset */
    gdili9341Select(gdili9341p);
    gdili9341WriteCommand(gdili9341p, GD_ILI9341_CMD_RESET);
    gdili9341Unselect(gdili9341p);

    /* Wait for reset delay according to datasheet. */
    chThdSleepMilliseconds(5);

    {
        /* Read device id. */
        uint8_t temp[4];
        gdili9341Select(gdili9341p);
        gdili9341WriteCommand(gdili9341p, GD_ILI9341_GET_ID_INFO);
        for (size_t i = 0; i < sizeof(temp); ++i)
            temp[i] = gdili9341ReadByte(gdili9341p);
        gdili9341Unselect(gdili9341p);
        memcpy(gdili9341p->gddi.id, temp + 1, sizeof(temp) - 1);
    }

    gdili9341Select(gdili9341p);

    /* Execute user supplied configuration. */
    if (config->config_cb != NULL)
        config->config_cb((BaseGDDevice*)gdili9341p);

    /* Execute mandatory configuration. */
    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_PIX_FMT);
    gdili9341WriteByte(gdili9341p, 0x05); /* 16 bits / pixel, mcu interface */

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_IF_CTL);
    gdili9341WriteByte(gdili9341p, 0x01);
    gdili9341WriteByte(gdili9341p, 0x00);
    gdili9341WriteByte(gdili9341p, 0x00);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_CMD_SLEEP_OFF);

    gdili9341Unselect(gdili9341p);

    /* We get memory corruption if we wait less than 21 ms here. */
    chThdSleepMilliseconds(30);

    /* Clear memory. */
    gdili9341RectFill(gdili9341p, 0, 0, gdili9341p->config->size_x,
            gdili9341p->config->size_y, 0);

    gdili9341Select(gdili9341p);
    gdili9341WriteCommand(gdili9341p, GD_ILI9341_CMD_DISPLAY_ON);
    gdili9341Unselect(gdili9341p);

    /* Unlock device. */
    gdili9341ReleaseBus(gdili9341p);
}

/**
 * @brief   Disables the graphics display peripheral.
 *
 * @param[in] gdili9341p    pointer to the @p GDILI9341Driver object
 *
 * @api
 */
void gdili9341Stop(GDILI9341Driver* gdili9341p)
{
    chDbgCheck(gdili9341p != NULL);
    /* Verify device status. */
    chDbgAssert((gdili9341p->state == GD_STOP) || (gdili9341p->state == GD_READY),
            "invalid state");

    gdili9341AcquireBus(gdili9341p);
    gdili9341Select(gdili9341p);
    gdili9341WriteCommand(gdili9341p, GD_ILI9341_CMD_DISPLAY_OFF);

    /* Enter deep standby mode. */
    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_ENTRY_MODE);
    gdili9341WriteByte(gdili9341p, 0x08);

    gdili9341Unselect(gdili9341p);
    gdili9341ReleaseBus(gdili9341p);

    gdili9341p->state = GD_STOP;
}

/**
 * @brief   Sets pixel color.
 *
 * @param[in] gdili9341p    pointer to the @p GDILI9341Driver object
 * @param[in] gddip         pointer to a @p GDDeviceInfo structure
 *
 * @api
 */
void gdili9341PixelSet(GDILI9341Driver* gdili9341p, coord_t x, coord_t y, color_t color)
{
    chDbgCheck(gdili9341p != NULL);
    /* Verify device status. */
    chDbgAssert(gdili9341p->state >= GD_READY, "invalid state");

    gdili9341Select(gdili9341p);
    gdili9341StreamStart(gdili9341p, x, y, 1, 1);
    gdili9341StreamWrite(gdili9341p, &color, 1);
    gdili9341StreamEnd(gdili9341p);
    gdili9341Unselect(gdili9341p);
}

/**
 * @brief   Starts streamed writing into a window.
 *
 * @param[in] gdili9341p    pointer to the @p GDILI9341Driver object
 * @param[in] left          left window border coordinate
 * @param[in] top           top window border coordinate
 * @param[in] width         height of the window
 * @param[in] height        width of the window
 *
 * @api
 */
void gdili9341StreamStart(GDILI9341Driver* gdili9341p, coord_t left, coord_t top,
        coord_t width, coord_t height)
{
    chDbgCheck(gdili9341p != NULL);
    /* Verify device status. */
    chDbgAssert(gdili9341p->state >= GD_READY, "invalid state");

    gdili9341Select(gdili9341p);

    {
        const uint8_t data[] =
        {
            left >> 8,
            left >> 0,
            (left + width - 1) >> 8,
            (left + width - 1) >> 0,
        };
        gdili9341p->config->write_cmd_cb(GD_ILI9341_SET_COL_ADDR);
        gdili9341p->config->write_parm_cb(data, NELEMS(data));
    }

    {
        const uint8_t data[] =
        {
            top >> 8,
            top >> 0,
            (top + height - 1) >> 8,
            (top + height - 1) >> 0,
        };
        gdili9341p->config->write_cmd_cb(GD_ILI9341_SET_PAGE_ADDR);
        gdili9341p->config->write_parm_cb(data, NELEMS(data));
    }

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_MEM);
}

/**
 * @brief   Write a chunk of data in stream mode.
 *
 * @param[in] gdili9341p    pointer to the @p GDILI9341Driver object
 * @param[in] data[]        array of color_t data
 * @param[in] n             number of array elements
 *
 * @api
 */
void gdili9341StreamWrite(GDILI9341Driver* gdili9341p, const color_t data[],
        size_t n)
{
    chDbgCheck(gdili9341p != NULL);
    /* Verify device status. */
    chDbgAssert(gdili9341p->state >= GD_ACTIVE, "invalid state");

    gdili9341p->config->write_mem_cb(data, n);
}

/**
 * @brief   End stream mode writing.
 *
 * @param[in] gdili9341p    pointer to the @p GDILI9341Driver object
 *
 * @api
 */
void gdili9341StreamEnd(GDILI9341Driver* gdili9341p)
{
    chDbgCheck(gdili9341p != NULL);
    /* Verify device status. */
    chDbgAssert(gdili9341p->state >= GD_ACTIVE, "invalid state");

    gdili9341Unselect(gdili9341p);
}

/**
 * @brief   Fills a rectangle with a color.
 *
 * @param[in] gdili9341p    pointer to the @p GDILI9341Driver object
 * @param[in] left          left rectangle border coordinate
 * @param[in] top           top rectangle border coordinate
 * @param[in] width         height of the rectangle
 * @param[in] height        width of the rectangle
 *
 * @api
 */
void gdili9341RectFill(GDILI9341Driver* gdili9341p, coord_t left, coord_t top,
        coord_t width, coord_t height, color_t color)
{
    chDbgCheck(gdili9341p != NULL);
    /* Verify device status. */
    chDbgAssert(gdili9341p->state >= GD_READY, "invalid state");

    gdili9341StreamStart(gdili9341p, left, top, width, height);

    const color_t temp[] =
    {
        color, color, color, color, color, color, color, color,
        color, color, color, color, color, color, color, color,
    };

    const size_t n = (size_t)width * height;

    for (size_t i = 0; i < n; i += NELEMS(temp))
        gdili9341StreamWrite(gdili9341p, temp, NELEMS(temp));

    gdili9341StreamEnd(gdili9341p);
}

/**
 * @brief   Returns device info.
 *
 * @param[in] gdili9341p    pointer to the @p GDILI9341Driver object
 * @param[out] gddip        pointer to a @p GDDeviceInfo structure
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool gdili9341GetInfo(GDILI9341Driver* gdili9341p, GDDeviceInfo* gddip)
{
    chDbgCheck(gdili9341p != NULL);
    /* Verify device status. */
    chDbgAssert(gdili9341p->state >= GD_READY, "invalid state");

    memcpy(gddip, &gdili9341p->gddi, sizeof(*gddip));

    return HAL_SUCCESS;
}

/**
 * @brief   Gains exclusive access to the graphics display device.
 * @details This function tries to gain ownership to the graphics display
 *          device, if the device is already being used then the invoking
 *          thread is queued.
 * @pre     In order to use this function the option
 *          @p GD_ILI9341_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] gdili9341p     pointer to the @p GDILI9341Driver object
 *
 * @api
 */
void gdili9341AcquireBus(GDILI9341Driver* gdili9341p)
{
    chDbgCheck(gdili9341p != NULL);

#if GD_ILI9341_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES
    chMtxLock(&gdili9341p->mutex);
#elif CH_CFG_USE_SEMAPHORES
    chSemWait(&gdili9341p->semaphore);
#endif
#endif /* GD_ILI9341_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the graphics display device.
 * @pre     In order to use this function the option
 *          @p GD_ILI9341_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] gdili9341p     pointer to the @p GDILI9341Driver object
 *
 * @api
 */
void gdili9341ReleaseBus(GDILI9341Driver* gdili9341p)
{
    chDbgCheck(gdili9341p != NULL);

#if GD_ILI9341_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES
    (void)gdili9341p;
    chMtxUnlock();
#elif CH_CFG_USE_SEMAPHORES
    chSemSignal(&gdili9341p->semaphore);
#endif
#endif /* GD_ILI9341_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Asserts the chip select signal and prepares for transfers.
 * @pre     ILI9341 is ready.
 *
 * @param[in] gdili9341p    pointer to the @p GDILI9341Driver object
 *
 * @api
 */
void gdili9341Select(GDILI9341Driver* gdili9341p)
{
    chDbgCheck(gdili9341p != NULL);
    chDbgAssert(gdili9341p->state == GD_READY, "invalid state");

    gdili9341p->state = GD_ACTIVE;

    gdili9341p->config->select_cb();
}

/**
 * @brief   Deasserts the chip select signal.
 * @details The previously selected peripheral is unselected.
 * @pre     ILI9341 is active.
 *
 * @param[in] gdili9341p    pointer to the @p GDILI9341Driver object
 *
 * @iclass
 */
void gdili9341Unselect(GDILI9341Driver* gdili9341p)
{
    chDbgCheck(gdili9341p != NULL);
    chDbgAssert(gdili9341p->state == GD_ACTIVE, "invalid state");

    gdili9341p->config->unselect_cb();

    gdili9341p->state = GD_READY;
}

/**
 * @brief   Write command byte.
 * @details Sends a command byte via SPI or parallel bus.
 *
 * @param[in] gdili9341p    pointer to the @p GDILI9341Driver object
 * @param[in] cmd           command byte
 *
 * @api
 */
void gdili9341WriteCommand(GDILI9341Driver* gdili9341p, uint8_t cmd)
{
    chDbgCheck(gdili9341p != NULL);
    chDbgAssert(gdili9341p->state == GD_ACTIVE, "invalid state");

    gdili9341p->config->write_cmd_cb(cmd);
}

/**
 * @brief   Write data byte.
 * @details Sends a data byte via SPI or parallel bus.
 *
 * @param[in] gdili9341p    pointer to the @p GDILI9341Driver object
 * @param[in] value         data byte
 *
 * @api
 */
void gdili9341WriteByte(GDILI9341Driver* gdili9341p, uint8_t value)
{
    chDbgCheck(gdili9341p != NULL);
    chDbgAssert(gdili9341p->state == GD_ACTIVE, "invalid state");

    gdili9341p->config->write_parm_cb(&value, 1);
}

/**
 * @brief   Read data byte.
 * @details Receives a data byte via SPI or parallel bus.
 *
 * @param[in] gdili9341p    pointer to the @p GDILI9341Driver object
 *
 * @return                  data byte
 *
 * @api
 */
uint8_t gdili9341ReadByte(GDILI9341Driver* gdili9341p)
{
    chDbgCheck(gdili9341p != NULL);
    chDbgAssert(gdili9341p->state == GD_ACTIVE, "invalid state");

    uint8_t result;
    gdili9341p->config->read_parm_cb(&result, 1);

    return result;
}

#endif /* HAL_USE_GD_ILI9341 */

/** @} */
