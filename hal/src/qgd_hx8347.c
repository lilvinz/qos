/**
 * @file    qgd_hx8347.c
 * @brief   Graphics display HX8347 code.
 *
 * @addtogroup GD_HX8347
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_GD_HX8347 || defined(__DOXYGEN__)

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
static const struct GDHX8347DriverVMT gd_sim_vmt =
{
    .pixel_set = (void (*)(void*, coord_t, coord_t, color_t))gdhx8347PixelSet,
    .stream_start = (void (*)(void*, coord_t, coord_t, coord_t, coord_t))
            gdhx8347StreamStart,
    .stream_write =
            (void (*)(void*, const color_t[], size_t))gdhx8347StreamWrite,
    .stream_end = (void (*)(void*))gdhx8347StreamEnd,
    .rect_fill = (void (*)(void*, coord_t, coord_t, coord_t, coord_t, color_t))
            gdhx8347RectFill,
    .get_info = (bool_t (*)(void*, GDDeviceInfo*))gdhx8347GetInfo,
    .acquire = (void (*)(void*))gdhx8347AcquireBus,
    .release = (void (*)(void*))gdhx8347ReleaseBus,
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void column_address_set(GDHX8347Driver* gdhx8347p, uint16_t x_start, uint16_t x_end)
{
    gdhx8347WriteCommand(gdhx8347p, GD_HX8347_COL_ADDR_START_2);
    gdhx8347WriteByte(gdhx8347p, x_start >> 8);
    gdhx8347WriteCommand(gdhx8347p, GD_HX8347_COL_ADDR_START_1);
    gdhx8347WriteByte(gdhx8347p, (uint8_t)x_start);
    gdhx8347WriteCommand(gdhx8347p, GD_HX8347_COL_ADDR_END_2);
    gdhx8347WriteByte(gdhx8347p, x_end >> 8);
    gdhx8347WriteCommand(gdhx8347p, GD_HX8347_COL_ADDR_END_1);
    gdhx8347WriteByte(gdhx8347p, (uint8_t)x_end);
}

static void page_address_set(GDHX8347Driver* gdhx8347p, uint16_t y_start, uint16_t y_end)
{
    gdhx8347WriteCommand(gdhx8347p, GD_HX8347_ROW_ADDR_START_2);
    gdhx8347WriteByte(gdhx8347p, y_start >> 8);
    gdhx8347WriteCommand(gdhx8347p, GD_HX8347_ROW_ADDR_START_1);
    gdhx8347WriteByte(gdhx8347p, (uint8_t)y_start);
    gdhx8347WriteCommand(gdhx8347p, GD_HX8347_ROW_ADDR_END_2);
    gdhx8347WriteByte(gdhx8347p, y_end >> 8);
    gdhx8347WriteCommand(gdhx8347p, GD_HX8347_ROW_ADDR_END_1);
    gdhx8347WriteByte(gdhx8347p, (uint8_t)y_end);
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Graphics display HX8347.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void gdhx8347Init(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] gdhx8347p    pointer to the @p GDHX8347Driver object
 *
 * @init
 */
void gdhx8347ObjectInit(GDHX8347Driver* gdhx8347p)
{
    gdhx8347p->vmt = &gd_sim_vmt;
    gdhx8347p->state = GD_STOP;
    gdhx8347p->config = NULL;
#if GD_HX8347_USE_MUTUAL_EXCLUSION
#if CH_USE_MUTEXES
    chMtxInit(&gdhx8347p->mutex);
#else
    chSemInit(&gdhx8347p->semaphore, 1);
#endif
#endif /* GD_HX8347_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the graphics display HX8347.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 * @param[in] config        pointer to the @p GDHX8347Config object.
 *
 * @api
 */
void gdhx8347Start(GDHX8347Driver* gdhx8347p, const GDHX8347Config* config)
{
    chDbgCheck((gdhx8347p != NULL) && (config != NULL), "gdhx8347Start");
    /* Verify device status. */
    chDbgAssert((gdhx8347p->state == GD_STOP) || (gdhx8347p->state == GD_READY),
            "gdhx8347Start(), #1", "invalid state");

    if (gdhx8347p->state == GD_READY)
        gdhx8347Stop(gdhx8347p);

    gdhx8347p->config = config;
    gdhx8347p->state = GD_READY;

    /* Cache device info. */
    gdhx8347p->gddi.size_x = gdhx8347p->config->size_x;
    gdhx8347p->gddi.size_y = gdhx8347p->config->size_y;

    /* Lock device. */
    gdhx8347AcquireBus(gdhx8347p);

    /* Read device id. */
    gdhx8347Select(gdhx8347p);
    gdhx8347WriteCommand(gdhx8347p, GD_HX8347_ID_1);
    gdhx8347p->gddi.id[0] = gdhx8347ReadByte(gdhx8347p);
    gdhx8347WriteCommand(gdhx8347p, GD_HX8347_ID_2);
    gdhx8347p->gddi.id[1] = gdhx8347ReadByte(gdhx8347p);
    gdhx8347WriteCommand(gdhx8347p, GD_HX8347_ID_3);
    gdhx8347p->gddi.id[2] = gdhx8347ReadByte(gdhx8347p);
    gdhx8347Unselect(gdhx8347p);

#if 0
    /* Exit deep standby mode sequence according to datasheet. */
    gdhx8347Select(gdhx8347p);

    /* OSC control setting */
    gdhx8347WriteCommand(gdhx8347p, 0x19); /* OSC Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x87);

    chThdSleepMilliseconds(10);
    gdhx8347WriteCommand(gdhx8347p, 0x1b); /* Power Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x0c);

    /* Power supply setting initializing */
    gdhx8347WriteCommand(gdhx8347p, 0x20); /* Power Control 6 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x1f); /* Power Control 5 */
    gdhx8347WriteByte(gdhx8347p, 0x07);
    gdhx8347WriteCommand(gdhx8347p, 0x44); /* VCOM Control 2 */
    gdhx8347WriteByte(gdhx8347p, 0x7f);
    gdhx8347WriteCommand(gdhx8347p, 0x45); /* VCOM Control 3 */
    gdhx8347WriteByte(gdhx8347p, 0x14);
    gdhx8347WriteCommand(gdhx8347p, 0x1d); /* Power Control 3 */
    gdhx8347WriteByte(gdhx8347p, 0x05);
    gdhx8347WriteCommand(gdhx8347p, 0x1e); /* Power Control 4 */
    gdhx8347WriteByte(gdhx8347p, 0x00);

    /* Power supply operation start setting */
    gdhx8347WriteCommand(gdhx8347p, 0x1c); /* Power Control 2 */
    gdhx8347WriteByte(gdhx8347p, 0x04);
    gdhx8347WriteCommand(gdhx8347p, 0x1b); /* Power Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x14);
    chThdSleepMilliseconds(40);
    gdhx8347WriteCommand(gdhx8347p, 0x43); /* VCOM Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x80);
    chThdSleepMilliseconds(60);

    /* Display on setting */
    gdhx8347WriteCommand(gdhx8347p, 0x90); /* Display Control 8 */
    gdhx8347WriteByte(gdhx8347p, 0x7f);

    /* TEST1 setting */
    gdhx8347WriteCommand(gdhx8347p, 0x96); /* TEST1 */
    gdhx8347WriteByte(gdhx8347p, 0x01);

    /* Display on setting */
    gdhx8347WriteCommand(gdhx8347p, 0x26); /* Display Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x84);
    chThdSleepMilliseconds(40);
    gdhx8347WriteCommand(gdhx8347p, 0x26); /* Display Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0xa4);
    gdhx8347WriteCommand(gdhx8347p, 0x26); /* Display Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0xac);
    chThdSleepMilliseconds(40);
    gdhx8347WriteCommand(gdhx8347p, 0x26); /* Display Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0xbc);

    /* TEST1 setting */
    gdhx8347WriteCommand(gdhx8347p, 0x96); /* TEST1 */
    gdhx8347WriteByte(gdhx8347p, 0x00);

    gdhx8347Unselect(gdhx8347p);
#endif

    /* Power on sequence according to datasheet. */
    gdhx8347Select(gdhx8347p);

    /* TEST1 setting */
    gdhx8347WriteCommand(gdhx8347p, 0x96); /* TEST1 */
    gdhx8347WriteByte(gdhx8347p, 0x01);
    gdhx8347WriteCommand(gdhx8347p, 0x19); /* OSC Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x87);

    chThdSleepMilliseconds(10);

    /* Display OFF setting */
    gdhx8347WriteCommand(gdhx8347p, 0x26); /* Display Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x80);
    gdhx8347WriteCommand(gdhx8347p, 0x1b); /* Power Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x0c);
    gdhx8347WriteCommand(gdhx8347p, 0x43); /* VCOM Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x00);

    /* Power supply setting initializing */
    gdhx8347WriteCommand(gdhx8347p, 0x20); /* Power Control 6 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x1f); /* Power Control 5 */
    gdhx8347WriteByte(gdhx8347p, 0x07);
    gdhx8347WriteCommand(gdhx8347p, 0x44); /* VCOM Control 2 */
    gdhx8347WriteByte(gdhx8347p, 0x7f);
    gdhx8347WriteCommand(gdhx8347p, 0x45); /* VCOM Control 3 */
    gdhx8347WriteByte(gdhx8347p, 0x14);
    gdhx8347WriteCommand(gdhx8347p, 0x1d); /* Power Control 3 */
    gdhx8347WriteByte(gdhx8347p, 0x05);
    gdhx8347WriteCommand(gdhx8347p, 0x1e); /* Power Control 4 */
    gdhx8347WriteByte(gdhx8347p, 0x00);

    /* Power supply operation start setting */
    gdhx8347WriteCommand(gdhx8347p, 0x1c); /* Power Control 2 */
    gdhx8347WriteByte(gdhx8347p, 0x04);
    gdhx8347WriteCommand(gdhx8347p, 0x1b); /* Power Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x14);
    chThdSleepMilliseconds(40);
    gdhx8347WriteCommand(gdhx8347p, 0x43); /* VCOM Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x80);

    /* Power control setting */
    gdhx8347WriteCommand(gdhx8347p, 0x42); /* BGP Control */
    gdhx8347WriteByte(gdhx8347p, 0x08);
    gdhx8347WriteCommand(gdhx8347p, 0x23); /* Cycle Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x95);
    gdhx8347WriteCommand(gdhx8347p, 0x24); /* Cycle Control 2 */
    gdhx8347WriteByte(gdhx8347p, 0x95);
    gdhx8347WriteCommand(gdhx8347p, 0x25); /* Cycle Control 3 */
    gdhx8347WriteByte(gdhx8347p, 0xff);
    gdhx8347WriteCommand(gdhx8347p, 0x21); /* Power Control 7 */
    gdhx8347WriteByte(gdhx8347p, 0x10);
    gdhx8347WriteCommand(gdhx8347p, 0x2b); /* Power Control 11 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x95); /* DCCLK SYNC TO CL1 */
    gdhx8347WriteByte(gdhx8347p, 0x01);

    /* OSC control setting */
    gdhx8347WriteCommand(gdhx8347p, 0x1a); /* OSC Control 2 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x93); /* OSC Control 3 */
    gdhx8347WriteByte(gdhx8347p, 0x0f);
    gdhx8347WriteCommand(gdhx8347p, 0x70); /* Internal Use 28 */
    gdhx8347WriteByte(gdhx8347p, 0x66);
    gdhx8347WriteCommand(gdhx8347p, 0x18); /* Gate Scan control */
    gdhx8347WriteByte(gdhx8347p, 0x01);

    /* r control setting */
    gdhx8347WriteCommand(gdhx8347p, 0x46); /* r control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x86);
    gdhx8347WriteCommand(gdhx8347p, 0x47); /* r control 2 */
    gdhx8347WriteByte(gdhx8347p, 0x60);
    gdhx8347WriteCommand(gdhx8347p, 0x48); /* r control 3 */
    gdhx8347WriteByte(gdhx8347p, 0x01);
    gdhx8347WriteCommand(gdhx8347p, 0x49); /* r control 4 */
    gdhx8347WriteByte(gdhx8347p, 0x67);
    gdhx8347WriteCommand(gdhx8347p, 0x4a); /* r control 5 */
    gdhx8347WriteByte(gdhx8347p, 0x46);
    gdhx8347WriteCommand(gdhx8347p, 0x4b); /* r control 6 */
    gdhx8347WriteByte(gdhx8347p, 0x13);
    gdhx8347WriteCommand(gdhx8347p, 0x4c); /* r control 7 */
    gdhx8347WriteByte(gdhx8347p, 0x01);
    gdhx8347WriteCommand(gdhx8347p, 0x4d); /* r control 8 */
    gdhx8347WriteByte(gdhx8347p, 0x67);
    gdhx8347WriteCommand(gdhx8347p, 0x4e); /* r control 9 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x4f); /* r control 10 */
    gdhx8347WriteByte(gdhx8347p, 0x13);
    gdhx8347WriteCommand(gdhx8347p, 0x50); /* r control 11 */
    gdhx8347WriteByte(gdhx8347p, 0x02);
    gdhx8347WriteCommand(gdhx8347p, 0x51); /* r control 12 */
    gdhx8347WriteByte(gdhx8347p, 0x00);

    /* RGB interface control setting */
    gdhx8347WriteCommand(gdhx8347p, 0x38); /* RGB interface control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x39); /* RGB interface control 2 */
    gdhx8347WriteByte(gdhx8347p, 0x00);

    /* Display control setting */
    gdhx8347WriteCommand(gdhx8347p, 0x27); /* Display Control 2 */
    gdhx8347WriteByte(gdhx8347p, 0x02);
    gdhx8347WriteCommand(gdhx8347p, 0x28); /* Display Control 3 */
    gdhx8347WriteByte(gdhx8347p, 0x03);
    gdhx8347WriteCommand(gdhx8347p, 0x29); /* Display Control 4 */
    gdhx8347WriteByte(gdhx8347p, 0x08);
    gdhx8347WriteCommand(gdhx8347p, 0x2a); /* Display Control 5 */
    gdhx8347WriteByte(gdhx8347p, 0x08);
    gdhx8347WriteCommand(gdhx8347p, 0x2c); /* Display Control 6 */
    gdhx8347WriteByte(gdhx8347p, 0x08);
    gdhx8347WriteCommand(gdhx8347p, 0x2d); /* Display Control 7 */
    gdhx8347WriteByte(gdhx8347p, 0x08);
    gdhx8347WriteCommand(gdhx8347p, 0x35); /* Display Control 9 */
    gdhx8347WriteByte(gdhx8347p, 0x09);
    gdhx8347WriteCommand(gdhx8347p, 0x36); /* Display Control 10 */
    gdhx8347WriteByte(gdhx8347p, 0x09);
    gdhx8347WriteCommand(gdhx8347p, 0x91); /* Display Control 11 */
    gdhx8347WriteByte(gdhx8347p, 0x14);
    gdhx8347WriteCommand(gdhx8347p, 0x37); /* Display Control 12 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x01); /* Display Mode control */
    gdhx8347WriteByte(gdhx8347p, 0x06);
    gdhx8347WriteCommand(gdhx8347p, 0x3a); /* Cycle Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0xa1);
    gdhx8347WriteCommand(gdhx8347p, 0x3b); /* Cycle Control 2 */
    gdhx8347WriteByte(gdhx8347p, 0xa1);
    gdhx8347WriteCommand(gdhx8347p, 0x3c); /* Cycle Control 3 */
    gdhx8347WriteByte(gdhx8347p, 0xa0);
    gdhx8347WriteCommand(gdhx8347p, 0x3d); /* Cycle Control 4 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x3e); /* Cycle Control 5 */
    gdhx8347WriteByte(gdhx8347p, 0x2d);
    gdhx8347WriteCommand(gdhx8347p, 0x40); /* Cycle Control 6 */
    gdhx8347WriteByte(gdhx8347p, 0x03);
    gdhx8347WriteCommand(gdhx8347p, 0x41); /* Cycle Control 7 */
    gdhx8347WriteByte(gdhx8347p, 0xcc);

    /* Partial Image Display setting */
    gdhx8347WriteCommand(gdhx8347p, 0x0a); /* Partial area start row 2 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x0b); /* Partial area start row 1 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x0c); /* Partial area end row 2 */
    gdhx8347WriteByte(gdhx8347p, 0x01);
    gdhx8347WriteCommand(gdhx8347p, 0x0d); /* Partial area end row 1 */
    gdhx8347WriteByte(gdhx8347p, 0x3f);

    /* Vertical Scroll setting */
    gdhx8347WriteCommand(gdhx8347p, 0x0e); /* Vertical Scroll Top fixed area 2 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x0f); /* Vertical Scroll Top fixed area 1 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x10); /* Vertical Scroll height area 2 */
    gdhx8347WriteByte(gdhx8347p, 0x01);
    gdhx8347WriteCommand(gdhx8347p, 0x11); /* Vertical Scroll height area 1 */
    gdhx8347WriteByte(gdhx8347p, 0x40);
    gdhx8347WriteCommand(gdhx8347p, 0x12); /* Vertical Scroll button area 2 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x13); /* Vertical Scroll button area 1 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x14); /* Vertical Scroll Start address 2 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x15); /* Vertical Scroll Start address 1 */
    gdhx8347WriteByte(gdhx8347p, 0x00);

    /* Window address setting */
    gdhx8347WriteCommand(gdhx8347p, 0x02); /* Column address start 2 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x03); /* Column address start 1 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x04); /* Column address end 2 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x05); /* Column address end 1 */
    gdhx8347WriteByte(gdhx8347p, 0xef);
    gdhx8347WriteCommand(gdhx8347p, 0x06); /* Row address start 2 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x07); /* Row address start 1 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x08); /* Row address end 2 */
    gdhx8347WriteByte(gdhx8347p, 0x01);
    gdhx8347WriteCommand(gdhx8347p, 0x09); /* Row address end 1 */
    gdhx8347WriteByte(gdhx8347p, 0x3f);
    gdhx8347WriteCommand(gdhx8347p, 0x16); /* Memory Access control */
    gdhx8347WriteByte(gdhx8347p, 0x08);
    gdhx8347WriteCommand(gdhx8347p, 0x72); /* Data control */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x22); /* Write Data */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    chThdSleepMilliseconds(60);

    gdhx8347WriteCommand(gdhx8347p, 0x16); /* Memory Access control */
    gdhx8347WriteByte(gdhx8347p, 0x68); /* MV | MX */

    /* Clear memory. */
    gdhx8347Unselect(gdhx8347p);
    gdhx8347RectFill(gdhx8347p, 0, 0, gdhx8347p->config->size_x,
            gdhx8347p->config->size_y, 0);
    gdhx8347Select(gdhx8347p);

    /* Display on setting */
    gdhx8347WriteCommand(gdhx8347p, 0x94); /* SAP Idle mode */
    gdhx8347WriteByte(gdhx8347p, 0x0a);
    gdhx8347WriteCommand(gdhx8347p, 0x90); /* Display Control 8 */
    gdhx8347WriteByte(gdhx8347p, 0x7f);
    gdhx8347WriteCommand(gdhx8347p, 0x26); /* Display Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x84);
    chThdSleepMilliseconds(40);
    gdhx8347WriteCommand(gdhx8347p, 0x26); /* Display Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0xa4);
    gdhx8347WriteCommand(gdhx8347p, 0x26); /* Display Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0xac);
    chThdSleepMilliseconds(40);
    gdhx8347WriteCommand(gdhx8347p, 0x26); /* Display Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0xbc);

    /* TEST1 setting */
    gdhx8347WriteCommand(gdhx8347p, 0x96); /* TEST1 */
    gdhx8347WriteByte(gdhx8347p, 0x00);

    gdhx8347Unselect(gdhx8347p);

    /* Unlock device. */
    gdhx8347ReleaseBus(gdhx8347p);
}

/**
 * @brief   Disables the graphics display peripheral.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 *
 * @api
 */
void gdhx8347Stop(GDHX8347Driver* gdhx8347p)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347Stop");
    /* Verify device status. */
    chDbgAssert((gdhx8347p->state == GD_STOP) || (gdhx8347p->state == GD_READY),
            "gdhx8347Stop(), #1", "invalid state");

    gdhx8347AcquireBus(gdhx8347p);
    gdhx8347Select(gdhx8347p);

    /* TEST1 setting */
    gdhx8347WriteCommand(gdhx8347p, 0x96); /* TEST1 */
    gdhx8347WriteByte(gdhx8347p, 0x01);
    gdhx8347WriteCommand(gdhx8347p, 0x26); /* Display Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0xb8);
    chThdSleepMilliseconds(40);
    gdhx8347WriteCommand(gdhx8347p, 0x26); /* Display Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0xa8);
    gdhx8347WriteCommand(gdhx8347p, 0x26); /* Display Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x84);
    chThdSleepMilliseconds(40);
    gdhx8347WriteCommand(gdhx8347p, 0x26); /* Display Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x80);

    /* Power off setting */
    gdhx8347WriteCommand(gdhx8347p, 0x90); /* Display Control 8 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x1c); /* Display Control 2 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x1b); /* Power Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x04);
    gdhx8347WriteCommand(gdhx8347p, 0x43); /* VCOM Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x00);
    gdhx8347WriteCommand(gdhx8347p, 0x1b); /* Power Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x0c);

    /* TEST1 setting */
    gdhx8347WriteCommand(gdhx8347p, 0x96); /* TEST1 */
    gdhx8347WriteByte(gdhx8347p, 0x00);

    /* Power off setting */
    gdhx8347WriteCommand(gdhx8347p, 0x1b); /* Power Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x0d);

    /* OSC control setting */
    gdhx8347WriteCommand(gdhx8347p, 0x19); /* OSC Control 1 */
    gdhx8347WriteByte(gdhx8347p, 0x86);

    gdhx8347Unselect(gdhx8347p);
    gdhx8347ReleaseBus(gdhx8347p);

    gdhx8347p->state = GD_STOP;
}

/**
 * @brief   Sets pixel color.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 * @param[in] gddip         pointer to a @p GDDeviceInfo structure
 *
 * @api
 */
void gdhx8347PixelSet(GDHX8347Driver* gdhx8347p, coord_t x, coord_t y, color_t color)
{
    chDbgCheck(gdhx8347p != NULL, "gdPixelSet");
    /* Verify device status. */
    chDbgAssert(gdhx8347p->state >= GD_READY, "gdPixelSet(), #1",
            "invalid state");

    gdhx8347Select(gdhx8347p);

    column_address_set(gdhx8347p, x, x);
    page_address_set(gdhx8347p, y, y);

    gdhx8347WriteCommand(gdhx8347p, GD_HX8347_WRITE_DATA);

    uint8_t temp[2];

    temp[0] =
            (((color & 0xff0000) >> 16 >> 3) << 3) |
            (((color & 0x00ff00) >> 8 >> 2) >> 3);

    temp[1] =
            (((color & 0x00ff00) >> 8 >> 2) << 5) |
            (((color & 0x0000ff) >> 0 >> 3) << 0);

    gdhx8347WriteChunk(gdhx8347p, temp, sizeof(temp));

    gdhx8347Unselect(gdhx8347p);
}

/**
 * @brief   Starts streamed writing into a window.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 * @param[in] left          left window border coordinate
 * @param[in] top           top window border coordinate
 * @param[in] width         height of the window
 * @param[in] height        width of the window
 *
 * @api
 */
void gdhx8347StreamStart(GDHX8347Driver* gdhx8347p, coord_t left, coord_t top,
        coord_t width, coord_t height)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347StreamStart");
    /* Verify device status. */
    chDbgAssert(gdhx8347p->state >= GD_READY, "gdhx8347StreamStart(), #1",
            "invalid state");

    gdhx8347Select(gdhx8347p);

    column_address_set(gdhx8347p, left, left + width - 1);
    page_address_set(gdhx8347p, top, top + height - 1);

    gdhx8347WriteCommand(gdhx8347p, GD_HX8347_WRITE_DATA);
}

/**
 * @brief   Write a chunk of data in stream mode.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 * @param[in] data[]        array of color_t data
 * @param[in] n             number of array elements
 *
 * @api
 */
void gdhx8347StreamWrite(GDHX8347Driver* gdhx8347p, const color_t data[], size_t n)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347StreamWrite");
    /* Verify device status. */
    chDbgAssert(gdhx8347p->state >= GD_ACTIVE, "gdhx8347StreamWrite(), #1",
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

        gdhx8347WriteChunk(gdhx8347p, temp, sizeof(temp));
    }
}

/**
 * @brief   End stream mode writing.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 *
 * @api
 */
void gdhx8347StreamEnd(GDHX8347Driver* gdhx8347p)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347StreamEnd");
    /* Verify device status. */
    chDbgAssert(gdhx8347p->state >= GD_ACTIVE, "gdhx8347StreamEnd(), #1",
            "invalid state");

    gdhx8347Unselect(gdhx8347p);
}

/**
 * @brief   Fills a rectangle with a color.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 * @param[in] left          left rectangle border coordinate
 * @param[in] top           top rectangle border coordinate
 * @param[in] width         height of the rectangle
 * @param[in] height        width of the rectangle
 *
 * @api
 */
void gdhx8347RectFill(GDHX8347Driver* gdhx8347p, coord_t left, coord_t top,
        coord_t width, coord_t height, color_t color)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347RectFill");
    /* Verify device status. */
    chDbgAssert(gdhx8347p->state >= GD_READY, "gdhx8347RectFill(), #1",
            "invalid state");

    gdhx8347Select(gdhx8347p);

    column_address_set(gdhx8347p, left, left + width - 1);
    page_address_set(gdhx8347p, top, top + height - 1);

    gdhx8347WriteCommand(gdhx8347p, GD_HX8347_WRITE_DATA);

    uint8_t temp[2];

    temp[0] =
            (((color & 0xff0000) >> 16 >> 3) << 3) |
            (((color & 0x00ff00) >> 8 >> 2) >> 3);

    temp[1] =
            (((color & 0x00ff00) >> 8 >> 2) << 5) |
            (((color & 0x0000ff) >> 0 >> 3) << 0);

    uint8_t buffer[2 * 16];
    for (size_t i = 0; i < sizeof(buffer); i += 2)
    {
        buffer[i + 0] = temp[0];
        buffer[i + 1] = temp[1];
    }

    size_t n = (size_t)width * height;

    for (size_t i = 0; i < n; i += sizeof(buffer) / 2)
        gdhx8347WriteChunk(gdhx8347p, buffer, sizeof(buffer));

    gdhx8347Unselect(gdhx8347p);
}

/**
 * @brief   Returns device info.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 * @param[out] gddip        pointer to a @p GDDeviceInfo structure
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t gdhx8347GetInfo(GDHX8347Driver* gdhx8347p, GDDeviceInfo* gddip)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347GetInfo");
    /* Verify device status. */
    chDbgAssert(gdhx8347p->state >= GD_READY, "gdhx8347GetInfo(), #1",
            "invalid state");

    memcpy(gddip, &gdhx8347p->gddi, sizeof(*gddip));

    return CH_SUCCESS;
}

/**
 * @brief   Gains exclusive access to the graphics display device.
 * @details This function tries to gain ownership to the graphics display
 *          device, if the device is already being used then the invoking
 *          thread is queued.
 * @pre     In order to use this function the option
 *          @p GD_HX8347_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 *
 * @api
 */
void gdhx8347AcquireBus(GDHX8347Driver* gdhx8347p)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347AcquireBus");

#if GD_HX8347_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    chMtxLock(&gdhx8347p->mutex);
#elif CH_USE_SEMAPHORES
    chSemWait(&gdhx8347p->semaphore);
#endif
#endif /* GD_HX8347_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the graphics display device.
 * @pre     In order to use this function the option
 *          @p GD_HX8347_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 *
 * @api
 */
void gdhx8347ReleaseBus(GDHX8347Driver* gdhx8347p)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347ReleaseBus");

#if GD_HX8347_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    (void)gdhx8347p;
    chMtxUnlock();
#elif CH_USE_SEMAPHORES
    chSemSignal(&gdhx8347p->semaphore);
#endif
#endif /* GD_HX8347_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Asserts the chip select signal and prepares for transfers.
 * @pre     HX8347 is ready.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 *
 * @api
 */
void gdhx8347Select(GDHX8347Driver* gdhx8347p)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347SelectI");
    chDbgAssert(gdhx8347p->state == GD_READY,
            "gdhx8347SelectI(), #1", "invalid state");

    gdhx8347p->state = GD_ACTIVE;
    palClearPad(gdhx8347p->config->csx_port, gdhx8347p->config->csx_pad);
}

/**
 * @brief   Deasserts the chip select signal.
 * @details The previously selected peripheral is unselected.
 * @pre     HX8347 is active.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 *
 * @iclass
 */
void gdhx8347Unselect(GDHX8347Driver* gdhx8347p)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347Unselect");
    chDbgAssert(gdhx8347p->state == GD_ACTIVE,
            "gdhx8347UnselectI(), #1", "invalid state");

    palSetPad(gdhx8347p->config->csx_port, gdhx8347p->config->csx_pad);
    gdhx8347p->state = GD_READY;
}

/**
 * @brief   Write command byte.
 * @details Sends a command byte.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 * @param[in] cmd           command byte
 *
 * @api
 */
void gdhx8347WriteCommand(GDHX8347Driver* gdhx8347p, uint8_t cmd)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347WriteCommand");
    chDbgAssert(gdhx8347p->state == GD_ACTIVE,
            "gdhx8347WriteCommand(), #1", "invalid state");

    gdhx8347p->value = cmd;
    /* Cmd */
    palClearPad(gdhx8347p->config->dcx_port, gdhx8347p->config->dcx_pad);
    palSetBusMode(gdhx8347p->config->db_bus,
            PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
    palClearPad(gdhx8347p->config->wrx_port, gdhx8347p->config->wrx_pad);
    palWriteBus(gdhx8347p->config->db_bus, cmd);
    palSetPad(gdhx8347p->config->wrx_port, gdhx8347p->config->wrx_pad);
}

/**
 * @brief   Write data byte.
 * @details Sends a data byte.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 * @param[in] value         data byte
 *
 * @api
 */
void gdhx8347WriteByte(GDHX8347Driver* gdhx8347p, uint8_t value)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347WriteByte");
    chDbgAssert(gdhx8347p->state == GD_ACTIVE,
            "gdhx8347WriteByte(), #1", "invalid state");

    gdhx8347p->value = value;
    /* Data */
    palSetPad(gdhx8347p->config->dcx_port, gdhx8347p->config->dcx_pad);
    palSetBusMode(gdhx8347p->config->db_bus,
            PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
    palClearPad(gdhx8347p->config->wrx_port, gdhx8347p->config->wrx_pad);
    palWriteBus(gdhx8347p->config->db_bus, value);
    palSetPad(gdhx8347p->config->wrx_port, gdhx8347p->config->wrx_pad);
}

/**
 * @brief   Read data byte.
 * @details Receives a data byte.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 *
 * @return                  data byte
 *
 * @api
 */
uint8_t gdhx8347ReadByte(GDHX8347Driver* gdhx8347p)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347ReadByte");
    chDbgAssert(gdhx8347p->state == GD_ACTIVE,
            "gdhx8347ReadByte(), #1", "invalid state");

    /* Data */
    palSetPad(gdhx8347p->config->dcx_port, gdhx8347p->config->dcx_pad);
    palSetBusMode(gdhx8347p->config->db_bus, PAL_MODE_INPUT);
    palClearPad(gdhx8347p->config->rdx_port, gdhx8347p->config->rdx_pad);
    uint8_t value = palReadBus(gdhx8347p->config->db_bus);
    palSetPad(gdhx8347p->config->rdx_port, gdhx8347p->config->rdx_pad);
    return value;
}

/**
 * @brief   Write data chunk.
 * @details Sends a data chunk.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 * @param[in] chunk         chunk bytes
 * @param[in] length        chunk length
 *
 * @api
 */
void gdhx8347WriteChunk(GDHX8347Driver* gdhx8347p, const uint8_t chunk[],
        size_t length)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347WriteChunk");
    chDbgCheck(chunk != NULL, "gdhx8347WriteChunk");
    chDbgAssert(gdhx8347p->state == GD_ACTIVE,
              "gdhx8347WriteChunk(), #1", "invalid state");

    /* Data */
    palSetPad(gdhx8347p->config->dcx_port, gdhx8347p->config->dcx_pad);
    palSetBusMode(gdhx8347p->config->db_bus,
            PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
    for (size_t i = 0; i < length; i += 2)
    {
        palClearPad(gdhx8347p->config->wrx_port, gdhx8347p->config->wrx_pad);
        palWriteBus(gdhx8347p->config->db_bus,
                (chunk[i + 0] << 8) | (chunk[i + 1] << 0));
        palSetPad(gdhx8347p->config->wrx_port, gdhx8347p->config->wrx_pad);
    }
}

/**
 * @brief   Read data chunk.
 * @details Receives a data chunk.
 *
 * @param[in] gdhx8347p     pointer to the @p GDHX8347Driver object
 * @param[out] chunk        chunk bytes
 * @param[in] length        chunk length
 *
 * @api
 */
void gdhx8347ReadChunk(GDHX8347Driver* gdhx8347p, uint8_t chunk[],
        size_t length)
{
    chDbgCheck(gdhx8347p != NULL, "gdhx8347ReadChunk");
    chDbgCheck(chunk != NULL, "gdhx8347ReadChunk");
    chDbgAssert(gdhx8347p->state == GD_ACTIVE,
            "gdhx8347ReadChunk(), #1", "invalid state");

    /* Data */
    palSetPad(gdhx8347p->config->dcx_port, gdhx8347p->config->dcx_pad);
    palSetBusMode(gdhx8347p->config->db_bus, PAL_MODE_INPUT);
    for (size_t i = 0; i < length; ++i)
    {
        palClearPad(gdhx8347p->config->rdx_port, gdhx8347p->config->rdx_pad);
        chunk[i] = palReadBus(gdhx8347p->config->db_bus);
        palSetPad(gdhx8347p->config->rdx_port, gdhx8347p->config->rdx_pad);
    }
}

#endif /* HAL_USE_GD_HX8347 */

/** @} */
