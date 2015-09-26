/**
 * @file    qnvm_file.c
 * @brief   NVM emulation through binary file code.
 *
 * @addtogroup NVM_FILE
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_NVM_FILE || defined(__DOXYGEN__)

#include "static_assert.h"

#include <string.h>

/*
 * @todo    - add write protection emulation
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
static const struct NVMFileDriverVMT nvm_file_vmt =
{
    .read = (bool (*)(void*, uint32_t, uint32_t, uint8_t*))nvmfileRead,
    .write = (bool (*)(void*, uint32_t, uint32_t, const uint8_t*))nvmfileWrite,
    .erase = (bool (*)(void*, uint32_t, uint32_t))nvmfileErase,
    .mass_erase = (bool (*)(void*))nvmfileMassErase,
    .sync = (bool (*)(void*))nvmfileSync,
    .get_info = (bool (*)(void*, NVMDeviceInfo*))nvmfileGetInfo,
    .acquire = (void (*)(void*))nvmfileAcquireBus,
    .release = (void (*)(void*))nvmfileReleaseBus,
    .writeprotect = (bool (*)(void*, uint32_t, uint32_t))nvmfileWriteProtect,
    .mass_writeprotect = (bool (*)(void*))nvmfileMassWriteProtect,
    .writeunprotect = (bool (*)(void*, uint32_t, uint32_t))nvmfileWriteUnprotect,
    .mass_writeunprotect = (bool (*)(void*))nvmfileMassWriteUnprotect,
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   NVM emulation through binary file initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void nvmfileInit(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] nvmfilep     pointer to the @p NVMFileDriver object
 *
 * @init
 */
void nvmfileObjectInit(NVMFileDriver* nvmfilep)
{
    nvmfilep->vmt = &nvm_file_vmt;
    nvmfilep->state = NVM_STOP;
    nvmfilep->config = NULL;
#if NVM_FILE_USE_MUTUAL_EXCLUSION
#if CH_CFG_USE_MUTEXES
    chMtxInit(&nvmfilep->mutex);
#else
    chSemInit(&nvmfilep->semaphore, 1);
#endif
#endif /* NVM_FILE_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the NVM emulation.
 *
 * @param[in] nvmfilep      pointer to the @p NVMFileDriver object
 * @param[in] config        pointer to the @p NVMFileConfig object.
 *
 * @api
 */
void nvmfileStart(NVMFileDriver* nvmfilep, const NVMFileConfig* config)
{
    chDbgCheck((nvmfilep != NULL) && (config != NULL));
    /* Verify device status. */
    chDbgAssert((nvmfilep->state == NVM_STOP) || (nvmfilep->state == NVM_READY),
            "invalid state");

    if (nvmfilep->state == NVM_READY)
        nvmfileStop(nvmfilep);

    nvmfilep->config = config;

    nvmfilep->file = fopen(nvmfilep->config->file_name, "r+b");

    if (nvmfilep->file == NULL)
    {
        /* Create file as it seems to not exist */
        nvmfilep->file = fopen(nvmfilep->config->file_name, "w+b");
    }

    chDbgAssert(nvmfilep->file != NULL, "invalid file pointer");

    if (nvmfilep->file == NULL)
        return;

    /* grow file according to configuration if necessary */
    if (fseek(nvmfilep->file, 0, SEEK_END) != 0)
        return;

    size_t current_size = ftell(nvmfilep->file);
    size_t desired_size =
            nvmfilep->config->sector_size * nvmfilep->config->sector_num;

    if (current_size < desired_size)
    {
        static const uint8_t erased = 0xff;
        for (size_t i = current_size; i < desired_size; ++i)
            if (fwrite(&erased, 1, sizeof(erased), nvmfilep->file) != sizeof(erased))
                return;
        if (fflush(nvmfilep->file) != 0)
            return;
    }

    nvmfilep->state = NVM_READY;
}

/**
 * @brief   Disables the NVM peripheral.
 *
 * @param[in] nvmfilep      pointer to the @p NVMFileDriver object
 *
 * @api
 */
void nvmfileStop(NVMFileDriver* nvmfilep)
{
    chDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    chDbgAssert((nvmfilep->state == NVM_STOP) || (nvmfilep->state == NVM_READY),
            "invalid state");

    if (nvmfilep->file != NULL)
        fclose(nvmfilep->file);

    nvmfilep->file = NULL;
    nvmfilep->state = NVM_STOP;
}

/**
 * @brief   Reads data crossing sector boundaries if required.
 *
 * @param[in] nvmfilep      pointer to the @p NVMFileDriver object
 * @param[in] startaddr     address to start reading from
 * @param[in] n             number of bytes to read
 * @param[in] buffer        pointer to data buffer
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfileRead(NVMFileDriver* nvmfilep, uint32_t startaddr, uint32_t n,
        uint8_t* buffer)
{
    chDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    chDbgAssert((startaddr + n <= nvmfilep->config->sector_size * nvmfilep->config->sector_num),
            "invalid parameters");

    if (nvmfileSync(nvmfilep) != HAL_SUCCESS)
        return HAL_FAILED;

    /* Read operation in progress. */
    nvmfilep->state = NVM_READING;

    if (fseek(nvmfilep->file, startaddr, SEEK_SET) != 0)
        return HAL_FAILED;

    if (fread(buffer, 1, n, nvmfilep->file) != n)
        return HAL_FAILED;

    /* Read operation finished. */
    nvmfilep->state = NVM_READY;

    return HAL_SUCCESS;
}

/**
 * @brief   Writes data crossing sector boundaries if required.
 *
 * @param[in] nvmfilep      pointer to the @p NVMFileDriver object
 * @param[in] startaddr     address to start writing to
 * @param[in] n             number of bytes to write
 * @param[in] buffer        pointer to data buffer
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfileWrite(NVMFileDriver* nvmfilep, uint32_t startaddr, uint32_t n,
        const uint8_t* buffer)
{
    chDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    chDbgAssert((startaddr + n <= nvmfilep->config->sector_size * nvmfilep->config->sector_num),
            "invalid parameters");

    if (nvmfileSync(nvmfilep) != HAL_SUCCESS)
        return HAL_FAILED;

    /* Write operation in progress. */
    nvmfilep->state = NVM_WRITING;

    if (fseek(nvmfilep->file, startaddr, SEEK_SET) != 0)
        return HAL_FAILED;

    if (fwrite(buffer, 1, n, nvmfilep->file) != n)
        return HAL_FAILED;

    return HAL_SUCCESS;
}

/**
 * @brief   Erases one or more sectors.
 *
 * @param[in] nvmfilep      pointer to the @p NVMFileDriver object
 * @param[in] startaddr     address within to be erased sector
 * @param[in] n             number of bytes to erase
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfileErase(NVMFileDriver* nvmfilep, uint32_t startaddr, uint32_t n)
{
    chDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    chDbgAssert((startaddr + n <= nvmfilep->config->sector_size * nvmfilep->config->sector_num),
            "invalid parameters");

    if (nvmfileSync(nvmfilep) != HAL_SUCCESS)
        return HAL_FAILED;

    /* Erase operation in progress. */
    nvmfilep->state = NVM_ERASING;

    uint32_t first_sector_addr =
            startaddr - (startaddr % nvmfilep->config->sector_size);

    if (fseek(nvmfilep->file, first_sector_addr, SEEK_SET) != 0)
        return HAL_FAILED;

    for (uint32_t addr = first_sector_addr;
            addr < startaddr + n;
            addr += nvmfilep->config->sector_size)
    {
        static const uint8_t erased = 0xff;
        for (size_t i = 0; i < nvmfilep->config->sector_size; ++i)
            if (fwrite(&erased, 1, sizeof(erased), nvmfilep->file) != sizeof(erased))
                return HAL_FAILED;
    }

    return HAL_SUCCESS;
}

/**
 * @brief   Erases whole chip.
 *
 * @param[in] nvmfilep      pointer to the @p NVMFileDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfileMassErase(NVMFileDriver* nvmfilep)
{
    chDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");

    if (nvmfileSync(nvmfilep) != HAL_SUCCESS)
        return HAL_FAILED;

    /* Erase operation in progress. */
    nvmfilep->state = NVM_ERASING;

    for (uint32_t addr = 0;
            addr < nvmfilep->config->sector_size * nvmfilep->config->sector_num;
            addr += nvmfilep->config->sector_size)
    {
        if (fseek(nvmfilep->file, addr, SEEK_SET) != 0)
            return HAL_FAILED;

        static const uint8_t erased = 0xff;
        for (size_t i = 0; i < nvmfilep->config->sector_size; ++i)
            if (fwrite(&erased, 1, sizeof(erased), nvmfilep->file) != sizeof(erased))
                return HAL_FAILED;
    }

    return HAL_SUCCESS;
}

/**
 * @brief   Waits for idle condition.
 *
 * @param[in] nvmfilep      pointer to the @p NVMFileDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfileSync(NVMFileDriver* nvmfilep)
{
    chDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");

    if (nvmfilep->state == NVM_READY)
        return HAL_SUCCESS;

    if (fflush(nvmfilep->file) != 0)
        return HAL_FAILED;

    /* No more operation in progress. */
    nvmfilep->state = NVM_READY;

    return HAL_SUCCESS;
}

/**
 * @brief   Returns media info.
 *
 * @param[in] nvmfilep      pointer to the @p NVMFileDriver object
 * @param[out] nvmdip       pointer to a @p NVMDeviceInfo structure
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfileGetInfo(NVMFileDriver* nvmfilep, NVMDeviceInfo* nvmdip)
{
    chDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");

    nvmdip->sector_num = nvmfilep->config->sector_num;
    nvmdip->sector_size = nvmfilep->config->sector_size;
    nvmdip->identification[0] = 'F';
    nvmdip->identification[1] = 'I';
    nvmdip->identification[2] = 'L';
    /* Note: The virtual address room can be written byte wise */
    nvmdip->write_alignment = 0;

    return HAL_SUCCESS;
}

/**
 * @brief   Gains exclusive access to the nvm device.
 * @details This function tries to gain ownership to the nvm device, if the
 *          device is already being used then the invoking thread is queued.
 * @pre     In order to use this function the option
 *          @p NVM_FILE_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] nvmfilep      pointer to the @p NVMFileDriver object
 *
 * @api
 */
void nvmfileAcquireBus(NVMFileDriver* nvmfilep)
{
    chDbgCheck(nvmfilep != NULL);

#if NVM_FILE_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES
    chMtxLock(&nvmfilep->mutex);
#elif CH_CFG_USE_SEMAPHORES
    chSemWait(&nvmfilep->semaphore);
#endif
#endif /* NVM_FILE_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the nvm device.
 * @pre     In order to use this function the option
 *          @p NVM_FILE_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] nvmfilep      pointer to the @p NVMFileDriver object
 *
 * @api
 */
void nvmfileReleaseBus(NVMFileDriver* nvmfilep)
{
    chDbgCheck(nvmfilep != NULL);

#if NVM_FILE_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES
    (void)nvmfilep;
    chMtxUnlock();
#elif CH_CFG_USE_SEMAPHORES
    chSemSignal(&nvmfilep->semaphore);
#endif
#endif /* NVM_FILE_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Write protects one or more sectors.
 *
 * @param[in] nvmfilep      pointer to the @p NVMFileDriver object
 * @param[in] startaddr     address within to be protected sector
 * @param[in] n             number of bytes to protect
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfileWriteProtect(NVMFileDriver* nvmfilep,
        uint32_t startaddr, uint32_t n)
{
    chDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");

    /* TODO: add implementation */

    return HAL_SUCCESS;
}

/**
 * @brief   Write protects the whole device.
 *
 * @param[in] nvmfilep      pointer to the @p NVMFileDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfileMassWriteProtect(NVMFileDriver* nvmfilep)
{
    chDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");

    /* TODO: add implementation */

    return HAL_SUCCESS;
}

/**
 * @brief   Write unprotects one or more sectors.
 *
 * @param[in] nvmfilep      pointer to the @p NVMFileDriver object
 * @param[in] startaddr     address within to be unprotected sector
 * @param[in] n             number of bytes to unprotect
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfileWriteUnprotect(NVMFileDriver* nvmfilep,
        uint32_t startaddr, uint32_t n)
{
    chDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");

    /* TODO: add implementation */

    return HAL_SUCCESS;
}

/**
 * @brief   Write unprotects the whole device.
 *
 * @param[in] nvmfilep      pointer to the @p NVMFileDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfileMassWriteUnprotect(NVMFileDriver* nvmfilep)
{
    chDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");

    /* TODO: add implementation */

    return HAL_SUCCESS;
}

#endif /* HAL_USE_NVM_FILE */

/** @} */
