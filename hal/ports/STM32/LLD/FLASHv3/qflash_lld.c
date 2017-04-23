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
 * @file    STM32/FLASHv3/qflash.c
 * @brief   STM32 low level FLASH driver code.
 *
 * @addtogroup FLASH
 * @{
 */

#include "qhal.h"

#if HAL_USE_FLASH || defined(__DOXYGEN__)

#include <string.h>

/**
 * @todo    - add error propagation
 *          - replace sync polling by synchronization with isr
 *          - add support for second bank
 */

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @brief   Flash size register address
 */
#if defined(STM32L471xx) ||                 \
        defined(STM32L475xx) ||             \
        defined(STM32L476xx) ||             \
        defined(STM32L485xx) ||             \
        defined(STM32L486xx)
#define FLASH_SIZE_REGISTER_ADDRESS ((uint32_t)0x1fff75e0)
#endif

/**
 * @brief   Flash sector size
 */
#if defined(STM32L471xx) ||                 \
        defined(STM32L475xx) ||             \
        defined(STM32L476xx) ||             \
        defined(STM32L485xx) ||             \
        defined(STM32L486xx)
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
    flashp->flash->CR |= FLASH_CR_OPTLOCK;
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
    static const uint32_t FLASH_OPT_UNLOCK_KEY1 = 0x08192a3b;
    static const uint32_t FLASH_OPT_UNLOCK_KEY2 = 0x4c5d6e7f;

    if ((flashp->flash->CR & FLASH_CR_OPTLOCK) != 0)
    {
        flashp->flash->OPTKEYR = FLASH_OPT_UNLOCK_KEY1;
        flashp->flash->OPTKEYR = FLASH_OPT_UNLOCK_KEY2;
    }
}

/**
 * @brief   Programs a 64bit quadword of data to flash memory
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] addr      absolute address
 * @param[in] data      data to program
 *
 * @notapi
 */
static void flash_lld_program_64(FLASHDriver* flashp, uint32_t addr,
        uint64_t data)
{
    flash_lld_sync(flashp);

    flash_lld_cr_unlock(flashp);

    /* Set operation to perform. */
    flashp->flash->CR &= ~(FLASH_CR_FSTPG | FLASH_CR_MER2 |
            FLASH_CR_PNB | FLASH_CR_MER1 | FLASH_CR_PER | FLASH_CR_PG);
    flashp->flash->CR |= FLASH_CR_PG;

    *(__IO uint32_t*)addr = (uint32_t)data;
    *(__IO uint32_t*)(addr + 4) = (uint32_t)(data >> 32);

    flash_lld_cr_lock(flashp);
}

#if 0
/**
 * @brief   Programs a 32bit word of data to option flash memory
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] addr      absolute address
 * @param[in] data      data to program
 *
 * @notapi
 */
static void flash_lld_ob_program_32(FLASHDriver* flashp, uint32_t addr,
        uint32_t data)
{
    flash_lld_sync(flashp);

    flash_lld_cr_unlock(flashp);
    flash_lld_optcr_unlock(flashp);
#if 0
    /* Set operation to perform. */
    flashp->flash->CR &= ~(FLASH_CR_OPTER | FLASH_CR_OPTPG |
            FLASH_CR_MER | FLASH_CR_PER | FLASH_CR_PG);
    flashp->flash->CR |= FLASH_CR_OPTPG;
#endif

    *(__O uint32_t*)addr = data;

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
            flash_lld_ob_program_32(flashp, (uint32_t)dst, *src & 0xff);
        ++src;
        ++dst;
    }
}
#endif

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
#if 0
    flashp->flash->CR &= ~(FLASH_CR_OPTER | FLASH_CR_OPTPG |
            FLASH_CR_MER | FLASH_CR_PER | FLASH_CR_PG);
    flashp->flash->CR |= FLASH_CR_OPTER;
#endif

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
    return 0;
#if 0
    return ((OB->WRP0 & 0xff) << 0) |
            ((OB->WRP1 & 0xff) << 8) |
            ((OB->WRP2 & 0xff) << 16) |
            ((OB->WRP3 & 0xff) << 24);
#endif
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
#if 0
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
#endif
}

/**
 * @brief   FLASH common service routine.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 */
static void serve_flash_irq(FLASHDriver* flashp)
{
    uint32_t sr;
    FLASH_TypeDef* fp = flashp->flash;

    sr = fp->SR;
    fp->SR = FLASH_SR_OPTVERR | FLASH_SR_RDERR | FLASH_SR_FASTERR |
            FLASH_SR_MISERR | FLASH_SR_PGSERR | FLASH_SR_SIZERR |
            FLASH_SR_PGAERR | FLASH_SR_WRPERR | FLASH_SR_PROGERR |
            FLASH_SR_OPERR | FLASH_SR_EOP;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   FLASH IRQ handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_FLASH_HANDLER)
{
    OSAL_IRQ_PROLOGUE();

    serve_flash_irq(&FLASHD);

    OSAL_IRQ_EPILOGUE();
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
            /* Enable interrupt handler. */
            nvicEnableVector(STM32_FLASH_NUMBER, STM32_FLASH_IRQ_PRIORITY);

            /* Clear all possibly pending flags. */
            flashp->flash->SR = FLASH_SR_OPTVERR | FLASH_SR_RDERR |
                    FLASH_SR_FASTERR | FLASH_SR_MISERR | FLASH_SR_PGSERR |
                    FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_WRPERR |
                    FLASH_SR_PROGERR | FLASH_SR_OPERR | FLASH_SR_EOP;

            /* Enable interrupt sources. */
            flash_lld_cr_unlock(flashp);
            flashp->flash->CR = FLASH_CR_RDERRIE | FLASH_CR_ERRIE |
                    FLASH_CR_EOPIE;
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
            /* Disable interrupt sources. */
            flash_lld_cr_unlock(flashp);
            flashp->flash->CR = 0;
            flash_lld_cr_lock(flashp);

            /* Clear all possibly pending flags. */
            flashp->flash->SR = FLASH_SR_OPTVERR | FLASH_SR_RDERR |
                    FLASH_SR_FASTERR | FLASH_SR_MISERR | FLASH_SR_PGSERR |
                    FLASH_SR_SIZERR | FLASH_SR_PGAERR | FLASH_SR_WRPERR |
                    FLASH_SR_PROGERR | FLASH_SR_OPERR | FLASH_SR_EOP;

            /* Disable interrupt handler. */
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
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @notapi
 */
bool flash_lld_addr_to_sector(uint32_t addr, FLASHSectorInfo* sinfo)
{
    /* Test against total flash size. */
    if (addr >=
            (uint32_t)(*((__I uint16_t*)FLASH_SIZE_REGISTER_ADDRESS)) * 1024)
        return HAL_FAILED;

    if (sinfo != NULL)
    {
        sinfo->sector = addr / FLASH_SECTOR_SIZE;
        sinfo->origin = sinfo->sector * FLASH_SECTOR_SIZE;
        sinfo->size = FLASH_SECTOR_SIZE;
        sinfo->wrp_bit = 0xff;
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
    /* Verify that we are writing properly bounded address and size. */
    osalDbgAssert((startaddr % 8) == 0 && (n % 8) == 0, "invalid parameters");

    uint32_t addr = FLASH_BASE + startaddr;
    uint32_t offset = 0;
    while (offset < n)
    {
        flash_lld_program_64(flashp, addr, *(uint64_t*)(buffer + offset));
        addr += 8;
        offset += 8;
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
    flashp->flash->CR &= ~(FLASH_CR_FSTPG | FLASH_CR_MER2 |
            FLASH_CR_PNB | FLASH_CR_MER1 | FLASH_CR_PER | FLASH_CR_PG);
    flashp->flash->CR |= ((startaddr / FLASH_SECTOR_SIZE) << 3) | FLASH_CR_PER;

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
    flashp->flash->CR &= ~(FLASH_CR_FSTPG | FLASH_CR_MER2 |
            FLASH_CR_MER1 | FLASH_CR_PER | FLASH_CR_PG);
    flashp->flash->CR |= FLASH_CR_MER1 | FLASH_CR_MER2;

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
        osalSysUnlock();
        osalThreadSleep(1);
        osalSysLock();
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
    nvmdip->identification[2] = '3';
    /* Note: This chip must be written in chunks of 64bits.
     * Also the chip can not write a word which is not 0xffffffffffffffff so
     * we have no way to hide this drawback from the user.*/
    nvmdip->write_alignment = 8;
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
#if 0
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
#endif
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
void flash_lld_ob_user(FLASHDriver* flashp, ob_user_e user)
{
#if 0
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
#endif
}

#endif /* HAL_USE_FLASH */

/** @} */
