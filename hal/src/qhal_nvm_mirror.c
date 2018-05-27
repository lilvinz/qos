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
 * @file    qnvm_mirror.c
 * @brief   NVM mirror driver code.
 *
 * @addtogroup NVM_MIRROR
 * @{
 */

#include "qhal.h"

#if HAL_USE_NVM_MIRROR || defined(__DOXYGEN__)

#include "static_assert.h"
#include "nelems.h"

#include <string.h>

/*
 * @brief   Functional description
 *          Memory partitioning is:
 *          - header (at least 1 sector)
 *          - mirror a
 *          - mirror b (same size as mirror a)
 *          Header is being used to store the current state
 *          where a state entry can be:
 *          - unused
 *          - dirty a
 *          - dirty b
 *          - synced
 *          Mirror a and mirror b must be of the same size.
 *          The flow of operation is:
 *          Startup / Recovery rollback:
 *              - state synced:
 *                  Do nothing.
 *              - state dirty a:
 *                  Copy mirror b to mirror a erasing pages as required.
 *                  Set state to synced.
 *                  Execute sync of lower level driver.
 *              - state dirty b:
 *                  Copy mirror a to mirror b erasing pages as required.
 *                  Set state to synced.
 *                  Execute sync of lower level driver.
 *          Sync:
 *              - state synced:
 *                  Execute sync of lower level driver.
 *              - state dirty a:
 *                  Invalid state!
 *              - state dirty b:
 *                  Invalid state!
 *          Read:
 *              - state synced:
 *              - state dirty a:
 *                  Read data from mirror a.
 *              - state dirty b:
 *                  Invalid state!
 *          Write / Erase:
 *              - state synced:
 *                  State is being set to dirty a.
 *                  Execute sync of lower level driver.
 *                  Write(s) and / or erase(s) are being executed on mirror a.
 *                  State is changed to dirty b.
 *                  Execute sync of lower level driver.
 *                  Write(s) and / or erase(s) are being executed on mirror b.
 *                  State is changed to synced.
 *                  Execute sync of lower level driver.
 *              - state dirty a:
 *                  Invalid state!
 *              - state dirty b:
 *                  Invalid state!
 *
 * @todo    - add write protection pass-through to lower level driver
 *
 */

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*
 * @brief   State values have to be chosen so that they can be represented on
 *          any nvm device including devices with the following
 *          limitations:
 *          - Writes are not allowed to cells which contain low bits.
 *          - Smallest write operation is 16bits
 *
 *          The header section is being filled by an array of state entries.
 *          On startup, the last used entry is taken into account. Further
 *          updates are being written to unused entries until the array has
 *          been filled. At that point the header is being erased and the first
 *          entry is being used.
 *          Also note that the chosen patterns assumes a little endian architecture.
 */
static const uint64_t nvm_mirror_state_mark_table[] =
{
    0xffffffffffffffff,
    0xffffffffffff0000,
    0xffffffff00000000,
    0xffff000000000000,
};
STATIC_ASSERT(NELEMS(nvm_mirror_state_mark_table) == STATE_COUNT);
STATIC_ASSERT(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Virtual methods table.
 */
static const struct NVMMirrorDriverVMT nvm_mirror_vmt =
{
    (size_t)0,
    .read = (bool (*)(void*, uint32_t, uint32_t, uint8_t*))nvmmirrorRead,
    .write = (bool (*)(void*, uint32_t, uint32_t, const uint8_t*))nvmmirrorWrite,
    .erase = (bool (*)(void*, uint32_t, uint32_t))nvmmirrorErase,
    .mass_erase = (bool (*)(void*))nvmmirrorMassErase,
    .sync = (bool (*)(void*))nvmmirrorSync,
    .get_info = (bool (*)(void*, NVMDeviceInfo*))nvmmirrorGetInfo,
    /* End of mandatory functions. */
    .acquire = (void (*)(void*))nvmmirrorAcquireBus,
    .release = (void (*)(void*))nvmmirrorReleaseBus,
    .writeprotect = (bool (*)(void*, uint32_t, uint32_t))nvmmirrorWriteProtect,
    .mass_writeprotect = (bool (*)(void*))nvmmirrorMassWriteProtect,
    .writeunprotect = (bool (*)(void*, uint32_t, uint32_t))nvmmirrorWriteUnprotect,
    .mass_writeunprotect = (bool (*)(void*))nvmmirrorMassWriteUnprotect,
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static bool nvm_mirror_state_init(NVMMirrorDriver* nvmmirrorp)
{
    osalDbgCheck((nvmmirrorp != NULL));

    const uint32_t header_orig = 0;
    const uint32_t header_size = nvmmirrorp->mirror_a_org;

    NVMMirrorState new_state = STATE_INVALID;
    uint32_t new_state_addr = 0;

    uint64_t state_mark;

    for (uint32_t i = header_orig;
            i < header_orig + header_size;
            i += sizeof(state_mark))
    {
        bool result = nvmRead(nvmmirrorp->config->nvmp,
                i,
                sizeof(state_mark),
                (uint8_t*)&state_mark);
        if (result != HAL_SUCCESS)
            return result;

        if (state_mark == nvm_mirror_state_mark_table[STATE_SYNCED])
        {
            new_state = STATE_SYNCED;
            new_state_addr = i;
        }
        else if (state_mark == nvm_mirror_state_mark_table[STATE_DIRTY_A])
        {
            new_state = STATE_DIRTY_A;
            new_state_addr = i;
        }
        else if (state_mark == nvm_mirror_state_mark_table[STATE_DIRTY_B])
        {
            new_state = STATE_DIRTY_B;
            new_state_addr = i;
        }
        else if (state_mark == nvm_mirror_state_mark_table[STATE_INVALID])
        {
            /* skip unused */
        }
        else
        {
            /* invalid, force header reinit */
            new_state = STATE_INVALID;
            new_state_addr = 0;
            return HAL_SUCCESS;
        }
    }

    nvmmirrorp->mirror_state = new_state;
    nvmmirrorp->mirror_state_addr = new_state_addr;

    return HAL_SUCCESS;
}

static bool nvm_mirror_state_update(NVMMirrorDriver* nvmmirrorp,
        NVMMirrorState new_state)
{
    osalDbgCheck((nvmmirrorp != NULL));

    if (new_state == nvmmirrorp->mirror_state)
        return HAL_SUCCESS;

    const uint32_t header_orig = 0;
    const uint32_t header_size = nvmmirrorp->mirror_a_org;

    uint32_t new_state_addr = nvmmirrorp->mirror_state_addr;
    uint64_t new_state_mark = nvm_mirror_state_mark_table[new_state];

    /* Advance state entry pointer if last state was synced */
    if (nvmmirrorp->mirror_state == STATE_SYNCED)
        new_state_addr += sizeof(new_state_mark);

    /* Erase header in case of wrap around or if its invalid. */
    if (new_state_addr >= header_orig + header_size ||
            nvmmirrorp->mirror_state == STATE_INVALID)
    {
        new_state_addr = 0;
        bool result = nvmErase(nvmmirrorp->config->nvmp, header_orig,
                header_size);
        if (result != HAL_SUCCESS)
            return result;
    }

    /* Write updated state entry. */
    {
        bool result = nvmWrite(nvmmirrorp->config->nvmp, new_state_addr,
                sizeof(new_state_mark), (uint8_t*)&new_state_mark);
        if (result != HAL_SUCCESS)
            return result;
    }

    /* Sync lower level driver. */
    {
        bool result = nvmSync(nvmmirrorp->config->nvmp);
        if (result != HAL_SUCCESS)
            return result;
    }

    nvmmirrorp->mirror_state = new_state;
    nvmmirrorp->mirror_state_addr = new_state_addr;

    return HAL_SUCCESS;
}

static bool nvm_mirror_copy(NVMMirrorDriver* nvmmirrorp, uint32_t src_addr,
        uint32_t dst_addr, size_t n)
{
    osalDbgCheck((nvmmirrorp != NULL));

    uint64_t state_mark;

    for (uint32_t offset = 0; offset < n; offset += sizeof(state_mark))
    {
        /* Detect start of a new sector and erase destination accordingly. */
        if ((offset % nvmmirrorp->llnvmdi.sector_size) == 0)
        {
            bool result = nvmErase(nvmmirrorp->config->nvmp,
                    dst_addr + offset, nvmmirrorp->llnvmdi.sector_size);
            if (result != HAL_SUCCESS)
                return result;
        }

        /* Read mark into temporary buffer. */
        {
            bool result = nvmRead(nvmmirrorp->config->nvmp,
                    src_addr + offset, sizeof(state_mark),
                    (uint8_t*)&state_mark);
            if (result != HAL_SUCCESS)
                return result;
        }

        /* Write mark to destination. */
        {
            bool result = nvmWrite(nvmmirrorp->config->nvmp,
                    dst_addr + offset, sizeof(state_mark),
                    (uint8_t*)&state_mark);
            if (result != HAL_SUCCESS)
                return result;
        }
    }

    return HAL_SUCCESS;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   NVM mirror driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void nvmmirrorInit(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] nvmmirrorp   pointer to the @p NVMMirrorDriver object
 *
 * @init
 */
void nvmmirrorObjectInit(NVMMirrorDriver* nvmmirrorp)
{
    nvmmirrorp->vmt = &nvm_mirror_vmt;
    nvmmirrorp->state = NVM_STOP;
    nvmmirrorp->config = NULL;
#if NVM_MIRROR_USE_MUTUAL_EXCLUSION
    osalMutexObjectInit(&nvmmirrorp->mutex);
#endif /* NVM_JEDEC_SPI_USE_MUTUAL_EXCLUSION */
    nvmmirrorp->mirror_state = STATE_INVALID;
    nvmmirrorp->mirror_state_addr = 0;
}

/**
 * @brief   Configures and activates the NVM mirror.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 * @param[in] config        pointer to the @p NVMMirrorConfig object.
 *
 * @api
 */
void nvmmirrorStart(NVMMirrorDriver* nvmmirrorp, const NVMMirrorConfig* config)
{
    osalDbgCheck((nvmmirrorp != NULL) && (config != NULL));
    /* Verify device status. */
    osalDbgAssert((nvmmirrorp->state == NVM_STOP) || (nvmmirrorp->state == NVM_READY),
            "invalid state");

    nvmmirrorp->config = config;

    /* Calculate and cache often reused values. */
    nvmGetInfo(nvmmirrorp->config->nvmp, &nvmmirrorp->llnvmdi);
    nvmmirrorp->mirror_size =
            (nvmmirrorp->llnvmdi.sector_num - nvmmirrorp->config->sector_header_num) /
            2 * nvmmirrorp->llnvmdi.sector_size;
    nvmmirrorp->mirror_a_org =
            nvmmirrorp->llnvmdi.sector_size * nvmmirrorp->config->sector_header_num;
    nvmmirrorp->mirror_b_org =
            nvmmirrorp->mirror_a_org + nvmmirrorp->mirror_size;

    nvm_mirror_state_init(nvmmirrorp);

    {
        switch (nvmmirrorp->mirror_state)
        {
        case STATE_DIRTY_A:
            /* Copy mirror b to mirror a erasing pages as required. */
            if (nvm_mirror_copy(nvmmirrorp, nvmmirrorp->mirror_b_org,
                    nvmmirrorp->mirror_a_org, nvmmirrorp->mirror_size) != HAL_SUCCESS)
                return;
            /* Set state to synced. */
            if (nvm_mirror_state_update(nvmmirrorp, STATE_SYNCED) != HAL_SUCCESS)
                return;
            /* Execute sync of lower level driver. */
            if (nvmSync(nvmmirrorp->config->nvmp) != HAL_SUCCESS)
                return;
            break;
        case STATE_INVALID:
            /* Invalid state (all header invalid) assumes mirror b to be dirty. */
        case STATE_DIRTY_B:
            /* Copy mirror a to mirror b erasing pages as required. */
            if (nvm_mirror_copy(nvmmirrorp, nvmmirrorp->mirror_a_org,
                    nvmmirrorp->mirror_b_org, nvmmirrorp->mirror_size) != HAL_SUCCESS)
                return;
            /* Set state to synced. */
            if (nvm_mirror_state_update(nvmmirrorp, STATE_SYNCED) != HAL_SUCCESS)
                return;
            /* Execute sync of lower level driver. */
            if (nvmSync(nvmmirrorp->config->nvmp) != HAL_SUCCESS)
                return;
            break;
        case STATE_SYNCED:
        default:
            break;
        }
    }

    nvmmirrorp->state = NVM_READY;
}

/**
 * @brief   Disables the NVM mirror.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 *
 * @api
 */
void nvmmirrorStop(NVMMirrorDriver* nvmmirrorp)
{
    osalDbgCheck(nvmmirrorp != NULL);
    /* Verify device status. */
    osalDbgAssert((nvmmirrorp->state == NVM_STOP) || (nvmmirrorp->state == NVM_READY),
            "invalid state");
    /* Verify mirror is in valid sync state. */
    osalDbgAssert(nvmmirrorp->mirror_state == STATE_SYNCED, "invalid mirror state");

    nvmmirrorp->state = NVM_STOP;
}

/**
 * @brief   Reads data crossing sector boundaries if required.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
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
bool nvmmirrorRead(NVMMirrorDriver* nvmmirrorp, uint32_t startaddr,
        uint32_t n, uint8_t* buffer)
{
    osalDbgCheck(nvmmirrorp != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmmirrorp->state >= NVM_READY, "invalid state");
    /* Verify range is within mirror size. */
    osalDbgAssert(startaddr + n <= nvmmirrorp->mirror_size,
            "invalid parameters");
    /* Verify mirror is in valid sync state. */
    osalDbgAssert(nvmmirrorp->mirror_state == STATE_SYNCED, "invalid mirror state");

    /* Read operation in progress. */
    nvmmirrorp->state = NVM_READING;

    bool result = nvmRead(nvmmirrorp->config->nvmp,
            nvmmirrorp->mirror_a_org + startaddr,
            n, buffer);
    if (result != HAL_SUCCESS)
        return result;

    /* Read operation finished. */
    nvmmirrorp->state = NVM_READY;

    return HAL_SUCCESS;
}

/**
 * @brief   Writes data crossing sector boundaries if required.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
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
bool nvmmirrorWrite(NVMMirrorDriver* nvmmirrorp, uint32_t startaddr,
        uint32_t n, const uint8_t* buffer)
{
    osalDbgCheck(nvmmirrorp != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmmirrorp->state >= NVM_READY, "invalid state");
    /* Verify range is within mirror size. */
    osalDbgAssert(startaddr + n <= nvmmirrorp->mirror_size,
            "invalid parameters");
    /* Verify mirror is in valid sync state. */
    osalDbgAssert(nvmmirrorp->mirror_state == STATE_SYNCED, "invalid mirror state");

    /* Write operation in progress. */
    nvmmirrorp->state = NVM_WRITING;

    bool result;
    /* Set state to mirror a dirty before changing contents. */
    result = nvm_mirror_state_update(nvmmirrorp, STATE_DIRTY_A);
    if (result != HAL_SUCCESS)
        return result;

    /* Apply write to mirror a. */
    result = nvmWrite(nvmmirrorp->config->nvmp,
            nvmmirrorp->mirror_a_org + startaddr, n, buffer);
    if (result != HAL_SUCCESS)
        return result;

    /* Advance state to mirror b dirty. */
    result = nvm_mirror_state_update(nvmmirrorp, STATE_DIRTY_B);
    if (result != HAL_SUCCESS)
        return result;

    /* Apply write to mirror b. */
    result = nvmWrite(nvmmirrorp->config->nvmp,
            nvmmirrorp->mirror_b_org + startaddr, n, buffer);
    if (result != HAL_SUCCESS)
        return result;

    /* Advance state to synced. */
    result = nvm_mirror_state_update(nvmmirrorp, STATE_SYNCED);
    if (result != HAL_SUCCESS)
        return result;

    return HAL_SUCCESS;
}

/**
 * @brief   Erases one or more sectors.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 * @param[in] startaddr     address within to be erased sector
 * @param[in] n             number of bytes to erase
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmmirrorErase(NVMMirrorDriver* nvmmirrorp, uint32_t startaddr, uint32_t n)
{
    osalDbgCheck(nvmmirrorp != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmmirrorp->state >= NVM_READY, "invalid state");
    /* Verify range is within mirror size. */
    osalDbgAssert(startaddr + n <= nvmmirrorp->mirror_size,
            "invalid parameters");
    /* Verify mirror is in valid sync state. */
    osalDbgAssert(nvmmirrorp->mirror_state == STATE_SYNCED, "invalid mirror state");

    /* Erase operation in progress. */
    nvmmirrorp->state = NVM_ERASING;

    bool result;
    /* Set state to mirror a dirty before changing contents. */
    result = nvm_mirror_state_update(nvmmirrorp, STATE_DIRTY_A);
    if (result != HAL_SUCCESS)
        return result;

    /* Apply erase to mirror a. */
    result = nvmErase(nvmmirrorp->config->nvmp,
            nvmmirrorp->mirror_a_org + startaddr, n);
    if (result != HAL_SUCCESS)
        return result;

    /* Advance state to mirror b dirty. */
    result = nvm_mirror_state_update(nvmmirrorp, STATE_DIRTY_B);
    if (result != HAL_SUCCESS)
        return result;

    /* Apply erase to mirror b. */
    result = nvmErase(nvmmirrorp->config->nvmp,
            nvmmirrorp->mirror_b_org + startaddr, n);
    if (result != HAL_SUCCESS)
        return result;

    /* Advance state to synced. */
    result = nvm_mirror_state_update(nvmmirrorp, STATE_SYNCED);
    if (result != HAL_SUCCESS)
        return result;

    return HAL_SUCCESS;
}

/**
 * @brief   Erases all sectors.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmmirrorMassErase(NVMMirrorDriver* nvmmirrorp)
{
    osalDbgCheck(nvmmirrorp != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmmirrorp->state >= NVM_READY, "invalid state");
    /* Verify mirror is in valid sync state. */
    osalDbgAssert(nvmmirrorp->mirror_state != STATE_DIRTY_B, "invalid mirror state");
    /* Set mirror state to dirty if necessary. */
    if (nvmmirrorp->mirror_state == STATE_SYNCED)
    {
        bool result = nvm_mirror_state_update(nvmmirrorp, STATE_DIRTY_A);
        if (result != HAL_SUCCESS)
            return result;
    }

    /* Erase operation in progress. */
    nvmmirrorp->state = NVM_ERASING;

    bool result;
    /* Set state to mirror a dirty before changing contents. */
    result = nvm_mirror_state_update(nvmmirrorp, STATE_DIRTY_A);
    if (result != HAL_SUCCESS)
        return result;

    /* Apply erase to mirror a. */
    result = nvmErase(nvmmirrorp->config->nvmp,
            nvmmirrorp->mirror_a_org, nvmmirrorp->mirror_size);
    if (result != HAL_SUCCESS)
        return result;

    /* Advance state to mirror b dirty. */
    result = nvm_mirror_state_update(nvmmirrorp, STATE_DIRTY_B);
    if (result != HAL_SUCCESS)
        return result;

    /* Apply erase to mirror b. */
    result = nvmErase(nvmmirrorp->config->nvmp,
            nvmmirrorp->mirror_b_org, nvmmirrorp->mirror_size);
    if (result != HAL_SUCCESS)
        return result;

    /* Advance state to synced. */
    result = nvm_mirror_state_update(nvmmirrorp, STATE_SYNCED);
    if (result != HAL_SUCCESS)
        return result;

    return HAL_SUCCESS;
}

/**
 * @brief   Waits for idle condition.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmmirrorSync(NVMMirrorDriver* nvmmirrorp)
{
    osalDbgCheck(nvmmirrorp != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmmirrorp->state >= NVM_READY, "invalid state");
    /* Verify mirror is in valid sync state. */
    osalDbgAssert(nvmmirrorp->mirror_state == STATE_SYNCED, "invalid mirror state");

    if (nvmmirrorp->state == NVM_READY)
        return HAL_SUCCESS;

    bool result = nvmSync(nvmmirrorp->config->nvmp);
    if (result != HAL_SUCCESS)
        return result;

    /* No more operation in progress. */
    nvmmirrorp->state = NVM_READY;

    return HAL_SUCCESS;
}

/**
 * @brief   Returns media info.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 * @param[out] nvmdip       pointer to a @p NVMDeviceInfo structure
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmmirrorGetInfo(NVMMirrorDriver* nvmmirrorp, NVMDeviceInfo* nvmdip)
{
    osalDbgCheck(nvmmirrorp != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmmirrorp->state >= NVM_READY, "invalid state");

    nvmdip->sector_num =
            (nvmmirrorp->llnvmdi.sector_num - nvmmirrorp->config->sector_header_num) / 2;
    nvmdip->sector_size =
            nvmmirrorp->llnvmdi.sector_size;
    memcpy(nvmdip->identification, nvmmirrorp->llnvmdi.identification,
           sizeof(nvmdip->identification));
    nvmdip->write_alignment =
            nvmmirrorp->llnvmdi.write_alignment;

    return HAL_SUCCESS;
}

/**
 * @brief   Gains exclusive access to the nvm device.
 * @details This function tries to gain ownership to the nvm device, if the
 *          device is already being used then the invoking thread is queued.
 * @pre     In order to use this function the option
 *          @p NVM_MIRROR_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 *
 * @api
 */
void nvmmirrorAcquireBus(NVMMirrorDriver* nvmmirrorp)
{
    osalDbgCheck(nvmmirrorp != NULL);

#if NVM_MIRROR_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
    osalMutexLock(&nvmmirrorp->mutex);

    /* Lock the underlying device as well */
    nvmAcquire(nvmmirrorp->config->nvmp);
#endif /* NVM_MIRROR_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the nvm device.
 * @pre     In order to use this function the option
 *          @p NVM_MIRROR_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 *
 * @api
 */
void nvmmirrorReleaseBus(NVMMirrorDriver* nvmmirrorp)
{
    osalDbgCheck(nvmmirrorp != NULL);

#if NVM_MIRROR_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
    osalMutexUnlock(&nvmmirrorp->mutex);

    /* Release the underlying device as well */
    nvmRelease(nvmmirrorp->config->nvmp);
#endif /* NVM_MIRROR_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Write protects one or more sectors.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 * @param[in] startaddr     address within to be protected sector
 * @param[in] n             number of bytes to protect
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmmirrorWriteProtect(NVMMirrorDriver* nvmmirrorp,
        uint32_t startaddr, uint32_t n)
{
    osalDbgCheck(nvmmirrorp != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmmirrorp->state >= NVM_READY, "invalid state");

    /* TODO: add implementation */

    return HAL_SUCCESS;
}

/**
 * @brief   Write protects the whole device.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmmirrorMassWriteProtect(NVMMirrorDriver* nvmmirrorp)
{
    osalDbgCheck(nvmmirrorp != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmmirrorp->state >= NVM_READY, "invalid state");

    /* TODO: add implementation */

    return HAL_SUCCESS;
}

/**
 * @brief   Write unprotects one or more sectors.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 * @param[in] startaddr     address within to be unprotected sector
 * @param[in] n             number of bytes to unprotect
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmmirrorWriteUnprotect(NVMMirrorDriver* nvmmirrorp,
        uint32_t startaddr, uint32_t n)
{
    osalDbgCheck(nvmmirrorp != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmmirrorp->state >= NVM_READY, "invalid state");

    /* TODO: add implementation */

    return HAL_SUCCESS;
}

/**
 * @brief   Write unprotects the whole device.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmmirrorMassWriteUnprotect(NVMMirrorDriver* nvmmirrorp)
{
    osalDbgCheck(nvmmirrorp != NULL);
    /* Verify device status. */
    osalDbgAssert(nvmmirrorp->state >= NVM_READY, "invalid state");

    /* TODO: add implementation */

    return HAL_SUCCESS;
}

#endif /* HAL_USE_NVM_MIRROR */

/** @} */
