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
 * @file    qflash.c
 * @brief   MCU internal flash driver code.
 *
 * @addtogroup FLASH
 * @{
 */

#include "qhal.h"

#if HAL_USE_FLASH || defined(__DOXYGEN__)

#include "static_assert.h"

/**
 * @todo    - add error propagation
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
static const struct FLASHDriverVMT flash_vmt =
{
    (size_t)0,
    .read = (bool (*)(void*, uint32_t, uint32_t, uint8_t*))flashRead,
    .write = (bool (*)(void*, uint32_t, uint32_t, const uint8_t*))flashWrite,
    .erase = (bool (*)(void*, uint32_t, uint32_t))flashErase,
    .sync = (bool (*)(void*))flashSync,
    .get_info = (bool (*)(void*, NVMDeviceInfo*))flashGetInfo,
    /* End of mandatory functions. */
    .acquire = (void (*)(void*))flashAcquireBus,
    .release = (void (*)(void*))flashReleaseBus,
    .writeprotect = (bool (*)(void*, uint32_t, uint32_t))flashWriteProtect,
    .mass_writeprotect = (bool (*)(void*))flashMassWriteProtect,
    .writeunprotect = (bool (*)(void*, uint32_t, uint32_t))flashWriteUnprotect,
    .mass_writeunprotect = (bool (*)(void*))flashMassWriteUnprotect,
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   FLASH INTERNAL driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void flashInit(void)
{
    flash_lld_init();
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] flashp   pointer to the @p FLASHDriver object
 *
 * @init
 */
void flashObjectInit(FLASHDriver* flashp)
{
    flashp->vmt = &flash_vmt;
    flashp->state = NVM_STOP;
    flashp->config = NULL;
#if FLASH_USE_MUTUAL_EXCLUSION
    chMtxObjectInit(&flashp->mutex);
#endif /* FLASH_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the FLASH peripheral.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] config    pointer to the @p FLASHConfig object.
 *
 * @api
 */
void flashStart(FLASHDriver* flashp, const FLASHConfig* config)
{
    chDbgCheck((flashp != NULL) && (config != NULL));
    /* Verify device status. */
    chDbgAssert((flashp->state == NVM_STOP) || (flashp->state == NVM_READY),
            "invalid state");

    flashp->config = config;

    chSysLock();
    flash_lld_start(flashp);
    chSysUnlock();

    flashp->state = NVM_READY;
}

/**
 * @brief   Disables the FLASH peripheral.
 *
 * @param[in] flashp      pointer to the @p FLASHDriver object
 *
 * @api
 */
void flashStop(FLASHDriver* flashp)
{
    chDbgCheck(flashp != NULL);
    /* Verify device status. */
    chDbgAssert((flashp->state == NVM_STOP) || (flashp->state == NVM_READY),
            "invalid state");

    chSysLock();
    flash_lld_stop(flashp);
    chSysUnlock();

    flashp->state = NVM_STOP;
}

/**
 * @brief   Reads data crossing sector boundaries if required.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
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
bool flashRead(FLASHDriver* flashp, uint32_t startaddr, uint32_t n,
        uint8_t* buffer)
{
    chDbgCheck(flashp != NULL);
    /* Verify device status. */
    chDbgAssert(flashp->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    chDbgAssert(
            flash_lld_addr_to_sector(startaddr, NULL) == HAL_SUCCESS
            && flash_lld_addr_to_sector(startaddr + n - 1, NULL) == HAL_SUCCESS,
            "invalid parameters");

    /* Read operation in progress. */
    flashp->state = NVM_READING;

    chSysLock();
    flash_lld_sync(flashp);
    flash_lld_read(flashp, startaddr, n, buffer);
    chSysUnlock();

    /* Read operation finished. */
    flashp->state = NVM_READY;

    return HAL_SUCCESS;
}

/**
 * @brief   Writes data crossing sector boundaries if required.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
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
bool flashWrite(FLASHDriver* flashp, uint32_t startaddr, uint32_t n,
        const uint8_t* buffer)
{
    chDbgCheck(flashp != NULL);
    /* Verify device status. */
    chDbgAssert(flashp->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    chDbgAssert(
            flash_lld_addr_to_sector(startaddr, NULL) == HAL_SUCCESS
            && flash_lld_addr_to_sector(startaddr + n - 1, NULL) == HAL_SUCCESS,
            "invalid parameters");

    /* Write operation in progress. */
    flashp->state = NVM_WRITING;

    chSysLock();
    flash_lld_sync(flashp);
    flash_lld_write(flashp, startaddr, n, buffer);
    chSysUnlock();

    return HAL_SUCCESS;
}

/**
 * @brief   Erases one or more sectors.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] startaddr address within to be erased sector
 * @param[in] n         number of bytes to erase
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool flashErase(FLASHDriver* flashp, uint32_t startaddr, uint32_t n)
{
    chDbgCheck(flashp != NULL);
    /* Verify device status. */
    chDbgAssert(flashp->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    chDbgAssert(
            flash_lld_addr_to_sector(startaddr, NULL) == HAL_SUCCESS
            && flash_lld_addr_to_sector(startaddr + n - 1, NULL) == HAL_SUCCESS,
            "invalid parameters");

    /* Erase operation in progress. */
    flashp->state = NVM_ERASING;

    FLASHSectorInfo sector;

    for (sector.origin = startaddr;
            sector.origin < startaddr + n;
            sector.origin += sector.size)
    {
        if (flash_lld_addr_to_sector(sector.origin, &sector) != HAL_SUCCESS)
            return HAL_FAILED;

        chSysLock();
        flash_lld_sync(flashp);
        flash_lld_erase_sector(flashp, sector.origin);
        chSysUnlock();
    }

    return HAL_SUCCESS;
}

/**
 * @brief   Erases whole chip.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool flashMassErase(FLASHDriver* flashp)
{
    chDbgCheck(flashp != NULL);
    /* Verify device status. */
    chDbgAssert(flashp->state >= NVM_READY, "invalid state");

    /* Erase operation in progress. */
    flashp->state = NVM_ERASING;

    chSysLock();
    flash_lld_sync(flashp);
    flash_lld_erase_mass(flashp);
    chSysUnlock();

    return HAL_SUCCESS;
}

/**
 * @brief   Waits for idle condition.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool flashSync(FLASHDriver* flashp)
{
    chDbgCheck(flashp != NULL);
    /* Verify device status. */
    chDbgAssert(flashp->state >= NVM_READY, "invalid state");

    if (flashp->state == NVM_READY)
        return HAL_SUCCESS;

    chSysLock();
    flash_lld_sync(flashp);
    chSysUnlock();

    flashp->state = NVM_READY;

    return HAL_SUCCESS;
}

/**
 * @brief   Returns media info.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[out] nvmdip   pointer to a @p NVMDeviceInfo structure
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool flashGetInfo(FLASHDriver* flashp, NVMDeviceInfo* nvmdip)
{
    chDbgCheck(flashp != NULL);
    /* Verify device status. */
    chDbgAssert(flashp->state >= NVM_READY, "invalid state");

    chSysLock();
    flash_lld_get_info(flashp, nvmdip);
    chSysUnlock();

    return HAL_SUCCESS;
}

/**
 * @brief   Gains exclusive access to the flash device.
 * @details This function tries to gain ownership to the flash device, if the
 *          device is already being used then the invoking thread is queued.
 * @pre     In order to use this function the option
 *          @p FLASH_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @api
 */
void flashAcquireBus(FLASHDriver* flashp)
{
    chDbgCheck(flashp != NULL);

#if FLASH_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
    chMtxLock(&flashp->mutex);
#endif /* FLASH_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the flash device.
 * @pre     In order to use this function the option
 *          @p FLASH_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @api
 */
void flashReleaseBus(FLASHDriver* flashp)
{
    chDbgCheck(flashp != NULL);

#if FLASH_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
    chMtxUnlock(&flashp->mutex);
#endif /* FLASH_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Write protects one or more sectors.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] startaddr address within to be protected sector
 * @param[in] n         number of bytes to protect
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool flashWriteProtect(FLASHDriver* flashp, uint32_t startaddr, uint32_t n)
{
    chDbgCheck(flashp != NULL);
    /* Verify device status. */
    chDbgAssert(flashp->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    chDbgAssert(
            flash_lld_addr_to_sector(startaddr, NULL) == HAL_SUCCESS
            && flash_lld_addr_to_sector(startaddr + n - 1, NULL) == HAL_SUCCESS,
            "invalid parameters");

    FLASHSectorInfo sector;

    for (sector.origin = startaddr;
            sector.origin < startaddr + n;
            sector.origin += sector.size)
    {
        if (flash_lld_addr_to_sector(sector.origin, &sector) != HAL_SUCCESS)
            return HAL_FAILED;

        chSysLock();
        flash_lld_sync(flashp);
        flash_lld_writeprotect_sector(flashp, sector.origin);
        chSysUnlock();
    }

    return HAL_SUCCESS;
}

/**
 * @brief   Write protects the whole device.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool flashMassWriteProtect(FLASHDriver* flashp)
{
    chDbgCheck(flashp != NULL);
    /* Verify device status. */
    chDbgAssert(flashp->state >= NVM_READY, "invalid state");

    chSysLock();
    flash_lld_sync(flashp);
    flash_lld_writeprotect_mass(flashp);
    chSysUnlock();

    return HAL_SUCCESS;
}

/**
 * @brief   Write unprotects one or more sectors.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 * @param[in] startaddr address within to be unprotected sector
 * @param[in] n         number of bytes to unprotect
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool flashWriteUnprotect(FLASHDriver* flashp, uint32_t startaddr, uint32_t n)
{
    chDbgCheck(flashp != NULL);
    /* Verify device status. */
    chDbgAssert(flashp->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    chDbgAssert(
            flash_lld_addr_to_sector(startaddr, NULL) == HAL_SUCCESS
            && flash_lld_addr_to_sector(startaddr + n - 1, NULL) == HAL_SUCCESS,
            "invalid parameters");

    FLASHSectorInfo sector;

    for (sector.origin = startaddr;
            sector.origin < startaddr + n;
            sector.origin += sector.size)
    {
        if (flash_lld_addr_to_sector(sector.origin, &sector) != HAL_SUCCESS)
            return HAL_FAILED;

        chSysLock();
        flash_lld_sync(flashp);
        flash_lld_writeunprotect_sector(flashp, sector.origin);
        chSysUnlock();
    }

    return HAL_SUCCESS;
}

/**
 * @brief   Write unprotects the whole device.
 *
 * @param[in] flashp    pointer to the @p FLASHDriver object
 *
 * @return              The operation status.
 * @retval HAL_SUCCESS  the operation succeeded.
 * @retval HAL_FAILED   the operation failed.
 *
 * @api
 */
bool flashMassWriteUnprotect(FLASHDriver* flashp)
{
    chDbgCheck(flashp != NULL);
    /* Verify device status. */
    chDbgAssert(flashp->state >= NVM_READY, "invalid state");

    chSysLock();
    flash_lld_sync(flashp);
    flash_lld_writeunprotect_mass(flashp);
    chSysUnlock();

    return HAL_SUCCESS;
}

#endif /* HAL_USE_FLASH */

/** @} */
