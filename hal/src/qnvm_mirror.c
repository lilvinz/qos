/**
 * @file    qnvm_mirror.c
 * @brief   NVM mirror driver code.
 *
 * @addtogroup NVM_MIRROR
 * @{
 */

#include "ch.h"
#include "qhal.h"

#if HAL_USE_NVM_MIRROR || defined(__DOXYGEN__)

#include "static_assert.h"

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
 *                  State is changed to dirty b.
 *                  Execute sync of lower level driver.
 *                  Copy mirror a to mirror b erasing pages as required.
 *                  State is changed to synced.
 *                  Execute sync of lower level driver.
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
 *              - state dirty a:
 *                  Write(s) and / or erase(s) are being executed on mirror a.
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
STATIC_ASSERT(NELEMS(nvm_mirror_state_mark_table) == STATE_DIRTY_COUNT);
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
    .read = (bool_t (*)(void*, uint32_t, uint32_t, uint8_t*))nvmmirrorRead,
    .write = (bool_t (*)(void*, uint32_t, uint32_t, const uint8_t*))nvmmirrorWrite,
    .erase = (bool_t (*)(void*, uint32_t, uint32_t))nvmmirrorErase,
    .mass_erase = (bool_t (*)(void*))nvmmirrorMassErase,
    .sync = (bool_t (*)(void*))nvmmirrorSync,
    .get_info = (bool_t (*)(void*, NVMDeviceInfo*))nvmmirrorGetInfo,
    /* End of mandatory functions. */
    .acquire = (void (*)(void*))nvmmirrorAcquireBus,
    .release = (void (*)(void*))nvmmirrorReleaseBus,
    .writeprotect = (bool_t (*)(void*, uint32_t, uint32_t))nvmmirrorWriteProtect,
    .mass_writeprotect = (bool_t (*)(void*))nvmmirrorMassWriteProtect,
    .writeunprotect = (bool_t (*)(void*, uint32_t, uint32_t))nvmmirrorWriteUnprotect,
    .mass_writeunprotect = (bool_t (*)(void*))nvmmirrorMassWriteUnprotect,
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static bool_t nvm_mirror_state_init(NVMMirrorDriver* nvmmirrorp)
{
    chDbgCheck((nvmmirrorp != NULL), "nvm_mirror_state_init");

    const uint32_t header_orig = 0;
    const uint32_t header_size =
            nvmmirrorp->config->sector_header_num * nvmmirrorp->llnvmdi.sector_size;

    NVMMirrorState new_state = STATE_INVALID;
    uint32_t new_state_addr = 0;

    uint64_t state_mark;

    for (uint32_t i = header_orig;
            i < header_orig + header_size;
            i += sizeof(state_mark))
    {
        bool_t result = nvmRead(nvmmirrorp->config->nvmp,
                i,
                sizeof(state_mark),
                (uint8_t*)&state_mark);
        if (result != CH_SUCCESS)
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
            return CH_SUCCESS;
        }
    }

    nvmmirrorp->mirror_state = new_state;
    nvmmirrorp->mirror_state_addr = new_state_addr;

    return CH_SUCCESS;
}

static bool_t nvm_mirror_state_update(NVMMirrorDriver* nvmmirrorp,
        NVMMirrorState new_state)
{
    chDbgCheck((nvmmirrorp != NULL), "nvm_mirror_state_write");

    if (new_state == nvmmirrorp->mirror_state)
        return CH_SUCCESS;

    const uint32_t header_orig = 0;
    const uint32_t header_size =
            nvmmirrorp->config->sector_header_num * nvmmirrorp->llnvmdi.sector_size;

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
        bool_t result = nvmErase(nvmmirrorp->config->nvmp, header_orig,
                header_size);
        if (result != CH_SUCCESS)
            return result;
    }

    /* Write updated state entry. */
    {
        bool_t result = nvmWrite(nvmmirrorp->config->nvmp, new_state_addr,
                sizeof(new_state_mark), (uint8_t*)&new_state_mark);
        if (result != CH_SUCCESS)
            return result;
    }

    /* Sync lower level driver. */
    {
        bool_t result = nvmSync(nvmmirrorp->config->nvmp);
        if (result != CH_SUCCESS)
            return result;
    }

    nvmmirrorp->mirror_state = new_state;
    nvmmirrorp->mirror_state_addr = new_state_addr;

    return CH_SUCCESS;
}

static bool_t nvm_mirror_copy(NVMMirrorDriver* nvmmirrorp, uint32_t src_addr,
        uint32_t dst_addr, size_t n)
{
    chDbgCheck((nvmmirrorp != NULL), "nvm_mirror_copy");

    uint64_t state_mark;

    for (uint32_t offset = 0; offset < n; offset += sizeof(state_mark))
    {
        /* Detect start of a new sector and erase destination accordingly. */
        if ((offset % nvmmirrorp->llnvmdi.sector_size) == 0)
        {
            bool_t result = nvmErase(nvmmirrorp->config->nvmp,
                    dst_addr + offset, nvmmirrorp->llnvmdi.sector_size);
            if (result != CH_SUCCESS)
                return result;
        }

        /* Read mark into temporary buffer. */
        {
            bool_t result = nvmRead(nvmmirrorp->config->nvmp,
                    src_addr + offset, sizeof(state_mark),
                    (uint8_t*)&state_mark);
            if (result != CH_SUCCESS)
                return result;
        }

        /* Write mark to destination. */
        {
            bool_t result = nvmWrite(nvmmirrorp->config->nvmp,
                    dst_addr + offset, sizeof(state_mark),
                    (uint8_t*)&state_mark);
            if (result != CH_SUCCESS)
                return result;
        }
    }

    return CH_SUCCESS;
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
#if CH_USE_MUTEXES
    chMtxInit(&nvmmirrorp->mutex);
#else
    chSemInit(&nvmmirrorp->semaphore, 1);
#endif
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
    chDbgCheck((nvmmirrorp != NULL) && (config != NULL), "nvmmirrorStart");
    /* Verify device status. */
    chDbgAssert((nvmmirrorp->state == NVM_STOP) || (nvmmirrorp->state == NVM_READY),
            "nvmmirrorStart(), #1", "invalid state");

    nvmmirrorp->config = config;
    nvmGetInfo(nvmmirrorp->config->nvmp, &nvmmirrorp->llnvmdi);

    nvm_mirror_state_init(nvmmirrorp);

    {
        const uint32_t mirror_size =
                (nvmmirrorp->llnvmdi.sector_num - nvmmirrorp->config->sector_header_num) /
                2 * nvmmirrorp->llnvmdi.sector_size;
        const uint32_t mirror_a_org =
                nvmmirrorp->llnvmdi.sector_size * nvmmirrorp->config->sector_header_num;
        const uint32_t mirror_b_org =
                mirror_a_org + mirror_size;

        switch (nvmmirrorp->mirror_state)
        {
        case STATE_DIRTY_A:
            /* Copy mirror b to mirror a erasing pages as required. */
            if (nvm_mirror_copy(nvmmirrorp, mirror_b_org, mirror_a_org, mirror_size) != CH_SUCCESS)
                return;
            /* Set state to synced. */
            if (nvm_mirror_state_update(nvmmirrorp, STATE_SYNCED) != CH_SUCCESS)
                return;
            /* Execute sync of lower level driver. */
            if (nvmSync(nvmmirrorp->config->nvmp) != CH_SUCCESS)
                return;
            break;
        case STATE_INVALID:
            /* Invalid state (all header invalid) assumes mirror b to be dirty. */
        case STATE_DIRTY_B:
            /* Copy mirror a to mirror b erasing pages as required. */
            if (nvm_mirror_copy(nvmmirrorp, mirror_a_org, mirror_b_org, mirror_size) != CH_SUCCESS)
                return;
            /* Set state to synced. */
            if (nvm_mirror_state_update(nvmmirrorp, STATE_SYNCED) != CH_SUCCESS)
                return;
            /* Execute sync of lower level driver. */
            if (nvmSync(nvmmirrorp->config->nvmp) != CH_SUCCESS)
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
    chDbgCheck(nvmmirrorp != NULL, "nvmmirrorStop");
    /* Verify device status. */
    chDbgAssert((nvmmirrorp->state == NVM_STOP) || (nvmmirrorp->state == NVM_READY),
            "nvmmirrorStop(), #1", "invalid state");

    /* Verify mirror is in valid sync state. */
    chDbgAssert(nvmmirrorp->mirror_state == STATE_SYNCED, "nvmmirrorStop(), #2",
            "invalid mirror state");

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
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmirrorRead(NVMMirrorDriver* nvmmirrorp, uint32_t startaddr,
        uint32_t n, uint8_t* buffer)
{
    chDbgCheck(nvmmirrorp != NULL, "nvmmirrorRead");
    /* Verify device status. */
    chDbgAssert(nvmmirrorp->state >= NVM_READY, "nvmmirrorRead(), #1",
            "invalid state");

    /* Verify range is within mirror size. */
    chDbgAssert(
            (startaddr + n <=
                (nvmmirrorp->llnvmdi.sector_num - nvmmirrorp->config->sector_header_num) /
                2 * nvmmirrorp->llnvmdi.sector_size),
            "nvmmirrorRead(), #2", "invalid parameters");

    /* Verify mirror is in valid sync state. */
    chDbgAssert(nvmmirrorp->mirror_state != STATE_DIRTY_B, "nvmmirrorRead(), #3",
            "invalid mirror state");

    /* Read operation in progress. */
    nvmmirrorp->state = NVM_READING;

    bool_t result = nvmRead(nvmmirrorp->config->nvmp,
            nvmmirrorp->llnvmdi.sector_size * nvmmirrorp->config->sector_header_num + startaddr,
            n, buffer);

    /* Read operation finished. */
    nvmmirrorp->state = NVM_READY;

    return result;
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
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmirrorWrite(NVMMirrorDriver* nvmmirrorp, uint32_t startaddr,
        uint32_t n, const uint8_t* buffer)
{
    chDbgCheck(nvmmirrorp != NULL, "nvmmirrorWrite");
    /* Verify device status. */
    chDbgAssert(nvmmirrorp->state >= NVM_READY, "nvmmirrorWrite(), #1",
            "invalid state");

    /* Verify range is within mirror size. */
    chDbgAssert(
            (startaddr + n <=
                (nvmmirrorp->llnvmdi.sector_num - nvmmirrorp->config->sector_header_num) /
                2 * nvmmirrorp->llnvmdi.sector_size),
            "nvmmirrorWrite(), #2", "invalid parameters");

    /* Verify mirror is in valid sync state. */
    chDbgAssert(nvmmirrorp->mirror_state != STATE_DIRTY_B, "nvmmirrorWrite(), #3",
            "invalid mirror state");

    /* Set mirror state to dirty if necessary. */
    if (nvmmirrorp->mirror_state == STATE_SYNCED)
    {
        bool_t result = nvm_mirror_state_update(nvmmirrorp, STATE_DIRTY_A);
        if (result != CH_SUCCESS)
            return result;
    }

    /* Write operation in progress. */
    nvmmirrorp->state = NVM_WRITING;

    return nvmWrite(nvmmirrorp->config->nvmp,
            nvmmirrorp->llnvmdi.sector_size * nvmmirrorp->config->sector_header_num + startaddr,
            n, buffer);
}

/**
 * @brief   Erases one or more sectors.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 * @param[in] startaddr     address within to be erased sector
 * @param[in] n             number of bytes to erase
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmirrorErase(NVMMirrorDriver* nvmmirrorp, uint32_t startaddr, uint32_t n)
{
    chDbgCheck(nvmmirrorp != NULL, "nvmmirrorErase");
    /* Verify device status. */
    chDbgAssert(nvmmirrorp->state >= NVM_READY, "nvmmirrorErase(), #1",
            "invalid state");

    /* Verify range is within mirror size. */
    chDbgAssert(
            (startaddr + n <=
                (nvmmirrorp->llnvmdi.sector_num - nvmmirrorp->config->sector_header_num) /
                2 * nvmmirrorp->llnvmdi.sector_size),
            "nvmmirrorErase(), #2", "invalid parameters");

    /* Verify mirror is in valid sync state. */
    chDbgAssert(nvmmirrorp->mirror_state != STATE_DIRTY_B, "nvmmirrorErase(), #3",
            "invalid mirror state");

    /* Set mirror state to dirty if necessary. */
    if (nvmmirrorp->mirror_state == STATE_SYNCED)
    {
        bool_t result = nvm_mirror_state_update(nvmmirrorp, STATE_DIRTY_A);
        if (result != CH_SUCCESS)
            return result;
    }

    /* Erase operation in progress. */
    nvmmirrorp->state = NVM_ERASING;

    return nvmErase(nvmmirrorp->config->nvmp,
            nvmmirrorp->llnvmdi.sector_size * nvmmirrorp->config->sector_header_num + startaddr,
            n);
}

/**
 * @brief   Erases all sectors.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmirrorMassErase(NVMMirrorDriver* nvmmirrorp)
{
    chDbgCheck(nvmmirrorp != NULL, "nvmmirrorMassErase");
    /* Verify device status. */
    chDbgAssert(nvmmirrorp->state >= NVM_READY, "nvmmirrorMassErase(), #1",
            "invalid state");

    /* Verify mirror is in valid sync state. */
    chDbgAssert(nvmmirrorp->mirror_state != STATE_DIRTY_B, "nvmmirrorMassErase(), #3",
            "invalid mirror state");

    /* Set mirror state to dirty if necessary. */
    if (nvmmirrorp->mirror_state == STATE_SYNCED)
    {
        bool_t result = nvm_mirror_state_update(nvmmirrorp, STATE_DIRTY_A);
        if (result != CH_SUCCESS)
            return result;
    }

    /* Erase operation in progress. */
    nvmmirrorp->state = NVM_ERASING;

    return nvmErase(nvmmirrorp->config->nvmp,
            nvmmirrorp->llnvmdi.sector_size * nvmmirrorp->config->sector_header_num,
            nvmmirrorp->llnvmdi.sector_size *
            (nvmmirrorp->llnvmdi.sector_num - nvmmirrorp->config->sector_header_num));
}

/**
 * @brief   Waits for idle condition.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmirrorSync(NVMMirrorDriver* nvmmirrorp)
{
    chDbgCheck(nvmmirrorp != NULL, "nvmmirrorSync");
    /* Verify device status. */
    chDbgAssert(nvmmirrorp->state >= NVM_READY, "nvmmirrorSync(), #1",
            "invalid state");

    /* Verify mirror is in valid sync state. */
    chDbgAssert(nvmmirrorp->mirror_state != STATE_DIRTY_B, "nvmmirrorSync(), #2",
            "invalid mirror state");

    if (nvmmirrorp->mirror_state == STATE_SYNCED)
        return nvmSync(nvmmirrorp->config->nvmp);

    /* Update mirror state to dirty_b to inidicate sync in progress. */
    {
        bool_t result = nvm_mirror_state_update(nvmmirrorp, STATE_DIRTY_B);
        if (result != CH_SUCCESS)
            return result;
    }

    /* Copy mirror a contents to mirror b. */
    {
        const uint32_t mirror_size =
                (nvmmirrorp->llnvmdi.sector_num - nvmmirrorp->config->sector_header_num) /
                2 * nvmmirrorp->llnvmdi.sector_size;
        const uint32_t mirror_a_org =
                nvmmirrorp->llnvmdi.sector_size * nvmmirrorp->config->sector_header_num;
        const uint32_t mirror_b_org =
                mirror_a_org + mirror_size;

        /* Copy mirror a to mirror b erasing pages as required. */
        bool_t result = nvm_mirror_copy(nvmmirrorp, mirror_a_org, mirror_b_org, mirror_size);
        if (result != CH_SUCCESS)
            return result;
    }

    /* Update mirror state to synced. */
    {
        bool_t result = nvm_mirror_state_update(nvmmirrorp, STATE_SYNCED);
        if (result != CH_SUCCESS)
            return result;
    }

    /* No more operation in progress. */
    nvmmirrorp->state = NVM_READY;

    return CH_SUCCESS;
}

/**
 * @brief   Returns media info.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 * @param[out] nvmdip       pointer to a @p NVMDeviceInfo structure
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmirrorGetInfo(NVMMirrorDriver* nvmmirrorp, NVMDeviceInfo* nvmdip)
{
    chDbgCheck(nvmmirrorp != NULL, "nvmmirrorGetInfo");
    /* Verify device status. */
    chDbgAssert(nvmmirrorp->state >= NVM_READY, "nvmmirrorGetInfo(), #1",
            "invalid state");

    nvmdip->sector_num =
            (nvmmirrorp->llnvmdi.sector_num - nvmmirrorp->config->sector_header_num) / 2;
    nvmdip->sector_size =
            nvmmirrorp->llnvmdi.sector_size;
    memcpy(nvmdip->identification, nvmmirrorp->llnvmdi.identification,
           sizeof(nvmdip->identification));

    return CH_SUCCESS;
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
    chDbgCheck(nvmmirrorp != NULL, "nvmmirrorAcquireBus");

#if NVM_MIRROR_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    chMtxLock(&nvmmirrorp->mutex);
#elif CH_USE_SEMAPHORES
    chSemWait(&nvmmirrorp->semaphore);
#endif

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
    chDbgCheck(nvmmirrorp != NULL, "nvmmirrorReleaseBus");

#if NVM_MIRROR_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES
    chMtxUnlock();
#elif CH_USE_SEMAPHORES
    chSemSignal(&nvmmirrorp->semaphore);
#endif

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
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmirrorWriteProtect(NVMMirrorDriver* nvmmirrorp,
        uint32_t startaddr, uint32_t n)
{
    chDbgCheck(nvmmirrorp != NULL, "nvmmirrorWriteProtect");
    /* Verify device status. */
    chDbgAssert(nvmmirrorp->state >= NVM_READY, "nvmmirrorWriteProtect(), #1",
            "invalid state");

    /* TODO: add implementation */

    return CH_SUCCESS;
}

/**
 * @brief   Write protects the whole device.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmirrorMassWriteProtect(NVMMirrorDriver* nvmmirrorp)
{
    chDbgCheck(nvmmirrorp != NULL, "nvmmirrorMassWriteProtect");
    /* Verify device status. */
    chDbgAssert(nvmmirrorp->state >= NVM_READY, "nvmmirrorMassWriteProtect(), #1",
            "invalid state");

    /* TODO: add implementation */

    return CH_SUCCESS;
}

/**
 * @brief   Write unprotects one or more sectors.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 * @param[in] startaddr     address within to be unprotected sector
 * @param[in] n             number of bytes to unprotect
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmirrorWriteUnprotect(NVMMirrorDriver* nvmmirrorp,
        uint32_t startaddr, uint32_t n)
{
    chDbgCheck(nvmmirrorp != NULL, "nvmmirrorWriteUnprotect");
    /* Verify device status. */
    chDbgAssert(nvmmirrorp->state >= NVM_READY, "nvmmirrorWriteUnprotect(), #1",
            "invalid state");

    /* TODO: add implementation */

    return CH_SUCCESS;
}

/**
 * @brief   Write unprotects the whole device.
 *
 * @param[in] nvmmirrorp    pointer to the @p NVMMirrorDriver object
 *
 * @return                  The operation status.
 * @retval CH_SUCCESS       the operation succeeded.
 * @retval CH_FAILED        the operation failed.
 *
 * @api
 */
bool_t nvmmirrorMassWriteUnprotect(NVMMirrorDriver* nvmmirrorp)
{
    chDbgCheck(nvmmirrorp != NULL, "nvmmirrorMassWriteUnprotect");
    /* Verify device status. */
    chDbgAssert(nvmmirrorp->state >= NVM_READY, "nvmmirrorMassWriteUnprotect(), #1",
            "invalid state");

    /* TODO: add implementation */

    return CH_SUCCESS;
}

#endif /* HAL_USE_NVM_MIRROR */

/** @} */
