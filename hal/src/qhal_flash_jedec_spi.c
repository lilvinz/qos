/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    qflash_jedec_spi.c
 * @brief   FLASH JEDEC over SPI driver code.
 *
 * @addtogroup FLASH_JEDEC_SPI
 * @{
 */

#include "qhal.h"

#if HAL_USE_FLASH_JEDEC_SPI || defined(__DOXYGEN__)

#include "static_assert.h"
#include "nelems.h"

/**
 * @todo    - add efficient use of AAI writing for chips which support it
 *          - add error detection and handling
 */

/**
 * @note
 *      - Busy Flag handling:
 *        According to M25P64 and SST25VF032B datasheets it is necessary
 *        to check WIP/BUSY-Bit prior to any new command.
 *      - Write enable latch:
 *        - M25P64: the write enable latch bit is automatically reset on
 *          completion of write operation.
 *        - SST25VF032: the write enable latch bit must be manually reset on
 *          completion of AAI write operation.
 */

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define FLASH_JEDEC_WREN 0x06
#define FLASH_JEDEC_WRDI 0x04
#define FLASH_JEDEC_RDID 0x9f
#define FLASH_JEDEC_RDSR 0x05
#define FLASH_JEDEC_WRSR 0x01
#define FLASH_JEDEC_FAST_READ 0x0b
#define FLASH_JEDEC_MASS_ERASE 0xc7

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Virtual methods table.
 */
static const struct FlashJedecSPIDriverVMT flash_jedec_spi_vmt =
{
    .read = (bool (*)(void*, uint32_t, uint32_t, uint8_t*))fjsRead,
    .write = (bool (*)(void*, uint32_t, uint32_t, const uint8_t*))fjsWrite,
    .erase = (bool (*)(void*, uint32_t, uint32_t))fjsErase,
    .mass_erase = (bool (*)(void*))fjsMassErase,
    .sync = (bool (*)(void*))fjsSync,
    .get_info = (bool (*)(void*, NVMDeviceInfo*))fjsGetInfo,
    /* End of mandatory functions. */
    .acquire = (void (*)(void*))fjsAcquireBus,
    .release = (void (*)(void*))fjsReleaseBus,
    .writeprotect = (bool (*)(void*, uint32_t, uint32_t))fjsWriteProtect,
    .mass_writeprotect = (bool (*)(void*))fjsMassWriteProtect,
    .writeunprotect = (bool (*)(void*, uint32_t, uint32_t))fjsWriteUnprotect,
    .mass_writeunprotect = (bool (*)(void*))fjsMassWriteUnprotect,
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void flash_jedec_spi_reconfigure(FlashJedecSPIDriver* fjsp)
{
    /* Set slave specific bus configuration if defined. */
    if (fjsp->config->spi_cfgp)
        spiStart(fjsp->config->spip, fjsp->config->spi_cfgp);
}

static void flash_jedec_spi_write_enable(FlashJedecSPIDriver* fjsp)
{
    osalDbgCheck((fjsp != NULL));

    spiSelect(fjsp->config->spip);

    /* Send JEDEC WREN command. */
    static const uint8_t out[] =
    {
        FLASH_JEDEC_WREN,
    };

    spiSend(fjsp->config->spip, NELEMS(out), out);

    spiUnselect(fjsp->config->spip);
}

static void flash_jedec_spi_write_disable(FlashJedecSPIDriver* fjsp)
{
    osalDbgCheck((fjsp != NULL));

    spiSelect(fjsp->config->spip);

    /* Send JEDEC WRDI command. */
    static const uint8_t out[] =
    {
        FLASH_JEDEC_WRDI,
    };

    spiSend(fjsp->config->spip, NELEMS(out), out);

    spiUnselect(fjsp->config->spip);
}

static void flash_jedec_spi_wait_busy(FlashJedecSPIDriver* fjsp)
{
    osalDbgCheck((fjsp != NULL));
    spiSelect(fjsp->config->spip);

    static const uint8_t out[] =
    {
        FLASH_JEDEC_RDSR,
    };

    spiSend(fjsp->config->spip, NELEMS(out), out);

    uint8_t in;
    for (uint8_t i = 0; i < 16; ++i)
    {
        spiReceive(fjsp->config->spip, sizeof(in), &in);
        if ((in & 0x01) == 0x00)
            break;
    }

    /* Looks like it is a long wait. */
    while ((in & 0x01) != 0x00)
    {
#if FLASH_JEDEC_SPI_NICE_WAITING
        /* Trying to be nice with the other threads. */
        osalThreadSleep(1);
#endif
        spiReceive(fjsp->config->spip, sizeof(in), &in);
    }

    spiUnselect(fjsp->config->spip);
}

static uint8_t flash_jedec_spi_sr_read(FlashJedecSPIDriver* fjsp)
{
    osalDbgCheck((fjsp != NULL));

    spiSelect(fjsp->config->spip);

    static const uint8_t out[] =
    {
        FLASH_JEDEC_RDSR,
    };

    /* command bytes */
    spiSend(fjsp->config->spip, NELEMS(out), out);

    uint8_t in;
    spiReceive(fjsp->config->spip, sizeof(in), &in);

    spiUnselect(fjsp->config->spip);

    return in;
}

static void flash_jedec_spi_sr_write(FlashJedecSPIDriver* fjsp, uint8_t sr)
{
    osalDbgCheck((fjsp != NULL));

    flash_jedec_spi_wait_busy(fjsp);

    flash_jedec_spi_write_enable(fjsp);

    spiSelect(fjsp->config->spip);

    uint8_t out[] =
    {
        FLASH_JEDEC_WRSR,
        sr,
    };

    /* command bytes */
    spiSend(fjsp->config->spip, NELEMS(out), out);

    spiUnselect(fjsp->config->spip);
}

static void flash_jedec_spi_page_program(FlashJedecSPIDriver* fjsp,
        uint32_t startaddr, uint32_t n, const uint8_t* buffer)
{
    osalDbgCheck(fjsp != NULL);

    flash_jedec_spi_wait_busy(fjsp);

    flash_jedec_spi_write_enable(fjsp);

    uint32_t pre_pad = 0;
    uint32_t post_pad = 0;
    if (fjsp->config->page_alignment > 0)
    {
        pre_pad = startaddr % fjsp->config->page_alignment;
        post_pad = (startaddr + n) % fjsp->config->page_alignment;
    }

    spiSelect(fjsp->config->spip);

    const uint8_t out[] =
    {
        fjsp->config->cmd_page_program,
        ((startaddr - pre_pad) >> 24) & 0xff,
        ((startaddr - pre_pad) >> 16) & 0xff,
        ((startaddr - pre_pad) >> 8) & 0xff,
        ((startaddr - pre_pad) >> 0) & 0xff,
    };

    /* command byte */
    spiSend(fjsp->config->spip, 1, &out[0]);

    /* address bytes */
    spiSend(fjsp->config->spip, fjsp->config->addrbytes_num,
            &out[NELEMS(out) - fjsp->config->addrbytes_num]);

    /* pre_pad */
    {
        static const uint8_t erased = 0xff;
        for (uint32_t i = 0; i < pre_pad; ++i)
            spiSend(fjsp->config->spip, sizeof(erased), &erased);
    }

    /* data buffer */
    spiSend(fjsp->config->spip, n, buffer);

    /* post_pad */
    {
        static const uint8_t erased = 0xff;
        for (uint32_t i = 0; i < post_pad; ++i)
            spiSend(fjsp->config->spip, sizeof(erased), &erased);
    }

    spiUnselect(fjsp->config->spip);

    /* note: This is required to terminate AAI programming on some chips. */
    if (fjsp->config->cmd_page_program == 0xad)
    {
        flash_jedec_spi_wait_busy(fjsp);
        flash_jedec_spi_write_disable(fjsp);
    }
}

static void flash_jedec_spi_sector_erase(FlashJedecSPIDriver* fjsp,
        uint32_t startaddr)
{
    osalDbgCheck(fjsp != NULL);

    flash_jedec_spi_wait_busy(fjsp);

    flash_jedec_spi_write_enable(fjsp);

    spiSelect(fjsp->config->spip);

    const uint8_t out[] =
    {
        fjsp->config->cmd_sector_erase, /* Erase command is chip specific. */
        (startaddr >> 24) & 0xff,
        (startaddr >> 16) & 0xff,
        (startaddr >> 8) & 0xff,
        (startaddr >> 0) & 0xff,
    };

    /* command byte */
    spiSend(fjsp->config->spip, 1, &out[0]);

    /* address bytes */
    spiSend(fjsp->config->spip, fjsp->config->addrbytes_num,
            &out[NELEMS(out) - fjsp->config->addrbytes_num]);

    spiUnselect(fjsp->config->spip);
}

static void flash_jedec_spi_page_program_ff(FlashJedecSPIDriver* fjsp,
        uint32_t startaddr)
{
    osalDbgCheck(fjsp != NULL);

    flash_jedec_spi_wait_busy(fjsp);

    flash_jedec_spi_write_enable(fjsp);

    spiSelect(fjsp->config->spip);

    const uint8_t out[] =
    {
        fjsp->config->cmd_page_program,
        (startaddr >> 24) & 0xff,
        (startaddr >> 16) & 0xff,
        (startaddr >> 8) & 0xff,
        (startaddr >> 0) & 0xff,
    };

    /* command byte */
    spiSend(fjsp->config->spip, 1, &out[0]);

    /* address bytes */
    spiSend(fjsp->config->spip, fjsp->config->addrbytes_num,
            &out[NELEMS(out) - fjsp->config->addrbytes_num]);

    /* dummy data */
    static const uint8_t erased = 0xff;
    for (uint32_t i = 0; i < fjsp->config->page_size; ++i)
        spiSend(fjsp->config->spip, sizeof(erased), &erased);

    spiUnselect(fjsp->config->spip);

    /* note: This is required to terminate AAI programming on some chips. */
    if (fjsp->config->cmd_page_program == 0xad)
    {
        flash_jedec_spi_wait_busy(fjsp);
        flash_jedec_spi_write_disable(fjsp);
    }
}

static void flash_jedec_spi_mass_erase(FlashJedecSPIDriver* fjsp)
{
    osalDbgCheck(fjsp != NULL);

    flash_jedec_spi_wait_busy(fjsp);

    flash_jedec_spi_write_enable(fjsp);

    spiSelect(fjsp->config->spip);

    const uint8_t out[] =
    {
        FLASH_JEDEC_MASS_ERASE
    };

    /* command byte */
    spiSend(fjsp->config->spip, 1, &out[0]);

    spiUnselect(fjsp->config->spip);
}

/**
 * @brief   Convertes block protection bits into address of first
 *          protected block.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 * @param[in] bp        block protection bits
 *
 * @return              Address of first protected block.
 *
 * @noapi
 */
static uint32_t flash_jedec_spi_bp_to_address(FlashJedecSPIDriver* fjsp, uint8_t bp)
{
    static const uint8_t number_of_parts_table[] =
    {
        1,
        2,
        4,
        64,
    };

    uint8_t number_of_parts = number_of_parts_table[fjsp->config->bpbits_num];

    uint8_t number_of_protected_parts = 0;

    if (bp > 0)
    {
        number_of_protected_parts = 1 << (bp - 1);
    }

    uint32_t part_size = fjsp->config->sector_size * fjsp->config->sector_num / number_of_parts;

    uint8_t first_protected_part = number_of_parts - number_of_protected_parts;

    uint32_t first_protected_address = first_protected_part * part_size;

    return first_protected_address;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   FLASH JEDEC over SPI driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void fjsInit(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] fjsp     pointer to the @p FlashJedecSPIDriver object
 *
 * @init
 */
void fjsObjectInit(FlashJedecSPIDriver* fjsp)
{
    fjsp->vmt = &flash_jedec_spi_vmt;
    fjsp->state = NVM_STOP;
    fjsp->config = NULL;
#if FLASH_JEDEC_SPI_USE_MUTUAL_EXCLUSION
    osalMutexObjectInit(&fjsp->mutex);
#endif /* FLASH_JEDEC_SPI_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the FLASH peripheral.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 * @param[in] config    pointer to the @p FlashJedecSPIConfig object.
 *
 * @api
 */
void fjsStart(FlashJedecSPIDriver* fjsp, const FlashJedecSPIConfig* config)
{
    osalDbgCheck((fjsp != NULL) && (config != NULL));
    /* Verify device status. */
    osalDbgAssert((fjsp->state == NVM_STOP) || (fjsp->state == NVM_READY),
            "invalid state");

#define IS_POW2(x) ((((x) != 0) && !((x) & ((x) - 1))))

    /* Sanity check configuration. */
    osalDbgAssert(
            IS_POW2(config->sector_num) &&
            IS_POW2(config->sector_size) &&
            IS_POW2(config->page_size) &&
            IS_POW2(config->page_alignment) &&
            (config->page_alignment <= config->page_size) &&
            config->bpbits_num <= 3 &&
            config->cmd_read != 0x00,
            "invalid config");

    fjsp->config = config;
    fjsp->state = NVM_READY;
}

/**
 * @brief   Disables the FLASH peripheral.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 *
 * @api
 */
void fjsStop(FlashJedecSPIDriver* fjsp)
{
    osalDbgCheck(fjsp != NULL);
    /* Verify device status. */
    osalDbgAssert((fjsp->state == NVM_STOP) || (fjsp->state == NVM_READY),
            "invalid state");

    fjsp->state = NVM_STOP;
}

/**
 * @brief   Reads data crossing sector boundaries if required.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 * @param[in] startaddr address to start reading from
 * @param[in] n         number of bytes to read
 * @param[in] buffer    pointer to data buffer
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool fjsRead(FlashJedecSPIDriver* fjsp, uint32_t startaddr, uint32_t n,
        uint8_t* buffer)
{
    osalDbgCheck(fjsp != NULL);
    /* Verify device status. */
    osalDbgAssert(fjsp->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    osalDbgAssert((startaddr + n <= fjsp->config->sector_size * fjsp->config->sector_num),
            "invalid parameters");

    if (fjsSync(fjsp) != HAL_SUCCESS)
        return HAL_FAILED;

    /* Read operation in progress. */
    fjsp->state = NVM_READING;

    flash_jedec_spi_reconfigure(fjsp);

    spiSelect(fjsp->config->spip);

    const uint8_t out[] =
    {
        fjsp->config->cmd_read,
        (startaddr >> 24) & 0xff,
        (startaddr >> 16) & 0xff,
        (startaddr >> 8) & 0xff,
        (startaddr >> 0) & 0xff,
    };

    /* command byte */
    spiSend(fjsp->config->spip, 1, &out[0]);

    /* address bytes */
    spiSend(fjsp->config->spip, fjsp->config->addrbytes_num,
            &out[NELEMS(out) - fjsp->config->addrbytes_num]);

    if (fjsp->config->cmd_read == FLASH_JEDEC_FAST_READ)
    {
        /* Dummy byte required for timing. */
        static const uint8_t dummy = 0x00;
        spiSend(fjsp->config->spip, sizeof(dummy), &dummy);
    }

    /* Receive data. */
    spiReceive(fjsp->config->spip, n, buffer);

    spiUnselect(fjsp->config->spip);

    /* Read operation finished. */
    fjsp->state = NVM_READY;

    return HAL_SUCCESS;
}

/**
 * @brief   Writes data crossing sector boundaries if required.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 * @param[in] startaddr address to start writing to
 * @param[in] n         number of bytes to write
 * @param[in] buffer    pointer to data buffer
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool fjsWrite(FlashJedecSPIDriver* fjsp, uint32_t startaddr, uint32_t n,
        const uint8_t* buffer)
{
    osalDbgCheck(fjsp != NULL);
    /* Verify device status. */
    osalDbgAssert(fjsp->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    osalDbgAssert((startaddr + n <= fjsp->config->sector_size * fjsp->config->sector_num),
            "invalid parameters");

    /* Write operation in progress. */
    fjsp->state = NVM_WRITING;

    flash_jedec_spi_reconfigure(fjsp);

    uint32_t written = 0;

    while (written < n)
    {
        uint32_t n_chunk =
                fjsp->config->page_size - ((startaddr + written) % fjsp->config->page_size);
        if (n_chunk > n - written)
            n_chunk = n - written;

        flash_jedec_spi_page_program(fjsp, startaddr + written,
                n_chunk, buffer + written);

        written += n_chunk;
    }

    return HAL_SUCCESS;
}

/**
 * @brief   Erases one or more sectors.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 * @param[in] startaddr address within to be erased sector
 * @param[in] n         number of bytes to erase
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool fjsErase(FlashJedecSPIDriver* fjsp, uint32_t startaddr, uint32_t n)
{
    osalDbgCheck(fjsp != NULL);
    /* Verify device status. */
    osalDbgAssert(fjsp->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    osalDbgAssert((startaddr + n <= fjsp->config->sector_size * fjsp->config->sector_num),
            "invalid parameters");

    /* Erase operation in progress. */
    fjsp->state = NVM_ERASING;

    flash_jedec_spi_reconfigure(fjsp);

    uint32_t first_sector_addr =
            startaddr - (startaddr % fjsp->config->sector_size);

    for (uint32_t addr = first_sector_addr;
            addr < startaddr + n;
            addr += fjsp->config->sector_size)
    {
        /* Check if device supports erase command. */
        if (fjsp->config->cmd_sector_erase != 0x00)
        {
            /* Execute erase sector command. */
            flash_jedec_spi_sector_erase(fjsp, addr);
        }
        else
        {
            /* Emulate erase by writing 0xff. */
            for (uint32_t i = addr;
                    i < addr + fjsp->config->sector_size;
                    i += fjsp->config->page_size)
            {
                flash_jedec_spi_page_program_ff(fjsp, i);
            }
        }
    }

    return HAL_SUCCESS;
}

/**
 * @brief   Erases whole chip.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool fjsMassErase(FlashJedecSPIDriver* fjsp)
{
    osalDbgCheck(fjsp != NULL);
    /* Verify device status. */
    osalDbgAssert(fjsp->state >= NVM_READY, "invalid state");

    /* Erase operation in progress. */
    fjsp->state = NVM_ERASING;

    flash_jedec_spi_reconfigure(fjsp);

    /* Check if device supports erase command. */
    if (fjsp->config->cmd_sector_erase != 0x00)
    {
        /* Yes, so we assume there is mass erase as well. */
        flash_jedec_spi_mass_erase(fjsp);
    }
    else
    {
        /* No, so we will let the sector erase command fake it. */
        return fjsErase(fjsp, 0,
                fjsp->config->sector_size * fjsp->config->sector_num);
    }

    return HAL_SUCCESS;
}

/**
 * @brief   Waits for idle condition.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool fjsSync(FlashJedecSPIDriver* fjsp)
{
    osalDbgCheck(fjsp != NULL);
    /* Verify device status. */
    osalDbgAssert(fjsp->state >= NVM_READY, "invalid state");

    if (fjsp->state == NVM_READY)
        return HAL_SUCCESS;

    flash_jedec_spi_reconfigure(fjsp);

    flash_jedec_spi_wait_busy(fjsp);

    /* No more operation in progress. */
    fjsp->state = NVM_READY;

    return HAL_SUCCESS;
}

/**
 * @brief   Returns media info.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 * @param[out] nvmdip     pointer to a @p NVMDeviceInfo structure
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool fjsGetInfo(FlashJedecSPIDriver* fjsp, NVMDeviceInfo* nvmdip)
{
    osalDbgCheck(fjsp != NULL);
    /* Verify device status. */
    osalDbgAssert(fjsp->state >= NVM_READY, "invalid state");

    flash_jedec_spi_reconfigure(fjsp);

    flash_jedec_spi_wait_busy(fjsp);

    nvmdip->sector_num = fjsp->config->sector_num;
    nvmdip->sector_size = fjsp->config->sector_size;
    /* Note: The lower level driver part pads unaligned writes.
     * This makes sense here as you actually CAN write the chip
     * on a byte by byte basis by padding with 0xff. */
    nvmdip->write_alignment = 0;

    spiSelect(fjsp->config->spip);

    static const uint8_t out[] =
    {
        FLASH_JEDEC_RDID,
    };
    spiSend(fjsp->config->spip, NELEMS(out), out);

    /* Skip JEDEC continuation id. */
    nvmdip->identification[0] = 0x7f;
    while (nvmdip->identification[0] == 0x7f)
        spiReceive(fjsp->config->spip, 1, nvmdip->identification);

    spiReceive(fjsp->config->spip, NELEMS(nvmdip->identification) - 1,
            nvmdip->identification + 1);

    spiUnselect(fjsp->config->spip);

    return HAL_SUCCESS;
}

/**
 * @brief   Gains exclusive access to the flash device.
 * @details This function tries to gain ownership to the flash device, if the
 *          device is already being used then the invoking thread is queued.
 * @pre     In order to use this function the option
 *          @p FLASH_JEDEC_SPI_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 *
 * @api
 */
void fjsAcquireBus(FlashJedecSPIDriver* fjsp)
{
    osalDbgCheck(fjsp != NULL);

#if FLASH_JEDEC_SPI_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
    osalMutexLock(&fjsp->mutex);

#if SPI_USE_MUTUAL_EXCLUSION
    /* Acquire the underlying device as well. */
    spiAcquireBus(fjsp->config->spip);
#endif /* SPI_USE_MUTUAL_EXCLUSION */
#endif /* FLASH_JEDEC_SPI_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the flash device.
 * @pre     In order to use this function the option
 *          @p FLASH_JEDEC_SPI_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 *
 * @api
 */
void fjsReleaseBus(FlashJedecSPIDriver* fjsp)
{
    osalDbgCheck(fjsp != NULL);

#if FLASH_JEDEC_SPI_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
    osalMutexUnlock(&fjsp->mutex);

#if SPI_USE_MUTUAL_EXCLUSION
    /* Release the underlying device as well. */
    spiReleaseBus(fjsp->config->spip);
#endif /* SPI_USE_MUTUAL_EXCLUSION */
#endif /* FLASH_JEDEC_SPI_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Write protects one or more blocks if supported.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 * @param[in] startaddr address within to be protected sector
 * @param[in] n         number of bytes to protect
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool fjsWriteProtect(FlashJedecSPIDriver* fjsp, uint32_t startaddr,
        uint32_t n)
{
    osalDbgCheck(fjsp != NULL);
    /* Verify device status. */
    osalDbgAssert(fjsp->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    osalDbgAssert((startaddr + n <= fjsp->config->sector_size * fjsp->config->sector_num),
            "invalid parameters");

    /* Check if chip supports write protection. */
    if (fjsp->config->bpbits_num == 0)
        return HAL_SUCCESS;

    /* Protect as little of our address space as possible to
     satisfy request. */

    flash_jedec_spi_reconfigure(fjsp);

    flash_jedec_spi_wait_busy(fjsp);

    uint8_t bp_mask = (1 << fjsp->config->bpbits_num) - 1;
    uint8_t bp = (flash_jedec_spi_sr_read(fjsp) >> 2) & bp_mask;

    uint32_t first_protected_addr =
            flash_jedec_spi_bp_to_address(fjsp, bp);

    if (first_protected_addr <= startaddr)
        return HAL_SUCCESS;

    while (bp < bp_mask)
    {
        ++bp;
        first_protected_addr = flash_jedec_spi_bp_to_address(fjsp, bp);
        if (first_protected_addr <= startaddr)
        {
            flash_jedec_spi_sr_write(fjsp, bp << 2);
            return HAL_SUCCESS;
        }
    }

    return HAL_FAILED;
}

/**
 * @brief   Write protects the whole chip.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool fjsMassWriteProtect(FlashJedecSPIDriver* fjsp)
{
    osalDbgCheck(fjsp != NULL);
    /* Verify device status. */
    osalDbgAssert(fjsp->state >= NVM_READY, "invalid state");

    /* Check if chip supports write protection. */
    if (fjsp->config->bpbits_num == 0)
        return HAL_SUCCESS;

    flash_jedec_spi_reconfigure(fjsp);

    /* set BP3 ... BP0 */
    flash_jedec_spi_sr_write(fjsp, 0x07 << 2);

    return HAL_SUCCESS;
}

/**
 * @brief   Write unprotects one or more blocks if supported.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 * @param[in] startaddr address within to be unprotected sector
 * @param[in] n         number of bytes to unprotect
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool fjsWriteUnprotect(FlashJedecSPIDriver* fjsp, uint32_t startaddr,
        uint32_t n)
{
    osalDbgCheck(fjsp != NULL);
    /* Verify device status. */
    osalDbgAssert(fjsp->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    osalDbgAssert((startaddr + n <= fjsp->config->sector_size * fjsp->config->sector_num),
            "invalid parameters");

    /* Check if chip supports write protection. */
    if (fjsp->config->bpbits_num == 0)
        return HAL_SUCCESS;

    /* Unprotect as little of our address space as possible to
     satisfy request. */

    flash_jedec_spi_reconfigure(fjsp);

    flash_jedec_spi_wait_busy(fjsp);

    uint8_t bp_mask = (1 << fjsp->config->bpbits_num) - 1;
    uint8_t bp = (flash_jedec_spi_sr_read(fjsp) >> 2) & bp_mask;

    uint32_t first_protected_addr =
            flash_jedec_spi_bp_to_address(fjsp, bp);

    if (first_protected_addr >= startaddr + n)
        return HAL_SUCCESS;

    while (bp > 0)
    {
        --bp;
        first_protected_addr = flash_jedec_spi_bp_to_address(fjsp, bp);
        if (first_protected_addr >= startaddr + n)
        {
            flash_jedec_spi_sr_write(fjsp, bp << 2);
            return HAL_SUCCESS;
        }
    }

    return HAL_FAILED;
}

/**
 * @brief   Write unprotects the whole chip.
 *
 * @param[in] fjsp      pointer to the @p FlashJedecSPIDriver object
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool fjsMassWriteUnprotect(FlashJedecSPIDriver* fjsp)
{
    osalDbgCheck(fjsp != NULL);
    /* Verify device status. */
    osalDbgAssert(fjsp->state >= NVM_READY, "invalid state");

    /* Check if chip supports write protection. */
    if (fjsp->config->bpbits_num == 0)
        return HAL_SUCCESS;

    flash_jedec_spi_reconfigure(fjsp);

    flash_jedec_spi_sr_write(fjsp, 0x00);

    return HAL_SUCCESS;
}

#endif /* HAL_USE_FLASH_JEDEC_SPI */

/** @} */
