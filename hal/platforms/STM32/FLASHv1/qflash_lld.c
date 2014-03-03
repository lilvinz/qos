/**
 * @file    STM32/FLASHv1/qflash.c
 * @brief   STM32 low level FLASH driver code.
 *
 * @addtogroup FLASH
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_FLASH || defined(__DOXYGEN__)

#include <string.h>

/**
 * @todo    - add error propagation
 *          - replace sync polling by synchronization with isr
 *          - add support for USER1 and USER2 option bytes
 *          - add support for XL density devices
 */

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @brief   Flash size register address
 */
#if defined(STM32F10X_LD) ||                \
        defined(STM32F10X_LD_VL) ||         \
        defined(STM32F10X_MD) ||            \
        defined(STM32F10X_MD_VL) ||         \
        defined(STM32F10X_HD) ||            \
        defined(STM32F10X_HD_VL) ||         \
        defined(STM32F10X_XL) ||            \
        defined(STM32F10X_CL)
#define FLASH_SIZE_REGISTER_ADDRESS ((uint32_t)0x1ffff7e0)
#endif

/**
 * @brief   Flash sector size
 */
#if defined(STM32F10X_LD) ||                \
        defined(STM32F10X_LD_VL) ||         \
        defined(STM32F10X_MD) ||            \
        defined(STM32F10X_MD_VL)
#define FLASH_SECTOR_SIZE 1024
#elif defined(STM32F10X_HD) ||              \
        defined(STM32F10X_HD_VL) ||         \
        defined(STM32F10X_XL) ||            \
        defined(STM32F10X_CL)
#define FLASH_SECTOR_SIZE 2048
#endif

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
 * @brief   Locks access to FLASH CR peripheral register.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @notapi
 */
static void flash_lld_cr_lock(FLASHDriver* flashp)
{
    flashp->flash->CR |= FLASH_CR_LOCK;
}

/**
 * @brief   Unlocks access to FLASH CR peripheral register.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @notapi
 */
static void flash_lld_cr_unlock(FLASHDriver* flashp)
{
    static const uint32_t FLASH_UNLOCK_KEY1 = 0x45670123;
    static const uint32_t FLASH_UNLOCK_KEY2 = 0xcdef89ab;

    if ((flashp->flash->CR & FLASH_CR_LOCK) != 0)
    {
        flashp->flash->KEYR = FLASH_UNLOCK_KEY1;
        flashp->flash->KEYR = FLASH_UNLOCK_KEY2;
    }
}

/**
 * @brief   Locks access to FLASH OPTCR peripheral register.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @notapi
 */
static void flash_lld_optcr_lock(FLASHDriver* flashp)
{
    flashp->flash->CR &= ~FLASH_CR_OPTWRE;
}

/**
 * @brief   Unlocks access to FLASH OPTCR peripheral register.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @notapi
 */
static void flash_lld_optcr_unlock(FLASHDriver* flashp)
{
    static const uint32_t FLASH_OPT_UNLOCK_KEY1 = 0x45670123;
    static const uint32_t FLASH_OPT_UNLOCK_KEY2 = 0xcdef89ab;

    if ((flashp->flash->CR & FLASH_CR_OPTWRE) == 0)
    {
        flashp->flash->OPTKEYR = FLASH_OPT_UNLOCK_KEY1;
        flashp->flash->OPTKEYR = FLASH_OPT_UNLOCK_KEY2;
    }
}

/**
 * @brief   Programs a 16bit halfword of data to flash memory
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] addr      absolute address
 * @param[in] data      data to program
 *
 * @notapi
 */
static void flash_lld_program_16(FLASHDriver* flashp, uint32_t addr,
        uint16_t data)
{
    flash_lld_sync(flashp);

    flash_lld_cr_unlock(flashp);

    /* Set operation to perform. */
    flashp->flash->CR &= ~(FLASH_CR_OPTER | FLASH_CR_OPTPG |
            FLASH_CR_MER | FLASH_CR_PER | FLASH_CR_PG);
    flashp->flash->CR |= FLASH_CR_PG;

    flash_lld_cr_lock(flashp);

    *(__O uint16_t*)addr = data;
}

/**
 * @brief   Programs a 16bit halfword of data to option flash memory
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] addr      absolute address
 * @param[in] data      data to program
 *
 * @notapi
 */
static void flash_lld_ob_program_16(FLASHDriver* flashp, uint32_t addr,
        uint16_t data)
{
    flash_lld_sync(flashp);

    flash_lld_cr_unlock(flashp);
    flash_lld_optcr_unlock(flashp);

    /* Set operation to perform. */
    flashp->flash->CR &= ~(FLASH_CR_OPTER | FLASH_CR_OPTPG |
            FLASH_CR_MER | FLASH_CR_PER | FLASH_CR_PG);
    flashp->flash->CR |= FLASH_CR_OPTPG;

    *(__O uint16_t*)addr = data;

    flash_lld_optcr_lock(flashp);
    flash_lld_cr_lock(flashp);
}

/**
 * @brief   Reads all option bytes into a struct
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] ob        pointer to the ob structure receiving the bytes
 *
 * @notapi
 */
static void flash_lld_ob_read(FLASHDriver* flashp, OB_TypeDef *ob)
{
    memcpy(ob, OB, sizeof(*ob));
}

/**
 * @brief   Writes all option bytes to flash, masking the high bytes
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] ob        pointer to the ob structure
 *
 * @notapi
 */
static void flash_lld_ob_write(FLASHDriver* flashp, OB_TypeDef *ob)
{
    uint16_t *src = (uint16_t*)ob;
    uint16_t *dst = (uint16_t*)OB;

    for (size_t i = 0; i < sizeof(*OB); i += 2)
    {
        if ((*src & 0xff) != (*dst & 0xff))
            flash_lld_ob_program_16(flashp, (uint32_t)dst, *src & 0xff);
        ++src;
        ++dst;
    }
}

/**
 * @brief   Erases all option bytes.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @notapi
 */
void flash_lld_ob_erase(FLASHDriver* flashp)
{
    flash_lld_sync(flashp);

    flash_lld_cr_unlock(flashp);
    flash_lld_optcr_unlock(flashp);

    /* Set operation to perform. */
    flashp->flash->CR &= ~(FLASH_CR_OPTER | FLASH_CR_OPTPG |
            FLASH_CR_MER | FLASH_CR_PER | FLASH_CR_PG);
    flashp->flash->CR |= FLASH_CR_OPTER;

    /* Start operation. */
    flashp->flash->CR |= FLASH_CR_STRT;

    flash_lld_optcr_lock(flashp);
    flash_lld_cr_lock(flashp);
}

/**
 * @brief   Reads all option byte write protection bits into a uint32_t
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @return              bitmask with current write protection settings
 *
 * @notapi
 */
static uint32_t flash_lld_ob_wrp_read(FLASHDriver* flashp)
{
    return ((OB->WRP0 & 0xff) << 0) |
            ((OB->WRP1 & 0xff) << 8) |
            ((OB->WRP2 & 0xff) << 16) |
            ((OB->WRP3 & 0xff) << 24);
}

/**
 * @brief   Writes all option byte write protection bits from a uint32_t
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] bitmask   bitmask with desired write protection settings
 *
 * @notapi
 */
static void flash_lld_ob_wrp_write(FLASHDriver* flashp, uint32_t bitmask)
{
    /* Skip if no write is necessary. */
    if (flash_lld_ob_wrp_read(flashp) == bitmask)
        return;

    /* Backup option bytes. */
    OB_TypeDef temp;
    flash_lld_ob_read(flashp, &temp);
    /* Perform change. */
    temp.WRP0 = (bitmask >> 0) & 0xff;
    temp.WRP1 = (bitmask >> 8) & 0xff;
    temp.WRP2 = (bitmask >> 16) & 0xff;
    temp.WRP3 = (bitmask >> 24) & 0xff;
    /* Erase option bytes. */
    flash_lld_ob_erase(flashp);
    /* Restore backup with changes. */
    flash_lld_ob_write(flashp, &temp);
}

/**
 * @brief   FLASH common service routine.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 */
static void serve_flash_irq(FLASHDriver* flashp)
{
    uint32_t sr;
    FLASH_TypeDef *f = flashp->flash;

    sr = f->SR;
    f->SR = FLASH_SR_PGERR | FLASH_SR_WRPRTERR | FLASH_SR_EOP;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   FLASH IRQ handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_FLASH_HANDLER)
{
    CH_IRQ_PROLOGUE();

    serve_flash_irq(&FLASHD);

    CH_IRQ_EPILOGUE();
}

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
    FLASHD.flash = FLASH;
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
            nvicEnableVector(STM32_FLASH_NUMBER,
                    CORTEX_PRIORITY_MASK(STM32_FLASH_IRQ_PRIORITY));

            /* Clear all possibly pending flags. */
            flashp->flash->SR = FLASH_SR_PGERR | FLASH_SR_WRPRTERR | FLASH_SR_EOP;

            flash_lld_cr_unlock(flashp);
            flashp->flash->CR = FLASH_CR_EOPIE | FLASH_CR_ERRIE;
            flash_lld_cr_lock(flashp);
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
            flash_lld_cr_unlock(flashp);
            flashp->flash->CR = 0;
            flash_lld_cr_lock(flashp);

            /* Clear all possibly pending flags. */
            flashp->flash->SR = FLASH_SR_PGERR | FLASH_SR_WRPRTERR | FLASH_SR_EOP;

            nvicDisableVector(STM32_FLASH_NUMBER);
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
 * @retval CH_SUCCESS   the operation succeeded.
 * @retval CH_FAILED    the operation failed.
 *
 * @notapi
 */
bool_t flash_lld_addr_to_sector(uint32_t addr, FLASHSectorInfo* sinfo)
{
    /* Test against total flash size. */
    if (addr >=
            (uint32_t)(*((__I uint16_t*)FLASH_SIZE_REGISTER_ADDRESS)) * 1024)
        return CH_FAILED;

    if (sinfo != NULL)
    {
        sinfo->sector = addr / FLASH_SECTOR_SIZE;
        sinfo->origin = sinfo->sector * FLASH_SECTOR_SIZE;
        sinfo->size = FLASH_SECTOR_SIZE;
#if defined(STM32F10X_LD) ||                \
        defined(STM32F10X_LD_VL) ||         \
        defined(STM32F10X_MD) ||             \
        defined(STM32F10X_MD_VL)
        sinfo->wrp_bit = sinfo->sector / 4;
#elif defined(STM32F10X_HD) ||              \
        defined(STM32F10X_HD_VL) ||         \
        defined(STM32F10X_XL) ||            \
        defined(STM32F10X_CL)
        if (sinfo->sector >= 62)
            sinfo->wrp_bit = 31;
        else
            sinfo->wrp_bit = sinfo->sector / 2;
#endif
    }

    return CH_SUCCESS;
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
    memcpy(buffer, (uint8_t*)(FLASH_BASE + startaddr), n);
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
    chDbgAssert(
            (startaddr & 1) == 0 &&
            (n & 1) == 0,
            "flash_lld_write(), #1", "invalid parameters");

    uint32_t addr = FLASH_BASE + startaddr;
    uint32_t offset = 0;
    while (offset < n)
    {
        flash_lld_program_16(flashp, addr, *(uint16_t*)(buffer + offset));
        addr += 2;
        offset += 2;
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
    flash_lld_cr_unlock(flashp);

    /* Set operation to perform. */
    flashp->flash->CR &= ~(FLASH_CR_OPTER | FLASH_CR_OPTPG |
            FLASH_CR_MER | FLASH_CR_PER | FLASH_CR_PG);
    flashp->flash->CR |= FLASH_CR_PER;

    /* Set sector. */
    flashp->flash->AR = startaddr;

    /* Start operation. */
    flashp->flash->CR |= FLASH_CR_STRT;

    flash_lld_cr_lock(flashp);
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
    flash_lld_cr_unlock(flashp);

    /* Set operation to perform. */
    flashp->flash->CR &= ~(FLASH_CR_OPTER | FLASH_CR_OPTPG |
            FLASH_CR_MER | FLASH_CR_PER | FLASH_CR_PG);
    flashp->flash->CR |= FLASH_CR_MER;

    /* Start operation. */
    flashp->flash->CR |= FLASH_CR_STRT;

    flash_lld_cr_lock(flashp);
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
    while (flashp->flash->SR & FLASH_SR_BSY)
    {
#if FLASH_NICE_WAITING
        /* Trying to be nice with the other threads. */
        chSysUnlock();
        chThdSleep(1);
        chSysLock();
#endif
    }
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
    nvmdip->sector_size = FLASH_SECTOR_SIZE;
    nvmdip->sector_num =
            (uint32_t)(*(__I uint16_t*)(FLASH_SIZE_REGISTER_ADDRESS)) *
            1024 / nvmdip->sector_size;
    nvmdip->identification[0] = 'F';
    nvmdip->identification[1] = 'v';
    nvmdip->identification[2] = '1';
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
    FLASHSectorInfo fsi;
    flash_lld_addr_to_sector(startaddr, &fsi);

    uint32_t wrp_bits = flash_lld_ob_wrp_read(flashp);

    wrp_bits &= ~(1 << fsi.wrp_bit);

    flash_lld_ob_wrp_write(flashp, wrp_bits);
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
    flash_lld_ob_wrp_write(flashp, 0xffffffff);
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
    FLASHSectorInfo fsi;
    flash_lld_addr_to_sector(startaddr, &fsi);

    uint32_t wrp_bits = flash_lld_ob_wrp_read(flashp);

    wrp_bits |= 1 << fsi.wrp_bit;

    flash_lld_ob_wrp_write(flashp, wrp_bits);
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
    flash_lld_ob_wrp_write(flashp, 0x00000000);
}

/**
 * @brief   Sets read protection bits in option bytes.
 * @note    Read protection changes come into effect after reset.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] level     desired level of chip readout protection
 *
 * @notapi
 */
void flash_lld_ob_rdp(FLASHDriver* flashp, ob_rdp_level_e level)
{
    const uint16_t new_level = (((~level) << 8) | level);
    /* Check current state first. */
    if (OB->RDP != new_level)
    {
        /* Backup option bytes. */
        OB_TypeDef temp;
        flash_lld_ob_read(flashp, &temp);
        /* Perform change. */
        temp.RDP = new_level;
        /* Erase option bytes. */
        flash_lld_ob_erase(flashp);
        /* Restore backup with changes. */
        flash_lld_ob_write(flashp, &temp);
    }
}

/**
 * @brief   Sets user option bits in option bytes.
 * @note    Option byte changes come into effect after reset.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] user      desired user option bits
 *
 * @notapi
 */
void flash_lld_ob_user(FLASHDriver* flashp, uint8_t user)
{
    /* Set all unused option bits to 1. */
#if defined(STM32F10X_XL)
    user |= 0xf0;
#else
    user |= 0xf8;
#endif
    const uint16_t new_user = (((~user) << 8) | user);
    /* Check current state first. */
    if (OB->USER != new_user)
    {
        /* Backup option bytes. */
        OB_TypeDef temp;
        flash_lld_ob_read(flashp, &temp);
        /* Perform change. */
        temp.USER = new_user;
        /* Erase option bytes. */
        flash_lld_ob_erase(flashp);
        /* Restore backup with changes. */
        flash_lld_ob_write(flashp, &temp);
    }
}

#endif /* HAL_USE_FLASH */

/** @} */
