/**
 * @file    qnvm_memory.c
 * @brief   NVM emulation through memory block.
 *
 * @addtogroup NVM_MEMORY
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_NVM_MEMORY || defined(__DOXYGEN__)

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
static const struct NVMMemoryDriverVMT nvm_memory_vmt =
{
    .read = (bool_t (*)(void*, uint32_t, uint32_t, uint8_t*))nvmmemoryRead,
    .write = (bool_t (*)(void*, uint32_t, uint32_t, const uint8_t*))nvmmemoryWrite,
    .erase = (bool_t (*)(void*, uint32_t, uint32_t))nvmmemoryErase,
    .mass_erase = (bool_t (*)(void*))nvmmemoryMassErase,
    .sync = (bool_t (*)(void*))nvmmemorySync,
    .get_info = (bool_t (*)(void*, NVMDeviceInfo*))nvmmemoryGetInfo,
    .acquire = (void (*)(void*))nvmmemoryAcquireBus,
    .release = (void (*)(void*))nvmmemoryReleaseBus,
    .writeprotect = (bool_t (*)(void*, uint32_t, uint32_t))nvmmemoryWriteProtect,
    .mass_writeprotect = (bool_t (*)(void*))nvmmemoryMassWriteProtect,
    .writeunprotect = (bool_t (*)(void*, uint32_t, uint32_t))nvmmemoryWriteUnprotect,
    .mass_writeunprotect = (bool_t (*)(void*))nvmmemoryMassWriteUnprotect,
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   NVM emulation through binary memory initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void nvmmemoryInit(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] nvmmemoryp   pointer to the @p NVMMemoryDriver object
 *
 * @init
 */
void nvmmemoryObjectInit(NVMMemoryDriver* nvmmemoryp)
{
    nvmmemoryp->vmt = &nvm_memory_vmt;
    nvmmemoryp->state = NVM_STOP;
    nvmmemoryp->config = NULL;
#if NVM_MEMORY_USE_MUTUAL_EXCLUSION
#if CH_USE_MUTEXES
    chMtxInit(&nvmmemoryp->mutex);
#else
    chSemInit(&nvmmemoryp->semaphore, 1);
#endif
#endif /* NVM_MEMORY_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Configures and activates the NVM emulation.
 *
 * @param[in] nvmmemoryp    pointer to the @p NVMMemoryDriver object
 * @param[in] config        pointer to the @p NVMMemoryConfig object.
 *
 * @api
 */
void nvmmemoryStart(NVMMemoryDriver* nvmmemoryp, const NVMMemoryConfig* config)
{
    chDbgCheck((nvmmemoryp != NULL) && (config != NULL), "nvmmemoryStart");
    /* Verify device status. */
    chDbgAssert((nvmmemoryp->state == NVM_STOP) || (nvmmemoryp->state == NVM_READY),
            "nvmmemoryStart(), #1", "invalid state");

    nvmmemoryp->config = config;
    nvmmemoryp->state = NVM_READY;
}

/**
 * @brief   Disables the NVM peripheral.
 *
 * @param[in] nvmmemoryp    pointer to the @p NVMMemoryDriver object
 *
 * @api
 */
void nvmmemoryStop(NVMMemoryDriver* nvmmemoryp)
{
    chDbgCheck(nvmmemoryp != NULL, "nvmmemoryStop");
    /* Verify device status. */
    chDbgAssert((nvmmemoryp->state == NVM_STOP) || (nvmmemoryp->state == NVM_READY),
            "nvmmemoryStop(), #1", "invalid state");

    nvmmemoryp->state = NVM_STOP;
}

/**
 * @brief   Reads data crossing sector boundaries if required.
 *
 * @param[in] nvmmemoryp    pointer to the @p NVMMemoryDriver object
 * @param[in] startaddr     address to start reading from
 * @param[in] n             number of bytes to read
 * @param[in] buffer        pointer to data buffer
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmemoryRead(NVMMemoryDriver* nvmmemoryp, uint32_t startaddr, uint32_t n,
        uint8_t* buffer)
{
    chDbgCheck(nvmmemoryp != NULL, "nvmmemoryRead");
    /* Verify device status. */
    chDbgAssert(nvmmemoryp->state >= NVM_READY, "nvmmemoryRead(), #1",
            "invalid state");
    /* Verify range is within chip size. */
    chDbgAssert((startaddr + n <= nvmmemoryp->config->sector_size * nvmmemoryp->config->sector_num),
            "nvmmemoryRead(), #2", "invalid parameters");

    if (nvmmemorySync(nvmmemoryp) != CH_SUCCESS)
        return CH_FAILED;

    /* Read operation in progress. */
    nvmmemoryp->state = NVM_READING;

    memcpy(buffer, nvmmemoryp->config->memoryp + startaddr, n);

    /* Read operation finished. */
    nvmmemoryp->state = NVM_READY;

    return CH_SUCCESS;
}

/**
 * @brief   Writes data crossing sector boundaries if required.
 *
 * @param[in] nvmmemoryp    pointer to the @p NVMMemoryDriver object
 * @param[in] startaddr     address to start writing to
 * @param[in] n             number of bytes to write
 * @param[in] buffer        pointer to data buffer
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmemoryWrite(NVMMemoryDriver* nvmmemoryp, uint32_t startaddr, uint32_t n,
        const uint8_t* buffer)
{
    chDbgCheck(nvmmemoryp != NULL, "nvmmemoryWrite");
    /* Verify device status. */
    chDbgAssert(nvmmemoryp->state >= NVM_READY, "nvmmemoryWrite(), #1",
            "invalid state");
    /* Verify range is within chip size. */
    chDbgAssert((startaddr + n <= nvmmemoryp->config->sector_size * nvmmemoryp->config->sector_num),
            "nvmmemoryWrite(), #2", "invalid parameters");

    if (nvmmemorySync(nvmmemoryp) != CH_SUCCESS)
        return CH_FAILED;

    /* Write operation in progress. */
    nvmmemoryp->state = NVM_WRITING;

    memcpy(nvmmemoryp->config->memoryp + startaddr, buffer, n);

    /* Write operation finished. */
    nvmmemoryp->state = NVM_READY;

    return CH_SUCCESS;
}

/**
 * @brief   Erases one or more sectors.
 *
 * @param[in] nvmmemoryp    pointer to the @p NVMMemoryDriver object
 * @param[in] startaddr     address within to be erased sector
 * @param[in] n             number of bytes to erase
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmemoryErase(NVMMemoryDriver* nvmmemoryp, uint32_t startaddr, uint32_t n)
{
    chDbgCheck(nvmmemoryp != NULL, "nvmmemoryErase");
    /* Verify device status. */
    chDbgAssert(nvmmemoryp->state >= NVM_READY, "nvmmemoryErase(), #1",
            "invalid state");
    /* Verify range is within chip size. */
    chDbgAssert((startaddr + n <= nvmmemoryp->config->sector_size * nvmmemoryp->config->sector_num),
            "nvmmemoryErase(), #2", "invalid parameters");

    if (nvmmemorySync(nvmmemoryp) != CH_SUCCESS)
        return CH_FAILED;

    /* Erase operation in progress. */
    nvmmemoryp->state = NVM_ERASING;

    memset(nvmmemoryp->config->memoryp + startaddr, 0xff, n);

    /* Erase operation finished. */
    nvmmemoryp->state = NVM_READY;

    return CH_SUCCESS;
}

/**
 * @brief   Erases whole chip.
 *
 * @param[in] nvmmemoryp    pointer to the @p NVMMemoryDriver object
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmemoryMassErase(NVMMemoryDriver* nvmmemoryp)
{
    chDbgCheck(nvmmemoryp != NULL, "nvmmemoryMassErase");
    /* Verify device status. */
    chDbgAssert(nvmmemoryp->state >= NVM_READY, "nvmmemoryMassErase(), #1",
            "invalid state");

    if (nvmmemorySync(nvmmemoryp) != CH_SUCCESS)
        return CH_FAILED;

    /* Erase operation in progress. */
    nvmmemoryp->state = NVM_ERASING;

    memset(nvmmemoryp->config->memoryp, 0xff,
            nvmmemoryp->config->sector_size * nvmmemoryp->config->sector_num);

    /* Erase operation finished. */
    nvmmemoryp->state = NVM_READY;

    return CH_SUCCESS;
}

/**
 * @brief   Waits for idle condition.
 *
 * @param[in] nvmmemoryp    pointer to the @p NVMMemoryDriver object
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmemorySync(NVMMemoryDriver* nvmmemoryp)
{
    chDbgCheck(nvmmemoryp != NULL, "nvmmemorySync");
    /* Verify device status. */
    chDbgAssert(nvmmemoryp->state >= NVM_READY, "nvmmemorySync(), #1",
            "invalid state");

    if (nvmmemoryp->state == NVM_READY)
        return CH_SUCCESS;

    /* No more operation in progress. */
    nvmmemoryp->state = NVM_READY;

    return CH_SUCCESS;
}

/**
 * @brief   Returns media info.
 *
 * @param[in] nvmmemoryp    pointer to the @p NVMMemoryDriver object
 * @param[out] nvmdip       pointer to a @p NVMDeviceInfo structure
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmemoryGetInfo(NVMMemoryDriver* nvmmemoryp, NVMDeviceInfo* nvmdip)
{
    chDbgCheck(nvmmemoryp != NULL, "nvmmemoryGetInfo");
    /* Verify device status. */
    chDbgAssert(nvmmemoryp->state >= NVM_READY, "nvmmemoryGetInfo(), #1",
            "invalid state");

    nvmdip->sector_num = nvmmemoryp->config->sector_num;
    nvmdip->sector_size = nvmmemoryp->config->sector_size;
    nvmdip->identification[0] = 'M';
    nvmdip->identification[1] = 'E';
    nvmdip->identification[2] = 'M';

    return CH_SUCCESS;
}

/**
 * @brief   Gains exclusive access to the nvm device.
 * @details This function tries to gain ownership to the nvm device, if the
 *          device is already being used then the invoking thread is queued.
 * @pre     In order to use this function the option
 *          @p NVM_MEMORY_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] nvmmemoryp    pointer to the @p NVMMemoryDriver object
 *
 * @api
 */
void nvmmemoryAcquireBus(NVMMemoryDriver* nvmmemoryp)
{
    chDbgCheck(nvmmemoryp != NULL, "nvmmemoryAcquireBus");

#if NVM_MEMORY_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    chMtxLock(&nvmmemoryp->mutex);
#elif CH_USE_SEMAPHORES
    chSemWait(&nvmmemoryp->semaphore);
#endif
#endif /* NVM_MEMORY_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the nvm device.
 * @pre     In order to use this function the option
 *          @p NVM_MEMORY_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] nvmmemoryp    pointer to the @p NVMMemoryDriver object
 *
 * @api
 */
void nvmmemoryReleaseBus(NVMMemoryDriver* nvmmemoryp)
{
    chDbgCheck(nvmmemoryp != NULL, "nvmmemoryReleaseBus");

#if NVM_MEMORY_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    (void)nvmmemoryp;
    chMtxUnlock();
#elif CH_USE_SEMAPHORES
    chSemSignal(&nvmmemoryp->semaphore);
#endif
#endif /* NVM_MEMORY_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Write protects one or more sectors.
 *
 * @param[in] nvmmemoryp    pointer to the @p NVMMemoryDriver object
 * @param[in] startaddr     address within to be protected sector
 * @param[in] n             number of bytes to protect
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmemoryWriteProtect(NVMMemoryDriver* nvmmemoryp,
        uint32_t startaddr, uint32_t n)
{
    chDbgCheck(nvmmemoryp != NULL, "nvmmemoryWriteProtect");
    /* Verify device status. */
    chDbgAssert(nvmmemoryp->state >= NVM_READY, "nvmmemoryWriteProtect(), #1",
            "invalid state");

    /* TODO: add implementation */

    return CH_SUCCESS;
}

/**
 * @brief   Write protects the whole device.
 *
 * @param[in] nvmmemoryp    pointer to the @p NVMMemoryDriver object
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmemoryMassWriteProtect(NVMMemoryDriver* nvmmemoryp)
{
    chDbgCheck(nvmmemoryp != NULL, "nvmmemoryMassWriteProtect");
    /* Verify device status. */
    chDbgAssert(nvmmemoryp->state >= NVM_READY, "nvmmemoryMassWriteProtect(), #1",
            "invalid state");

    /* TODO: add implementation */

    return CH_SUCCESS;
}

/**
 * @brief   Write unprotects one or more sectors.
 *
 * @param[in] nvmmemoryp    pointer to the @p NVMMemoryDriver object
 * @param[in] startaddr     address within to be unprotected sector
 * @param[in] n             number of bytes to unprotect
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmemoryWriteUnprotect(NVMMemoryDriver* nvmmemoryp,
        uint32_t startaddr, uint32_t n)
{
    chDbgCheck(nvmmemoryp != NULL, "nvmmemoryWriteUnprotect");
    /* Verify device status. */
    chDbgAssert(nvmmemoryp->state >= NVM_READY, "nvmmemoryWriteUnprotect(), #1",
            "invalid state");

    /* TODO: add implementation */

    return CH_SUCCESS;
}

/**
 * @brief   Write unprotects the whole device.
 *
 * @param[in] nvmmemoryp    pointer to the @p NVMMemoryDriver object
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmemoryMassWriteUnprotect(NVMMemoryDriver* nvmmemoryp)
{
    chDbgCheck(nvmmemoryp != NULL, "nvmmemoryMassWriteUnprotect");
    /* Verify device status. */
    chDbgAssert(nvmmemoryp->state >= NVM_READY, "nvmmemoryMassWriteUnprotect(), #1",
            "invalid state");

    /* TODO: add implementation */

    return CH_SUCCESS;
}

#endif /* HAL_USE_NVM_MEMORY */

/** @} */
