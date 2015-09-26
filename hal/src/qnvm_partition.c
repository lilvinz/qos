/**
 * @file    qnvm_partition.c
 * @brief   NVM partition driver code.
 *
 * @addtogroup NVM_PARTITION
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_NVM_PARTITION || defined(__DOXYGEN__)

#include "static_assert.h"

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
static const struct NVMPartitionDriverVMT nvm_partition_vmt =
{
    .read = (bool (*)(void*, uint32_t, uint32_t, uint8_t*))nvmpartRead,
    .write = (bool (*)(void*, uint32_t, uint32_t, const uint8_t*))nvmpartWrite,
    .erase = (bool (*)(void*, uint32_t, uint32_t))nvmpartErase,
    .mass_erase = (bool (*)(void*))nvmpartMassErase,
    .sync = (bool (*)(void*))nvmpartSync,
    .get_info = (bool (*)(void*, NVMDeviceInfo*))nvmpartGetInfo,
    /* End of mandatory functions. */
    .acquire = (void (*)(void*))nvmpartAcquireBus,
    .release = (void (*)(void*))nvmpartReleaseBus,
    .writeprotect = (bool (*)(void*, uint32_t, uint32_t))nvmpartWriteProtect,
    .mass_writeprotect = (bool (*)(void*))nvmpartMassWriteProtect,
    .writeunprotect = (bool (*)(void*, uint32_t, uint32_t))nvmpartWriteUnprotect,
    .mass_writeunprotect = (bool (*)(void*))nvmpartMassWriteUnprotect,
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   NVM partition driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void nvmpartInit(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] nvmpartp   pointer to the @p NVMPartitionDriver object
 *
 * @init
 */
void nvmpartObjectInit(NVMPartitionDriver* nvmpartp)
{
    nvmpartp->vmt = &nvm_partition_vmt;
    nvmpartp->state = NVM_STOP;
    nvmpartp->config = NULL;
#if NVM_PARTITION_USE_MUTUAL_EXCLUSION
#if CH_CFG_USE_MUTEXES
    chMtxInit(&nvmpartp->mutex);
#else
    chSemInit(&nvmpartp->semaphore, 1);
#endif
#endif /* NVM_PARTITION_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the NVM partition.
 *
 * @param[in] nvmpartp      pointer to the @p NVMPartitionDriver object
 * @param[in] config        pointer to the @p NVMPartitionConfig object.
 *
 * @api
 */
void nvmpartStart(NVMPartitionDriver* nvmpartp,
        const NVMPartitionConfig* config)
{
    chDbgCheck((nvmpartp != NULL) && (config != NULL));
    /* Verify device status. */
    chDbgAssert((nvmpartp->state == NVM_STOP) || (nvmpartp->state == NVM_READY),
            "invalid state");

    nvmpartp->config = config;

    /* Calculate and cache often reused values. */
    nvmGetInfo(nvmpartp->config->nvmp, &nvmpartp->llnvmdi);
    nvmpartp->part_org = nvmpartp->llnvmdi.sector_size * nvmpartp->config->sector_offset;
    nvmpartp->part_size = nvmpartp->llnvmdi.sector_size * nvmpartp->config->sector_num;

    nvmpartp->state = NVM_READY;
}

/**
 * @brief   Disables the NVM partition.
 *
 * @param[in] nvmpartp      pointer to the @p NVMPartitionDriver object
 *
 * @api
 */
void nvmpartStop(NVMPartitionDriver* nvmpartp)
{
    chDbgCheck(nvmpartp != NULL);
    /* Verify device status. */
    chDbgAssert((nvmpartp->state == NVM_STOP) || (nvmpartp->state == NVM_READY),
            "invalid state");

    nvmpartp->state = NVM_STOP;
}

/**
 * @brief   Reads data crossing sector boundaries if required.
 *
 * @param[in] nvmpartp      pointer to the @p NVMPartitionDriver object
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
bool nvmpartRead(NVMPartitionDriver* nvmpartp, uint32_t startaddr,
        uint32_t n, uint8_t* buffer)
{
    chDbgCheck(nvmpartp != NULL);
    /* Verify device status. */
    chDbgAssert(nvmpartp->state >= NVM_READY, "invalid state");
    /* Verify range is within partition size. */
    chDbgAssert( (startaddr + n <= nvmpartp->part_size), "invalid parameters");

    /* Read operation in progress. */
    nvmpartp->state = NVM_READING;

    bool result = nvmRead(nvmpartp->config->nvmp,
            nvmpartp->part_org + startaddr,
            n, buffer);
    if (result != HAL_SUCCESS)
        return result;

    /* Read operation finished. */
    nvmpartp->state = NVM_READY;

    return HAL_SUCCESS;
}

/**
 * @brief   Writes data crossing sector boundaries if required.
 *
 * @param[in] nvmpartp      pointer to the @p NVMPartitionDriver object
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
bool nvmpartWrite(NVMPartitionDriver* nvmpartp, uint32_t startaddr,
       uint32_t n, const uint8_t* buffer)
{
    chDbgCheck(nvmpartp != NULL);
    /* Verify device status. */
    chDbgAssert(nvmpartp->state >= NVM_READY, "invalid state");
    /* Verify range is within partition size. */
    chDbgAssert( (startaddr + n <= nvmpartp->part_size), "invalid parameters");

    /* Write operation in progress. */
    nvmpartp->state = NVM_WRITING;

    return nvmWrite(nvmpartp->config->nvmp,
            nvmpartp->part_org + startaddr,
            n, buffer);
}

/**
 * @brief   Erases one or more sectors.
 *
 * @param[in] nvmpartp      pointer to the @p NVMPartitionDriver object
 * @param[in] startaddr     address within to be erased sector
 * @param[in] n             number of bytes to erase
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmpartErase(NVMPartitionDriver* nvmpartp, uint32_t startaddr,
        uint32_t n)
{
    chDbgCheck(nvmpartp != NULL);
    /* Verify device status. */
    chDbgAssert(nvmpartp->state >= NVM_READY, "invalid state");
    /* Verify range is within partition size. */
    chDbgAssert((startaddr + n <= nvmpartp->part_size), "invalid parameters");

    /* Erase operation in progress. */
    nvmpartp->state = NVM_ERASING;

    return nvmErase(nvmpartp->config->nvmp,
            nvmpartp->part_org + startaddr,
            n);
}

/**
 * @brief   Erases all sectors.
 *
 * @param[in] nvmpartp      pointer to the @p NVMPartitionDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmpartMassErase(NVMPartitionDriver* nvmpartp)
{
    chDbgCheck(nvmpartp != NULL);
    /* Verify device status. */
    chDbgAssert(nvmpartp->state >= NVM_READY, "invalid state");

    /* Erase operation in progress. */
    nvmpartp->state = NVM_ERASING;

    return nvmErase(nvmpartp->config->nvmp,
            nvmpartp->part_org,
            nvmpartp->part_size);
}

/**
 * @brief   Waits for idle condition.
 *
 * @param[in] nvmpartp      pointer to the @p NVMPartitionDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmpartSync(NVMPartitionDriver* nvmpartp)
{
    chDbgCheck(nvmpartp != NULL);
    /* Verify device status. */
    chDbgAssert(nvmpartp->state >= NVM_READY, "invalid state");

    if (nvmpartp->state == NVM_READY)
        return HAL_SUCCESS;

    bool result = nvmSync(nvmpartp->config->nvmp);
    if (result != HAL_SUCCESS)
        return result;

    /* No more operation in progress. */
    nvmpartp->state = NVM_READY;

    return HAL_SUCCESS;
}

/**
 * @brief   Returns media info.
 *
 * @param[in] nvmpartp      pointer to the @p NVMPartitionDriver object
 * @param[out] nvmdip       pointer to a @p NVMDeviceInfo structure
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmpartGetInfo(NVMPartitionDriver* nvmpartp, NVMDeviceInfo* nvmdip)
{
    chDbgCheck(nvmpartp != NULL);
    /* Verify device status. */
    chDbgAssert(nvmpartp->state >= NVM_READY, "invalid state");

    nvmdip->sector_num = nvmpartp->config->sector_num;
    nvmdip->sector_size = nvmpartp->llnvmdi.sector_size;
    memcpy(nvmdip->identification, nvmpartp->llnvmdi.identification,
            sizeof(nvmdip->identification));
    nvmdip->write_alignment =
            nvmpartp->llnvmdi.write_alignment;

    return HAL_SUCCESS;
}

/**
 * @brief   Gains exclusive access to the nvm partition device.
 * @details This function tries to gain ownership to the nvm partition device,
 *          if the device is already being used then the invoking thread
 *          is queued.
 * @pre     In order to use this function the option
 *          @p NVM_PARTITION_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] nvmpartp      pointer to the @p NVMPartitionDriver object
 *
 * @api
 */
void nvmpartAcquireBus(NVMPartitionDriver* nvmpartp)
{
    chDbgCheck(nvmpartp != NULL);

#if NVM_PARTITION_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES
    chMtxLock(&nvmpartp->mutex);
#elif CH_CFG_USE_SEMAPHORES
    chSemWait(&nvmpartp->semaphore);
#endif

    /* Lock the underlying device as well. */
    nvmAcquire(nvmpartp->config->nvmp);
#endif /* NVM_PARTITION_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the nvm partition device.
 * @pre     In order to use this function the option
 *          @p NVM_PARTITION_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] nvmpartp      pointer to the @p NVMPartitionDriver object
 *
 * @api
 */
void nvmpartReleaseBus(NVMPartitionDriver* nvmpartp)
{
    chDbgCheck(nvmpartp != NULL);

#if NVM_PARTITION_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES
    chMtxUnlock();
#elif CH_CFG_USE_SEMAPHORES
    chSemSignal(&nvmpartp->semaphore);
#endif

    /* Release the underlying device as well. */
    nvmRelease(nvmpartp->config->nvmp);
#endif /* NVM_PARTITION_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Write protects one or more sectors.
 *
 * @param[in] nvmpartp      pointer to the @p NVMPartitionDriver object
 * @param[in] startaddr     address within to be protected sector
 * @param[in] n             number of bytes to protect
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmpartWriteProtect(NVMPartitionDriver* nvmpartp,
        uint32_t startaddr, uint32_t n)
{
    chDbgCheck(nvmpartp != NULL);
    /* Verify device status. */
    chDbgAssert(nvmpartp->state >= NVM_READY, "invalid state");
    /* Verify range is within partition size. */
    chDbgAssert((startaddr + n <= nvmpartp->part_size), "invalid parameters");

    return nvmWriteProtect(nvmpartp->config->nvmp,
            nvmpartp->part_org + startaddr,
            n);
}

/**
 * @brief   Write protects the whole device.
 *
 * @param[in] nvmpartp      pointer to the @p NVMPartitionDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmpartMassWriteProtect(NVMPartitionDriver* nvmpartp)
{
    chDbgCheck(nvmpartp != NULL);
    /* Verify device status. */
    chDbgAssert(nvmpartp->state >= NVM_READY, "invalid state");

    return nvmWriteProtect(nvmpartp->config->nvmp,
            nvmpartp->part_org,
            nvmpartp->part_size);
}

/**
 * @brief   Write unprotects one or more sectors.
 *
 * @param[in] nvmpartp      pointer to the @p NVMPartitionDriver object
 * @param[in] startaddr     address within to be unprotected sector
 * @param[in] n             number of bytes to unprotect
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmpartWriteUnprotect(NVMPartitionDriver* nvmpartp,
        uint32_t startaddr, uint32_t n)
{
    chDbgCheck(nvmpartp != NULL);
    /* Verify device status. */
    chDbgAssert(nvmpartp->state >= NVM_READY, "invalid state");
    /* Verify range is within partition size. */
    chDbgAssert((startaddr + n <= nvmpartp->part_size), "invalid parameters");

    return nvmWriteUnprotect(nvmpartp->config->nvmp,
            nvmpartp->part_org + startaddr,
            n);
}

/**
 * @brief   Write unprotects the whole device.
 *
 * @param[in] nvmpartp      pointer to the @p NVMPartitionDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmpartMassWriteUnprotect(NVMPartitionDriver* nvmpartp)
{
    chDbgCheck(nvmpartp != NULL);
    /* Verify device status. */
    chDbgAssert(nvmpartp->state >= NVM_READY, "invalid state");

    return nvmWriteUnprotect(nvmpartp->config->nvmp,
            nvmpartp->part_org,
            nvmpartp->part_size);
}

#endif /* HAL_USE_NVM_PARTITION */

/** @} */
