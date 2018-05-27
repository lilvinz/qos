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
 * @file    qnvm_nvmioblock.c
 * @brief   qnvm to ioblock wrapper driver.
 *
 * @addtogroup NVM_IOBLOCK
 * @{
 */

#include "qhal.h"

#if (HAL_USE_NVM_IOBLOCK == TRUE) || defined(__DOXYGEN__)

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
static const struct NVMIOBlockDriverVMT nvm_memory_vmt =
{
    .read = (bool (*)(void*, uint32_t, uint8_t*, uint32_t))nvmioblockRead,
    .write = (bool (*)(void*, uint32_t, const uint8_t*, uint32_t))nvmioblockWrite,
    .sync = (bool (*)(void*))nvmioblockSync,
    .get_info = (bool (*)(void*, BlockDeviceInfo*))nvmioblockGetInfo,
    .is_inserted = (bool (*)(void *))nvmioblockIsInserted,
    .is_protected = (bool (*)(void *))nvmioblockIsProtected,
    .connect = (bool (*)(void *))nvmioblockConnect,
    .disconnect = (bool (*)(void *))nvmioblockDisconnect,
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   NVM emulation through ioblock device initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void nvmioblockInit(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] nvmioblockp   pointer to the @p NVMIOBlockDriver object
 *
 * @init
 */
void nvmioblockObjectInit(NVMIOBlockDriver* nvmioblockp)
{
    nvmioblockp->vmt = &nvm_memory_vmt;
    nvmioblockp->state = BLK_STOP;
    nvmioblockp->config = NULL;
}

/**
 * @brief   Configures and activates the NVM instance.
 *
 * @param[in] nvmioblockp   pointer to the @p NVMIOBlockDriver object
 * @param[in] config        pointer to the @p NVMIOBlockConfig object.
 *
 * @api
 */
void nvmioblockStart(NVMIOBlockDriver* nvmioblockp, const NVMIOBlockConfig* config)
{
    osalDbgCheck((nvmioblockp != NULL) && (config != NULL));
    osalDbgAssert((nvmioblockp->state == BLK_STOP) || (nvmioblockp->state == BLK_READY),
            "invalid state");

    nvmioblockp->config = config;
    nvmioblockp->state = BLK_READY;
}

/**
 * @brief   Disables the NVM instance.
 *
 * @param[in] nvmioblockp    pointer to the @p NVMIOBlockDriver object
 *
 * @api
 */
void nvmioblockStop(NVMIOBlockDriver* nvmioblockp)
{
    osalDbgCheck(nvmioblockp != NULL);
    osalDbgAssert((nvmioblockp->state == BLK_STOP) || (nvmioblockp->state == BLK_READY),
            "invalid state");

    nvmioblockp->state = BLK_STOP;
}

/**
 * @brief   Reads data crossing sector boundaries if required.
 *
 * @param[in] nvmioblockp   pointer to the @p NVMIOBlockDriver object
 * @param[in] startblk      block to start reading from
 * @param[in] buffer        pointer to data buffer
 * @param[in] n             number of blocks to read
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmioblockRead(NVMIOBlockDriver* nvmioblockp, uint32_t startblk,
        uint8_t* buffer, uint32_t n)
{
    osalDbgCheck(nvmioblockp != NULL);
    osalDbgAssert(nvmioblockp->state >= BLK_READY, "invalid state");

    if (nvmioblockSync(nvmioblockp) != HAL_SUCCESS)
        return HAL_FAILED;

    nvmioblockp->state = BLK_READING;

    bool result = nvmRead(nvmioblockp->config->nvmp,
            nvmioblockp->config->block_size * startblk,
            nvmioblockp->config->block_size * n,
            buffer);

    nvmioblockp->state = BLK_READY;

    return result;
}

/**
 * @brief   Writes data crossing sector boundaries if required.
 *
 * @param[in] nvmioblockp   pointer to the @p NVMIOBlockDriver object
 * @param[in] startblk      address to start writing to
 * @param[in] buffer        pointer to data buffer
 * @param[in] n             number of bytes to write
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmioblockWrite(NVMIOBlockDriver* nvmioblockp, uint32_t startblk,
        const uint8_t* buffer, uint32_t n)
{
    osalDbgCheck(nvmioblockp != NULL);
    osalDbgAssert(nvmioblockp->state >= BLK_READY, "invalid state");

    if (nvmioblockSync(nvmioblockp) != HAL_SUCCESS)
        return HAL_FAILED;

    nvmioblockp->state = BLK_WRITING;

    bool result = nvmWrite(nvmioblockp->config->nvmp,
            nvmioblockp->config->block_size * startblk,
            nvmioblockp->config->block_size * n,
            buffer);

    nvmioblockp->state = BLK_READY;

    return result;
}

/**
 * @brief   Waits for idle condition.
 *
 * @param[in] nvmioblockp   pointer to the @p NVMIOBlockDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmioblockSync(NVMIOBlockDriver* nvmioblockp)
{
    osalDbgCheck(nvmioblockp != NULL);
    osalDbgAssert(nvmioblockp->state >= BLK_READY, "invalid state");

    if (nvmioblockp->state == BLK_READY)
        return HAL_SUCCESS;

    bool result = nvmSync(nvmioblockp->config->nvmp);
    if (result != HAL_SUCCESS)
        return result;

    nvmioblockp->state = BLK_READY;

    return result;
}

/**
 * @brief   Returns media info.
 *
 * @param[in] nvmioblockp   pointer to the @p NVMIOBlockDriver object
 * @param[out] nvmdip       pointer to a @p NVMDeviceInfo structure
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmioblockGetInfo(NVMIOBlockDriver* nvmioblockp, BlockDeviceInfo* bdip)
{
    osalDbgCheck(nvmioblockp != NULL);
    osalDbgAssert(nvmioblockp->state >= BLK_READY, "invalid state");

    NVMDeviceInfo nvmdi;

    nvmGetInfo(nvmioblockp->config->nvmp, &nvmdi);

    bdip->blk_size = nvmioblockp->config->block_size;
    bdip->blk_num = nvmdi.sector_size * nvmdi.sector_num
            / nvmioblockp->config->block_size;

    return HAL_SUCCESS;
}

/**
 * @brief   Returns if media is inserted.
 *
 * @param[in] nvmioblockp   pointer to the @p NVMIOBlockDriver object
 *
 * @return                  The media presence status.
 * @retval true             media is inserted.
 * @retval false            media is not inserted.
 *
 * @api
 */
bool nvmioblockIsInserted(NVMIOBlockDriver* nvmioblockp)
{
    (void)nvmioblockp;

    /* Not supported by qio_nvm driver interface. Using a sensible default. */

    return true;
}

/**
 * @brief   Returns if media is write protected.
 *
 * @param[in] nvmioblockp   pointer to the @p NVMIOBlockDriver object
 *
 * @return                  The media write protection status.
 * @retval true             media is write protected.
 * @retval false            media is not write protected.
 *
 * @api
 */
bool nvmioblockIsProtected(NVMIOBlockDriver* nvmioblockp)
{
    (void)nvmioblockp;

    /* Not supported by qio_nvm driver interface. Using a sensible default. */

    return false;
}

/**
 * @brief   Start accessing the media.
 *
 * @param[in] nvmioblockp   pointer to the @p NVMIOBlockDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmioblockConnect(NVMIOBlockDriver* nvmioblockp)
{
    (void)nvmioblockp;
    return HAL_SUCCESS;
}

/**
 * @brief   Stop accessing the media so it can be removed.
 *
 * @param[in] nvmioblockp   pointer to the @p NVMIOBlockDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmioblockDisconnect(NVMIOBlockDriver* nvmioblockp)
{
    (void)nvmioblockp;
    return HAL_SUCCESS;
}

#endif /* HAL_USE_NVM_IOBLOCK */

/** @} */
