/**
 * @file    AT91SAM7/qflash_lld.c
 * @brief   AT91SAM7 low level FLASH driver code.
 *
 * @addtogroup FLASH
 * @{
 */

#include "qhal.h"

#if HAL_USE_FLASH || defined(__DOXYGEN__)

#include <string.h>

/**
 * @todo    - add error propagation
 *          - add support for EFC1
 */

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief FLASH driver identifier.
 */
FLASHDriver FLASHD;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Executes a flash command and waits for its completion.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @notapi
 */
__attribute__((section(".ramfunc")))
static void flash_lld_start_command(FLASHDriver* flashp, uint8_t cmd,
        uint16_t arg)
{
    /*
     * Before writing Non Volatile Memory bits (Lock bits,
     * General Purpose NVM bit and Security bits), this field must be set to
     * the number of Master Clock cycles in one microsecond.
     * When writing the rest of the Flash, this field defines the number of
     * Master Clock cycles in 1.5 microseconds. This number must be rounded up.
     */
    static const uint8_t fmcn_bits = MCK * (1.0d / 1000 / 1000) + 0.999999d;
    static const uint8_t fmcn_flash = MCK * (1.5d / 1000 / 1000) + 0.999999d;

    switch (cmd)
    {
    case AT91C_MC_FCMD_LOCK:
    case AT91C_MC_FCMD_UNLOCK:
    case AT91C_MC_FCMD_SET_GP_NVM:
    case AT91C_MC_FCMD_CLR_GP_NVM:
    case AT91C_MC_FCMD_SET_SECURITY:
        flashp->flash->EFC_FMR =
                (flashp->flash->EFC_FMR & ~AT91C_MC_FMCN) |
                (fmcn_bits << 16);
        break;
    case AT91C_MC_FCMD_START_PROG:
    case AT91C_MC_FCMD_ERASE_ALL:
        flashp->flash->EFC_FMR =
                (flashp->flash->EFC_FMR & ~AT91C_MC_FMCN) |
                (fmcn_flash << 16);
        break;
    }

    /* Start command. */
    flashp->flash->EFC_FCR = (0x5A << 24) | (arg << 8) | cmd;

    /* Wait while efc is busy. */
    while ((flashp->flash->EFC_FSR & AT91C_MC_FRDY) == 0);
}

/**
 * @brief   Reads all lockbits into a uint16_t
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @return              bitmask with current write protection settings
 *
 * @notapi
 */
static uint16_t flash_lld_lockbits_read(FLASHDriver* flashp)
{
    return flashp->flash->EFC_FSR >> 16;
}

/**
 * @brief   Writes all lockbits from a uint16_t
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] newbits   bitmask with desired write protection settings
 *
 * @notapi
 */
static void flash_lld_lockbits_write(FLASHDriver* flashp, uint16_t newbits)
{
    const uint16_t oldbits = flash_lld_lockbits_read(flashp);

    /* Quick-skip if no write is necessary. */
    if (newbits == oldbits)
        return;

    for (uint8_t i = 0; i < 15; ++i)
    {
        uint16_t imask = 1 << i;

        if ((oldbits & imask) != (newbits & imask))
        {
            if (newbits & imask)
            {
                /* Lock */
                flash_lld_start_command(flashp, AT91C_MC_FCMD_LOCK,
                        i * AT91C_IFLASH_LOCK_REGION_SIZE / AT91C_IFLASH_PAGE_SIZE);
            }
            else
            {
                /* Unlock */
                flash_lld_start_command(flashp, AT91C_MC_FCMD_UNLOCK,
                        i * AT91C_IFLASH_LOCK_REGION_SIZE / AT91C_IFLASH_PAGE_SIZE);
            }
        }
    }
}

/**
 * @brief   Reads all gpnvm bits into a uint8_t
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @return              bitmask with current gpnvm settings
 *
 * @notapi
 */
static uint8_t flash_lld_gpnvmbits_read(FLASHDriver* flashp)
{
    return flashp->flash->EFC_FSR >> 8;
}

/**
 * @brief   Writes all gpnvm from a uint8_t
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] newbits   bitmask with desired gpnvm settings
 *
 * @notapi
 */
static void flash_lld_gpnvmbits_write(FLASHDriver* flashp, uint8_t newbits)
{
    const uint8_t oldbits = flash_lld_gpnvmbits_read(flashp);

    /* Quick-skip if no write is necessary. */
    if (newbits == oldbits)
        return;

    for (uint8_t i = 0; i < 3; ++i)
    {
        uint8_t imask = 1 << i;

        if ((oldbits & imask) != (newbits & imask))
        {
            if (newbits & imask)
            {
                /* Set */
                flash_lld_start_command(flashp, AT91C_MC_FCMD_SET_GP_NVM, i);
            }
            else
            {
                /* Clear */
                flash_lld_start_command(flashp, AT91C_MC_FCMD_CLR_GP_NVM, i);
            }
        }
    }
}

/**
 * @brief   Reads status of security bit.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @return              current security bit status
 *
 * @notapi
 */
static bool flash_lld_securitybit_read(FLASHDriver* flashp)
{
    return (flashp->flash->EFC_FSR & AT91C_MC_SECURITY) != 0;
}

/**
 * @brief   Writes security bit.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] newstate  desired state of security bit
 *
 * @notapi
 */
static void flash_lld_securitybit_write(FLASHDriver* flashp, bool newstate)
{
    chDbgAssert(newstate == true, "invalid parameters");

    const bool oldstate = flash_lld_securitybit_read(flashp);

    if (newstate != oldstate)
    {
        if (newstate)
        {
            /* Set */
            flash_lld_start_command(flashp, AT91C_MC_FCMD_SET_SECURITY, 0);
        }
        else
        {
            /* Clear */
            /* @note: Once set, this bit can be reset only by an external
             * hardware ERASE request to the chip. Refer to the product
             * definition section for the pin name that controls the ERASE.
             * In this case, the full memory plane is erased and all lock and
             * general-purpose NVM bits are cleared. The security bit in the
             * MC_FSR is cleared only after these operations.
             */
        }
    }
}

/**
 * @brief   FLASH common service routine.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] mc_fsr    contents of the MC_FSR register on interrupt entry
 *
 * @notapi
 */
void flash_lld_serve_interrupt(FLASHDriver* flashp, uint32_t mc_fsr)
{
    /* @note: The bits in MC_FSR have already been cleared. */

}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level FLASH driver initialization.
 *
 * @notapi
 */
void flash_lld_init(void)
{
    flashObjectInit(&FLASHD);
    FLASHD.flash = AT91C_BASE_EFC;
}

/**
 * @brief   Configures and activates the FLASH peripheral.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @notapi
 */
void flash_lld_start(FLASHDriver* flashp)
{
    if (flashp->state == NVM_STOP)
    {
        if (flashp == &FLASHD)
        {
            /* Clear pending flags. */
            (void)flashp->flash->EFC_FSR;

            /* Disable automatic erase. */
            flashp->flash->EFC_FMR |= AT91C_MC_NEBP;

            return;
        }
    }
}

/**
 * @brief   Deactivates the FLASH peripheral.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @notapi
 */
void flash_lld_stop(FLASHDriver* flashp)
{
    if (flashp->state == NVM_READY)
    {
        if (flashp == &FLASHD)
        {
            /* Clear pending interrupts. */
            (void)flashp->flash->EFC_FSR;

            return;
        }
    }
}

/**
 * @brief   Converts address to sector.
 *
 * @param[in] addr      address to convert to sector
 * @param[out] sinfo    pointer to variable receiving sector info or NULL
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @notapi
 */
bool flash_lld_addr_to_sector(uint32_t addr, FLASHSectorInfo* sinfo)
{
    /* Test against total flash size. */
    if (addr >= AT91C_IFLASH_SIZE)
        return HAL_FAILED;

    if (sinfo != NULL)
    {
        sinfo->sector = addr / AT91C_IFLASH_PAGE_SIZE;
        sinfo->origin = sinfo->sector * AT91C_IFLASH_PAGE_SIZE;
        sinfo->size = AT91C_IFLASH_PAGE_SIZE;
        sinfo->lock_bit =sinfo->origin / AT91C_IFLASH_LOCK_REGION_SIZE;
    }

    return HAL_SUCCESS;
}

/**
 * @brief   Reads data from flash peripheral.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] startaddr relative address to start of flash
 * @param[in] n         number of bytes to read
 * @param[out] buffer   pointer to receiving data buffer
 *
 * @notapi
 */
void flash_lld_read(FLASHDriver* flashp, uint32_t startaddr, uint32_t n,
        uint8_t* buffer)
{
    memcpy(buffer, (uint8_t*)(AT91C_IFLASH + startaddr), n);
}

/**
 * @brief   Writes data to flash peripheral.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] startaddr relative address to start of flash
 * @param[in] n         number of bytes to read
 * @param[in] buffer    pointer to data buffer
 *
 * @notapi
 */
void flash_lld_write(FLASHDriver* flashp, uint32_t startaddr, uint32_t n,
        const uint8_t* buffer)
{
    /* Verify that we are writing an even address and size. */
    chDbgAssert((startaddr % 4) == 0 && (n % 4) == 0, "invalid parameters");

    /* This chip can only write full pages. */
    uint32_t addr = startaddr - (startaddr % AT91C_IFLASH_PAGE_SIZE);

    while (addr < startaddr + n)
    {
        /* Load page buffer. */
        for (size_t i = 0; i < AT91C_IFLASH_PAGE_SIZE; i += 4)
        {
            volatile uint32_t* dst = (volatile uint32_t*)(AT91C_IFLASH + addr + i);
            volatile uint32_t* src;

            if (dst >= (volatile uint32_t*)(AT91C_IFLASH + startaddr) &&
                    dst < (volatile uint32_t*)(AT91C_IFLASH + startaddr + n))
                src = (volatile uint32_t*)(buffer + ((addr + i) - startaddr));
            else
                src = dst;

            *dst = *src;
        }

        /* Perform operation. */
        flash_lld_start_command(flashp, AT91C_MC_FCMD_START_PROG,
                addr / AT91C_IFLASH_PAGE_SIZE);

        /* Increment to next page. */
        addr += AT91C_IFLASH_PAGE_SIZE;
    }
}

/**
 * @brief   Erases one flash sector.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] startaddr relative address to start of flash
 *
 * @notapi
 */
void flash_lld_erase_sector(FLASHDriver* flashp, uint32_t startaddr)
{
    FLASHSectorInfo fsi;
    if (flash_lld_addr_to_sector(startaddr, &fsi) != HAL_SUCCESS)
    {
        chDbgAssert(false, "invalid parameters");
        return;
    }

    /* Load page buffer with 0xffffffff. */
    for (size_t i = 0; i < fsi.size; i += 4)
    {
        volatile uint32_t* dst = (volatile uint32_t*)(AT91C_IFLASH + fsi.origin + i);
        *dst = 0xffffffff;
    }

    /* Enable automatic erase. */
    flashp->flash->EFC_FMR &= ~AT91C_MC_NEBP;

    /* Set operation to perform. */
    flash_lld_start_command(flashp, AT91C_MC_FCMD_START_PROG, fsi.sector);

    /* Disable automatic erase. */
    flashp->flash->EFC_FMR |= AT91C_MC_NEBP;
}

/**
 * @brief   Erases all flash sectors.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @notapi
 */
void flash_lld_erase_mass(FLASHDriver* flashp)
{
    /* Execute command. */
    flash_lld_start_command(flashp, AT91C_MC_FCMD_ERASE_ALL, 0);
}

/**
 * @brief   Waits for FLASH peripheral to become idle.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @notapi
 */
void flash_lld_sync(FLASHDriver* flashp)
{
    /* The EFC is kept in ready state all the time. */
}

/**
 * @brief   Returns chip information.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @notapi
 */
void flash_lld_get_info(FLASHDriver* flashp, NVMDeviceInfo* nvmdip)
{
    nvmdip->sector_size = AT91C_IFLASH_PAGE_SIZE;
    nvmdip->sector_num = AT91C_IFLASH_NB_OF_PAGES;
    nvmdip->identification[0] = 'E';
    nvmdip->identification[1] = 'F';
    nvmdip->identification[2] = 'C';
    nvmdip->write_alignment = 4;
}

/**
 * @brief   Write protects one sector.
 * @note    Write protection changes come into effect after reset.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] startaddr relative address to start of flash
 *
 * @notapi
 */
void flash_lld_writeprotect_sector(FLASHDriver* flashp, uint32_t startaddr)
{
    const uint16_t oldbits = flash_lld_lockbits_read(flashp);

    FLASHSectorInfo fsi;
    if (flash_lld_addr_to_sector(startaddr, &fsi) != HAL_SUCCESS)
    {
        chDbgAssert(false, "invalid parameters");
        return;
    }

    flash_lld_lockbits_write(flashp, oldbits | (1 << fsi.lock_bit));
}

/**
 * @brief   Write protects whole device.
 * @note    Write protection changes come into effect after reset.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @notapi
 */
void flash_lld_writeprotect_mass(FLASHDriver* flashp)
{
    flash_lld_lockbits_write(flashp, 0xffff);
}

/**
 * @brief   Write unprotects one sector.
 * @note    Write protection changes come into effect after reset.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] startaddr relative address to start of flash
 *
 * @notapi
 */
void flash_lld_writeunprotect_sector(FLASHDriver* flashp, uint32_t startaddr)
{
    const uint16_t oldbits = flash_lld_lockbits_read(flashp);

    FLASHSectorInfo fsi;
    if (flash_lld_addr_to_sector(startaddr, &fsi) != HAL_SUCCESS)
    {
        chDbgAssert(false, "invalid parameters");
        return;
    }

    flash_lld_lockbits_write(flashp, oldbits & ~(1 << fsi.lock_bit));
}

/**
 * @brief   Write unprotects whole device.
 * @note    Write protection changes come into effect after reset.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @notapi
 */
void flash_lld_writeunprotect_mass(FLASHDriver* flashp)
{
    flash_lld_lockbits_write(flashp, 0x0000);
}

/**
 * @brief   Sets or clears gpnvm bits.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] newbits   desired gpnvm bit status
 *
 * @notapi
 */
void flash_lld_gpnvm(FLASHDriver* flashp, uint8_t newbits)
{
    flash_lld_gpnvmbits_write(flashp, newbits);
}

/**
 * @brief   Sets or clears the security bit.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] newstate  desired security bit state
 *
 * @notapi
 */
void flash_lld_security(FLASHDriver* flashp, bool newstate)
{
    flash_lld_securitybit_write(flashp, newstate);
}

#endif /* HAL_USE_FLASH */

/** @} */
