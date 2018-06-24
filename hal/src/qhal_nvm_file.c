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
 * @file    qnvm_file.c
 * @brief   NVM emulation through binary file code.
 *
 * @addtogroup NVM_FILE
 * @{
 */

#include "qhal.h"

#if HAL_USE_NVM_FILE || defined(__DOXYGEN__)

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
    (size_t)0,
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
    osalMutexObjectInit(&nvmfilep->mutex);
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
    osalDbgCheck((nvmfilep != NULL) && (config != NULL));
    /* Verify device status. */
    osalDbgAssert((nvmfilep->state == NVM_STOP) || (nvmfilep->state == NVM_READY),
            "invalid state");

    if (nvmfilep->state == NVM_READY)
        nvmfileStop(nvmfilep);

    nvmfilep->config = config;

#if HAS_FATFS
    FRESULT result;
    result = f_open(&nvmfilep->file, nvmfilep->config->file_name,
            FA_READ | FA_WRITE | FA_OPEN_EXISTING);

    if (result != FR_OK)
    {
        result = f_open(&nvmfilep->file, nvmfilep->config->file_name,
                FA_READ | FA_WRITE | FA_CREATE_NEW);

        if (result != FR_OK)
            return;
    }

    size_t current_size = f_size(&nvmfilep->file);
    size_t desired_size =
            nvmfilep->config->sector_size * nvmfilep->config->sector_num;

    result = f_lseek(&nvmfilep->file, current_size);

    if (result != FR_OK)
        return;

    if (current_size < desired_size)
    {
        static const uint8_t erased = 0xff;
        for (size_t i = current_size; i < desired_size; ++i)
            if (f_write(&nvmfilep->file, &erased, 1, NULL) != FR_OK)
                return;
        if (f_sync(&nvmfilep->file) != FR_OK)
            return;
    }
#else /* HAS_FATFS */
    nvmfilep->file = fopen(nvmfilep->config->file_name, "r+b");

    if (nvmfilep->file == NULL)
    {
        /* Create file as it seems to not exist */
        nvmfilep->file = fopen(nvmfilep->config->file_name, "w+b");
    }

    osalDbgAssert(nvmfilep->file != NULL, "invalid file pointer");

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
#endif /* HAS_FATFS */

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
    osalDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    osalDbgAssert((nvmfilep->state == NVM_STOP) || (nvmfilep->state == NVM_READY),
            "invalid state");

#if HAS_FATFS
    f_close(&nvmfilep->file);
#else /* HAS_FATFS */
    if (nvmfilep->file != NULL)
        fclose(nvmfilep->file);

    nvmfilep->file = NULL;
#endif /* HAS_FATFS */

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
    osalDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    osalDbgAssert((startaddr + n <= nvmfilep->config->sector_size * nvmfilep->config->sector_num),
            "invalid parameters");

    if (nvmfileSync(nvmfilep) != HAL_SUCCESS)
        return HAL_FAILED;

    /* Read operation in progress. */
    nvmfilep->state = NVM_READING;

#if HAS_FATFS
    if (f_lseek(&nvmfilep->file, startaddr) != FR_OK)
        return HAL_FAILED;

    if (f_read(&nvmfilep->file, buffer, n, NULL) != FR_OK)
        return HAL_FAILED;
#else /* HAS_FATFS */
    if (fseek(nvmfilep->file, startaddr, SEEK_SET) != 0)
        return HAL_FAILED;

    if (fread(buffer, 1, n, nvmfilep->file) != n)
        return HAL_FAILED;
#endif /* HAS_FATFS */

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
    osalDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    osalDbgAssert((startaddr + n <= nvmfilep->config->sector_size * nvmfilep->config->sector_num),
            "invalid parameters");

    if (nvmfileSync(nvmfilep) != HAL_SUCCESS)
        return HAL_FAILED;

    /* Write operation in progress. */
    nvmfilep->state = NVM_WRITING;

#if HAS_FATFS
    if (f_lseek(&nvmfilep->file, startaddr) != FR_OK)
        return HAL_FAILED;

    if (f_write(&nvmfilep->file, buffer, n, NULL) != FR_OK)
        return HAL_FAILED;
#else /* HAS_FATFS */
    if (fseek(nvmfilep->file, startaddr, SEEK_SET) != 0)
        return HAL_FAILED;

    if (fwrite(buffer, 1, n, nvmfilep->file) != n)
        return HAL_FAILED;
#endif /* HAS_FATFS */

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
    osalDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");
    /* Verify range is within chip size. */
    osalDbgAssert((startaddr + n <= nvmfilep->config->sector_size * nvmfilep->config->sector_num),
            "invalid parameters");

    if (nvmfileSync(nvmfilep) != HAL_SUCCESS)
        return HAL_FAILED;

    /* Erase operation in progress. */
    nvmfilep->state = NVM_ERASING;

    uint32_t first_sector_addr =
            startaddr - (startaddr % nvmfilep->config->sector_size);

#if HAS_FATFS
    if (f_lseek(&nvmfilep->file, first_sector_addr) != FR_OK)
        return HAL_FAILED;

    for (uint32_t addr = first_sector_addr;
            addr < startaddr + n;
            addr += nvmfilep->config->sector_size)
    {
        static const uint8_t erased = 0xff;
        for (size_t i = 0; i < nvmfilep->config->sector_size; ++i)
            if (f_write(&nvmfilep->file, &erased, sizeof(erased), NULL) != FR_OK)
                return HAL_FAILED;
    }
#else /* HAS_FATFS */
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
#endif /* HAS_FATFS */

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
    osalDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");

    if (nvmfileSync(nvmfilep) != HAL_SUCCESS)
        return HAL_FAILED;

    /* Erase operation in progress. */
    nvmfilep->state = NVM_ERASING;

#if HAS_FATFS
    for (uint32_t addr = 0;
            addr < nvmfilep->config->sector_size * nvmfilep->config->sector_num;
            addr += nvmfilep->config->sector_size)
    {
        if (f_lseek(&nvmfilep->file, addr) != FR_OK)
            return HAL_FAILED;

        static const uint8_t erased = 0xff;
        for (size_t i = 0; i < nvmfilep->config->sector_size; ++i)
            if (f_write(&nvmfilep->file, &erased, sizeof(erased), NULL) != FR_OK)
                return HAL_FAILED;
    }
#else /* HAS_FATFS */
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
#endif /* HAS_FATFS */

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
    osalDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");

    if (nvmfilep->state == NVM_READY)
        return HAL_SUCCESS;

#if HAS_FATFS
    if (f_sync(&nvmfilep->file) != FR_OK)
        return HAL_FAILED;
#else /* HAS_FATFS */
    if (fflush(nvmfilep->file) != 0)
        return HAL_FAILED;
#endif /* HAS_FATFS */

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
    osalDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");

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
    osalDbgCheck(nvmfilep != NULL);

#if NVM_FILE_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
    osalMutexLock(&nvmfilep->mutex);
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
    osalDbgCheck(nvmfilep != NULL);

#if NVM_FILE_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
    osalMutexUnlock(&nvmfilep->mutex);
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
    osalDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");

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
    osalDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");

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
    osalDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");

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
    osalDbgCheck(nvmfilep != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmfilep->state >= NVM_READY, "invalid state");

    /* TODO: add implementation */

    return HAL_SUCCESS;
}

#endif /* HAL_USE_NVM_FILE */

/** @} */
