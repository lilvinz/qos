/**
 * @file    qgd_ili9341.c
 * @brief   Graphics display IL9341 code.
 *
 * @addtogroup GD_ILI9341
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_GD_ILI9341 || defined(__DOXYGEN__)

#include <string.h>

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
static const struct GDIL9341DriverVMT gd_sim_vmt =
{
    .pixel_set = (void (*)(void*, coord_t, coord_t, color_t))gdili9341PixelSet,
    .stream_start = (void (*)(void*, coord_t, coord_t, coord_t, coord_t))
            gdili9341StreamStart,
    .stream_write =
            (void (*)(void*, const color_t[], size_t))gdili9341StreamWrite,
    .stream_end = (void (*)(void*))gdili9341StreamEnd,
    .rect_fill = (void (*)(void*, coord_t, coord_t, coord_t, coord_t, color_t))
            gdili9341RectFill,
    .get_info = (bool_t (*)(void*, GDDeviceInfo*))gdili9341GetInfo,
    .acquire = (void (*)(void*))gdili9341AcquireBus,
    .release = (void (*)(void*))gdili9341ReleaseBus,
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void column_address_set(GDIL9341Driver* gdili9341p, uint16_t x_start, uint16_t x_end)
{
    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_COL_ADDR);
    gdili9341WriteByte(gdili9341p, x_start >> 8);
    gdili9341WriteByte(gdili9341p, (uint8_t)x_start);
    gdili9341WriteByte(gdili9341p, x_end >> 8);
    gdili9341WriteByte(gdili9341p, (uint8_t)x_end);
}

static void page_address_set(GDIL9341Driver* gdili9341p, uint16_t y_start, uint16_t y_end)
{
    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_PAGE_ADDR);
    gdili9341WriteByte(gdili9341p, y_start >> 8);
    gdili9341WriteByte(gdili9341p, (uint8_t)y_start);
    gdili9341WriteByte(gdili9341p, y_end >> 8);
    gdili9341WriteByte(gdili9341p, (uint8_t)y_end);
}

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
void gdili9341Init(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] gdili9341p    pointer to the @p GDIL9341Driver object
 *
 * @init
 */
void gdili9341ObjectInit(GDIL9341Driver* gdili9341p)
{
    gdili9341p->vmt = &gd_sim_vmt;
    gdili9341p->state = GD_STOP;
    gdili9341p->config = NULL;
#if GD_ILI9341_USE_MUTUAL_EXCLUSION
#if CH_USE_MUTEXES
    chMtxInit(&gdili9341p->mutex);
#else
    chSemInit(&gdili9341p->semaphore, 1);
#endif
#endif /* GD_ILI9341_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the graphics display simulation.
 *
 * @param[in] gdili9341p     pointer to the @p GDIL9341Driver object
 * @param[in] config        pointer to the @p GDIL9341Config object.
 *
 * @api
 */
void gdili9341Start(GDIL9341Driver* gdili9341p, const GDIL9341Config* config)
{
    chDbgCheck((gdili9341p != NULL) && (config != NULL), "gdili9341Start");
    /* Verify device status. */
    chDbgAssert((gdili9341p->state == GD_STOP) || (gdili9341p->state == GD_READY),
            "gdili9341Start(), #1", "invalid state");

    if (gdili9341p->state == GD_READY)
        gdili9341Stop(gdili9341p);

    gdili9341p->config = config;
    gdili9341p->state = GD_READY;

    /* Cache device info. */
    gdili9341p->gddi.size_x = gdili9341p->config->size_x;
    gdili9341p->gddi.size_y = gdili9341p->config->size_y;

    uint8_t temp[4];

    gdili9341AcquireBus(gdili9341p);
    gdili9341Select(gdili9341p);
    gdili9341WriteCommand(gdili9341p, GD_ILI9341_GET_ID_INFO);
    gdili9341ReadChunk(gdili9341p, temp, sizeof(temp));
    gdili9341Unselect(gdili9341p);
    gdili9341ReleaseBus(gdili9341p);

    memset(gdili9341p->gddi.id, 0, sizeof(gdili9341p->gddi.id));
    memcpy(gdili9341p->gddi.id, temp + 1, 3);

    /* Init sequence */
    static const uint8_t pgamma[15] =
    {
        0x0F, 0x29, 0x24, 0x0C, 0x0E, 0x09, 0x4E, 0x78,
        0x3C, 0x09, 0x13, 0x05, 0x17, 0x11, 0x00
    };
    static const uint8_t ngamma[15] =
    {
        0x00, 0x16, 0x1B, 0x04, 0x11, 0x07, 0x31, 0x33,
        0x42, 0x05, 0x0C, 0x0A, 0x28, 0x2F, 0x0F
    };

    gdili9341AcquireBus(gdili9341p);

    gdili9341Select(gdili9341p);
    gdili9341WriteCommand(gdili9341p, GD_ILI9341_CMD_RESET);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_MEM_ACS_CTL);
    gdili9341WriteByte(gdili9341p, 0xbc); /* MY MV ML BGR MH */

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_PIX_FMT);
    gdili9341WriteByte(gdili9341p, 0x05); /* 16 bits / pixel, mcu interface */

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_FRAME_CTL_NORMAL);
    gdili9341WriteByte(gdili9341p, 0x00);
    gdili9341WriteByte(gdili9341p, 0x1B);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_FUNCTION_CTL);
    gdili9341WriteByte(gdili9341p, 0x0A);
    gdili9341WriteByte(gdili9341p, 0xA2);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_POWER_CTL_1);
    gdili9341WriteByte(gdili9341p, 0x10);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_POWER_CTL_2);
    gdili9341WriteByte(gdili9341p, 0x10);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_VCOM_CTL_1);
    gdili9341WriteByte(gdili9341p, 0x45);
    gdili9341WriteByte(gdili9341p, 0x15);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_VCOM_CTL_2);
    gdili9341WriteByte(gdili9341p, 0x90);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_FUNCTION_CTL);
    gdili9341WriteByte(gdili9341p, 0x0A);
    gdili9341WriteByte(gdili9341p, 0xA7);
    gdili9341WriteByte(gdili9341p, 0x27);
    gdili9341WriteByte(gdili9341p, 0x04);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_COL_ADDR);
    gdili9341WriteByte(gdili9341p, 0x00);
    gdili9341WriteByte(gdili9341p, 0x00);
    gdili9341WriteByte(gdili9341p, 0x00);
    gdili9341WriteByte(gdili9341p, 0xEF);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_PAGE_ADDR);
    gdili9341WriteByte(gdili9341p, 0x00);
    gdili9341WriteByte(gdili9341p, 0x00);
    gdili9341WriteByte(gdili9341p, 0x01);
    gdili9341WriteByte(gdili9341p, 0x3F);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_IF_CTL);
    gdili9341WriteByte(gdili9341p, 0x01); /* WEMODE */
    gdili9341WriteByte(gdili9341p, 0x00);
    gdili9341WriteByte(gdili9341p, 0x00);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_GAMMA);
    gdili9341WriteByte(gdili9341p, 0x01);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_PGAMMA);
    gdili9341WriteChunk(gdili9341p, pgamma, 15);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_NGAMMA);
    gdili9341WriteChunk(gdili9341p, ngamma, 15);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_CMD_SLEEP_OFF);
    chThdSleepMilliseconds(10);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_CMD_DISPLAY_ON);
    chThdSleepMilliseconds(10);

    gdili9341Unselect(gdili9341p);
    gdili9341ReleaseBus(gdili9341p);
}

/**
 * @brief   Disables the graphics display peripheral.
 *
 * @param[in] gdili9341p     pointer to the @p GDIL9341Driver object
 *
 * @api
 */
void gdili9341Stop(GDIL9341Driver* gdili9341p)
{
    chDbgCheck(gdili9341p != NULL, "gdili9341Stop");
    /* Verify device status. */
    chDbgAssert((gdili9341p->state == GD_STOP) || (gdili9341p->state == GD_READY),
            "gdili9341Stop(), #1", "invalid state");

    gdili9341AcquireBus(gdili9341p);
    gdili9341Select(gdili9341p);
    gdili9341WriteCommand(gdili9341p, GD_ILI9341_CMD_DISPLAY_OFF);
    gdili9341WriteCommand(gdili9341p, GD_ILI9341_CMD_SLEEP_ON);
    gdili9341Unselect(gdili9341p);
    gdili9341ReleaseBus(gdili9341p);

    gdili9341p->state = GD_STOP;
}

/**
 * @brief   Sets pixel color.
 *
 * @param[in] gdili9341p     pointer to the @p GDIL9341Driver object
 * @param[in] gddip         pointer to a @p GDDeviceInfo structure
 *
 * @api
 */
void gdili9341PixelSet(GDIL9341Driver* gdili9341p, coord_t x, coord_t y, color_t color)
{
    chDbgCheck(gdili9341p != NULL, "gdPixelSet");
    /* Verify device status. */
    chDbgAssert(gdili9341p->state >= GD_READY, "gdPixelSet(), #1",
            "invalid state");

    gdili9341Select(gdili9341p);

    column_address_set(gdili9341p, x, x);
    page_address_set(gdili9341p, y, y);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_MEM);

    uint8_t temp[2];

    temp[0] =
            (((color & 0xff0000) >> 16 >> 3) << 3) |
            (((color & 0x00ff00) >> 8 >> 2) >> 3);

    temp[1] =
            (((color & 0x00ff00) >> 8 >> 2) << 5) |
            (((color & 0x0000ff) >> 0 >> 3) << 0);

    gdili9341WriteChunk(gdili9341p, temp, sizeof(temp));

    gdili9341Unselect(gdili9341p);
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
void gdili9341StreamStart(GDIL9341Driver* gdili9341p, coord_t left, coord_t top,
        coord_t width, coord_t height)
{
    chDbgCheck(gdili9341p != NULL, "gdili9341StreamStart");
    /* Verify device status. */
    chDbgAssert(gdili9341p->state >= GD_READY, "gdili9341StreamStart(), #1",
            "invalid state");

    gdili9341Select(gdili9341p);

    column_address_set(gdili9341p, left, left + width - 1);
    page_address_set(gdili9341p, top, top + height - 1);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_MEM);
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
void gdili9341StreamWrite(GDIL9341Driver* gdili9341p, const color_t data[], size_t n)
{
    chDbgCheck(gdili9341p != NULL, "gdili9341StreamWrite");
    /* Verify device status. */
    chDbgAssert(gdili9341p->state >= GD_ACTIVE, "gdili9341StreamWrite(), #1",
            "invalid state");

    for (size_t i = 0; i < n; ++i)
    {
        uint8_t temp[2];

        temp[0] =
                (((data[i] & 0xff0000) >> 16 >> 3) << 3) |
                (((data[i] & 0x00ff00) >> 8 >> 2) >> 3);

        temp[1] =
                (((data[i] & 0x00ff00) >> 8 >> 2) << 5) |
                (((data[i] & 0x0000ff) >> 0 >> 3) << 0);

        gdili9341WriteChunk(gdili9341p, temp, sizeof(temp));
    }
}

/**
 * @brief   End stream mode writing.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 *
 * @api
 */
void gdili9341StreamEnd(GDIL9341Driver* gdili9341p)
{
    chDbgCheck(gdili9341p != NULL, "gdili9341StreamEnd");
    /* Verify device status. */
    chDbgAssert(gdili9341p->state >= GD_ACTIVE, "gdili9341StreamEnd(), #1",
            "invalid state");

    gdili9341Unselect(gdili9341p);
}

/**
 * @brief   Fills a rectangle with a color.
 *
 * @param[in] ip        pointer to a @p BaseGDDevice or derived class
 * @param[in] left      left rectangle border coordinate
 * @param[in] top       top rectangle border coordinate
 * @param[in] width     height of the rectangle
 * @param[in] height    width of the rectangle
 *
 * @api
 */
void gdili9341RectFill(GDIL9341Driver* gdili9341p, coord_t left, coord_t top,
        coord_t width, coord_t height, color_t color)
{
    chDbgCheck(gdili9341p != NULL, "gdili9341RectFill");
    /* Verify device status. */
    chDbgAssert(gdili9341p->state >= GD_READY, "gdili9341RectFill(), #1",
            "invalid state");

    gdili9341Select(gdili9341p);

    column_address_set(gdili9341p, left, left + width - 1);
    page_address_set(gdili9341p, top, top + height - 1);

    gdili9341WriteCommand(gdili9341p, GD_ILI9341_SET_MEM);

    uint8_t temp[2];

    temp[0] =
            (((color & 0xff0000) >> 16 >> 3) << 3) |
            (((color & 0x00ff00) >> 8 >> 2) >> 3);

    temp[1] =
            (((color & 0x00ff00) >> 8 >> 2) << 5) |
            (((color & 0x0000ff) >> 0 >> 3) << 0);

    for (size_t i = 0; i < (size_t)width * height; ++i)
        gdili9341WriteChunk(gdili9341p, temp, sizeof(temp));

    gdili9341Unselect(gdili9341p);
}

/**
 * @brief   Returns device info.
 *
 * @param[in] gdili9341p     pointer to the @p GDIL9341Driver object
 * @param[out] gddip        pointer to a @p GDDeviceInfo structure
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t gdili9341GetInfo(GDIL9341Driver* gdili9341p, GDDeviceInfo* gddip)
{
    chDbgCheck(gdili9341p != NULL, "gdili9341GetInfo");
    /* Verify device status. */
    chDbgAssert(gdili9341p->state >= GD_READY, "gdili9341GetInfo(), #1",
            "invalid state");

    memcpy(gddip, &gdili9341p->gddi, sizeof(*gddip));

    return CH_SUCCESS;
}

/**
 * @brief   Gains exclusive access to the graphics display device.
 * @details This function tries to gain ownership to the graphics display
 *          device, if the device is already being used then the invoking
 *          thread is queued.
 * @pre     In order to use this function the option
 *          @p GD_ILI9341_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] gdili9341p     pointer to the @p GDIL9341Driver object
 *
 * @api
 */
void gdili9341AcquireBus(GDIL9341Driver* gdili9341p)
{
    chDbgCheck(gdili9341p != NULL, "gdili9341AcquireBus");

#if GD_ILI9341_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    chMtxLock(&gdili9341p->mutex);
#elif CH_USE_SEMAPHORES
    chSemWait(&gdili9341p->semaphore);
#endif
#endif /* GD_ILI9341_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the graphics display device.
 * @pre     In order to use this function the option
 *          @p GD_ILI9341_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] gdili9341p     pointer to the @p GDIL9341Driver object
 *
 * @api
 */
void gdili9341ReleaseBus(GDIL9341Driver* gdili9341p)
{
    chDbgCheck(gdili9341p != NULL, "gdili9341ReleaseBus");

#if GD_ILI9341_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    (void)gdili9341p;
    chMtxUnlock();
#elif CH_USE_SEMAPHORES
    chSemSignal(&gdili9341p->semaphore);
#endif
#endif /* GD_ILI9341_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Asserts the slave select signal and prepares for transfers.
 * @pre     ILI9341 is ready.
 *
 * @param[in] gdili9341p   pointer to the @p GDIL9341Driver object
 *
 * @api
 */
void gdili9341Select(GDIL9341Driver* gdili9341p)
{
    chDbgCheck(gdili9341p != NULL, "gdili9341SelectI");
    chDbgAssert(gdili9341p->state == GD_READY,
            "gdili9341SelectI(), #1", "invalid state");

    gdili9341p->state = GD_ACTIVE;
    spiSelect(gdili9341p->config->spip);
}

/**
 * @brief   Deasserts the slave select signal.
 * @details The previously selected peripheral is unselected.
 * @pre     ILI9341 is active.
 *
 * @param[in] gdili9341p   pointer to the @p GDIL9341Driver object
 *
 * @iclass
 */
void gdili9341Unselect(GDIL9341Driver* gdili9341p)
{
    chDbgCheck(gdili9341p != NULL, "gdili9341Unselect");
    chDbgAssert(gdili9341p->state == GD_ACTIVE,
            "gdili9341UnselectI(), #1", "invalid state");

    spiUnselect(gdili9341p->config->spip);
    gdili9341p->state = GD_READY;
}

/**
 * @brief   Write command byte.
 * @details Sends a command byte via SPI.
 *
 * @param[in] gdili9341p   pointer to the @p GDIL9341Driver object
 * @param[in] cmd       command byte
 *
 * @api
 */
void gdili9341WriteCommand(GDIL9341Driver* gdili9341p, uint8_t cmd)
{
    chDbgCheck(gdili9341p != NULL, "gdili9341WriteCommand");
    chDbgAssert(gdili9341p->state == GD_ACTIVE,
            "gdili9341WriteCommand(), #1", "invalid state");

    gdili9341p->value = cmd;
    /* Cmd */
    palClearPad(gdili9341p->config->dcx_port, gdili9341p->config->dcx_pad);
    spiSend(gdili9341p->config->spip, 1, &gdili9341p->value);
}

/**
 * @brief   Write data byte.
 * @details Sends a data byte via SPI.
 *
 * @param[in] gdili9341p   pointer to the @p GDIL9341Driver object
 * @param[in] value     data byte
 *
 * @api
 */
void gdili9341WriteByte(GDIL9341Driver* gdili9341p, uint8_t value)
{
    chDbgCheck(gdili9341p != NULL, "gdili9341WriteByte");
    chDbgAssert(gdili9341p->state == GD_ACTIVE,
            "gdili9341WriteByte(), #1", "invalid state");

    gdili9341p->value = value;
    /* Data */
    palSetPad(gdili9341p->config->dcx_port, gdili9341p->config->dcx_pad);
    spiSend(gdili9341p->config->spip, 1, &gdili9341p->value);
}

/**
 * @brief   Read data byte.
 * @details Receives a data byte via SPI.
 *
 * @param[in] gdili9341p   pointer to the @p GDIL9341Driver object
 *
 * @return              data byte
 *
 * @api
 */
uint8_t gdili9341ReadByte(GDIL9341Driver* gdili9341p)
{
    chDbgAssert(FALSE, "gdili9341ReadByte()", "should not be used");

    chDbgCheck(gdili9341p != NULL, "gdili9341ReadByte");
    chDbgAssert(gdili9341p->state == GD_ACTIVE,
            "gdili9341ReadByte(), #1", "invalid state");

    /* Data */
    palSetPad(gdili9341p->config->dcx_port, gdili9341p->config->dcx_pad);
    spiReceive(gdili9341p->config->spip, 1, &gdili9341p->value);
    return gdili9341p->value;
}

/**
 * @brief   Write data chunk.
 * @details Sends a data chunk via SPI.
 * @pre     The chunk must be accessed by DMA.
 *
 * @param[in] gdili9341p   pointer to the @p GDIL9341Driver object
 * @param[in] chunk     chunk bytes
 * @param[in] length    chunk length
 *
 * @api
 */
void gdili9341WriteChunk(GDIL9341Driver* gdili9341p, const uint8_t chunk[],
        size_t length)
{
    chDbgCheck(gdili9341p != NULL, "gdili9341WriteChunk");
    chDbgCheck(chunk != NULL, "gdili9341WriteChunk");
    chDbgAssert(gdili9341p->state == GD_ACTIVE,
              "gdili9341WriteChunk(), #1", "invalid state");

    /* Data */
    palSetPad(gdili9341p->config->dcx_port, gdili9341p->config->dcx_pad);
    spiSend(gdili9341p->config->spip, length, chunk);
}

/**
 * @brief   Read data chunk.
 * @details Receives a data chunk via SPI.
 * @pre     The chunk must be accessed by DMA.
 *
 * @param[in] gdili9341p   pointer to the @p GDIL9341Driver object
 * @param[out] chunk    chunk bytes
 * @param[in] length    chunk length
 *
 * @api
 */
void gdili9341ReadChunk(GDIL9341Driver* gdili9341p, uint8_t chunk[],
        size_t length)
{
    chDbgCheck(gdili9341p != NULL, "gdili9341ReadChunk");
    chDbgCheck(chunk != NULL, "gdili9341ReadChunk");
    chDbgAssert(gdili9341p->state == GD_ACTIVE,
            "gdili9341ReadChunk(), #1", "invalid state");

    /* Data */
    palSetPad(gdili9341p->config->dcx_port, gdili9341p->config->dcx_pad);
    spiReceive(gdili9341p->config->spip, length, chunk);
}

#endif /* HAL_USE_GD_ILI9341 */

/** @} */
