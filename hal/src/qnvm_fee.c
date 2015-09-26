/**
 * @file    qnvm_fee.c
 * @brief   NVM fee driver code.
 *
 * @addtogroup NVM_FEE
 * @{
 */

#include "qhal.h"

#if HAL_USE_NVM_FEE || defined(__DOXYGEN__)

#include "static_assert.h"
#include "nelems.h"

#include <string.h>

/*
 * @brief   This is a logging file system.
 *          Data consistency is guaranteed as long as there is no
 *          hardware defect. The device can be written up to 100% of its
 *          reported size. Garbage collection is performed automatically when
 *          necessary. The number of write is minimized by comparing to be
 *          written data with current content prio to executing write to the
 *          underlying device.
 *
 *          The memory partitioning is:
 *          - arena a
 *            - arena header
 *            - slots
 *          - arena b
 *            - arena header
 *            - slots
 *
 * @todo    - add write protection pass-through to lower level driver
 *
 */

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

static const uint32_t nvm_fee_magic = 0x86618c51 + NVM_FEE_SLOT_PAYLOAD_SIZE;

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Virtual methods table.
 */
static const struct NVMFeeDriverVMT nvm_fee_vmt =
{
    .read = (bool (*)(void*, uint32_t, uint32_t, uint8_t*))nvmfeeRead,
    .write = (bool (*)(void*, uint32_t, uint32_t, const uint8_t*))nvmfeeWrite,
    .erase = (bool (*)(void*, uint32_t, uint32_t))nvmfeeErase,
    .mass_erase = (bool (*)(void*))nvmfeeMassErase,
    .sync = (bool (*)(void*))nvmfeeSync,
    .get_info = (bool (*)(void*, NVMDeviceInfo*))nvmfeeGetInfo,
    /* End of mandatory functions. */
    .acquire = (void (*)(void*))nvmfeeAcquireBus,
    .release = (void (*)(void*))nvmfeeReleaseBus,
    .writeprotect = (bool (*)(void*, uint32_t, uint32_t))nvmfeeWriteProtect,
    .mass_writeprotect = (bool (*)(void*))nvmfeeMassWriteProtect,
    .writeunprotect = (bool (*)(void*, uint32_t, uint32_t))nvmfeeWriteUnprotect,
    .mass_writeunprotect = (bool (*)(void*))nvmfeeMassWriteUnprotect,
};

/**
 * @brief   Enum representing the internal state of an arena
 */
enum arena_state
{
    ARENA_STATE_UNUSED = 0,         /**< Not initialized.                    */
    ARENA_STATE_ACTIVE,             /**< Arena is in use.                    */
    ARENA_STATE_FROZEN,             /**< No more writes are allowed.         */
    ARENA_STATE_UNKNOWN,            /**< Unable to read state mark.          */
    ARENA_STATE_COUNT,
};

/**
 * @brief   Enum representing the internal state of a slot
 */
enum slot_state
{
    SLOT_STATE_UNUSED = 0,          /**< Not initialized.                    */
    SLOT_STATE_DIRTY,               /**< Slot data is being updated.         */
    SLOT_STATE_VALID,               /**< Slot data is valid.                 */
    SLOT_STATE_UNKNOWN,             /**< Unable to read state mark.          */
    SLOT_STATE_COUNT,
};

/*
 * @brief   State marks have to be chosen so that they can be represented on
 *          any nvm device including devices with the following
 *          limitations:
 *          - Writes are not allowed to cells which contain low bits.
 *          - Smallest write operation is 16bits
 *
 *          Also note that the chosen patterns requires little endian byte order.
 */
static const uint32_t nvm_fee_state_mark_table[] =
{
    0xffffffff,
    0xffff0000,
    0x00000000,
};
STATIC_ASSERT(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);

/**
 * @brief   Header structure at the beginning of each arena.
 */
struct __attribute__((__packed__)) arena_header
{
    uint32_t magic;
    uint32_t state_mark;
    uint32_t unused[6];
};

/**
 * @brief   Structure defining a single slot.
 */
struct __attribute__((__packed__)) slot
{
    uint32_t state_mark;
    uint32_t address;
    uint8_t payload[NVM_FEE_SLOT_PAYLOAD_SIZE];
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/
static uint32_t nvm_fee_slot_state_2_mark(enum slot_state state)
{
    return nvm_fee_state_mark_table[state];
}

static enum slot_state nvm_fee_mark_2_slot_state(uint32_t mark)
{
    if (mark == nvm_fee_state_mark_table[SLOT_STATE_UNUSED])
        return SLOT_STATE_UNUSED;
    else if (mark == nvm_fee_state_mark_table[SLOT_STATE_DIRTY])
        return SLOT_STATE_DIRTY;
    else if (mark == nvm_fee_state_mark_table[SLOT_STATE_VALID])
        return SLOT_STATE_VALID;

    return SLOT_STATE_UNKNOWN;
}

static bool nvm_fee_slot_read(NVMFeeDriver* nvmfeep, uint32_t arena,
        uint32_t slot, struct slot* slotp)
{
    chDbgCheck((nvmfeep != NULL));

    const uint32_t addr = arena *
            nvmfeep->arena_num_sectors * nvmfeep->llnvmdi.sector_size +
            sizeof(struct arena_header) + slot * sizeof(struct slot);

    bool result = nvmRead(nvmfeep->config->nvmp, addr,
            sizeof(*slotp), (uint8_t*)slotp);
    if (result != HAL_SUCCESS)
        return result;

    return HAL_SUCCESS;
}

static bool nvm_fee_slot_state_update(NVMFeeDriver* nvmfeep, uint32_t arena,
        uint32_t slot, enum slot_state state)
{
    chDbgCheck((nvmfeep != NULL));

    const uint32_t addr = arena *
            nvmfeep->arena_num_sectors * nvmfeep->llnvmdi.sector_size +
            sizeof(struct arena_header) + slot * sizeof(struct slot);

    const struct slot temp_slot =
    {
        .state_mark = nvm_fee_slot_state_2_mark(state),
    };

    bool result = nvmWrite(nvmfeep->config->nvmp, addr,
            sizeof(temp_slot.state_mark), (uint8_t*)&temp_slot.state_mark);
    if (result != HAL_SUCCESS)
        return result;

    return HAL_SUCCESS;
}

static bool nvm_fee_slot_write(NVMFeeDriver* nvmfeep, uint32_t arena,
        uint32_t slot, const struct slot* slotp)
{
    chDbgCheck((nvmfeep != NULL));
    /* Verify state of to be written slot. */
    chDbgAssert(nvm_fee_mark_2_slot_state(slotp->state_mark) ==
            SLOT_STATE_VALID, "invalid state");

    const uint32_t addr = arena *
            nvmfeep->arena_num_sectors * nvmfeep->llnvmdi.sector_size +
            sizeof(struct arena_header) + slot * sizeof(struct slot);

    bool result;

    /* Set slot to dirty.*/
    result = nvm_fee_slot_state_update(nvmfeep, arena,
            slot, SLOT_STATE_DIRTY);
    if (result != HAL_SUCCESS)
        return result;

    /* Write new slot. */
    result = nvmWrite(nvmfeep->config->nvmp, addr,
            sizeof(*slotp), (uint8_t*)slotp);
    if (result != HAL_SUCCESS)
        return result;

    return HAL_SUCCESS;
}

static bool nvm_fee_slot_lookup(NVMFeeDriver* nvmfeep, uint32_t arena,
        uint32_t address, uint32_t* slotp, bool* foundp)
{
    chDbgCheck((nvmfeep != NULL));

    *foundp = false;

    /* Walk through active slots. */
    for (uint32_t slot = 0;
            slot < nvmfeep->arena_slots[nvmfeep->arena_active];
            ++slot)
    {
        struct slot temp_slot;
        bool result;

        /* Read slot. */
        result = nvm_fee_slot_read(nvmfeep, arena, slot, &temp_slot);
        if (result != HAL_SUCCESS)
            return result;

        /* Skip if slot is not in valid state. */
        if (nvm_fee_mark_2_slot_state(temp_slot.state_mark) != SLOT_STATE_VALID)
            continue;

        /* Check if slot's address matches. */
        if (temp_slot.address == address)
        {
            *foundp = true;
            *slotp = slot;
        }
    }

    return HAL_SUCCESS;
}

static uint32_t nvm_fee_arena_state_2_mark(enum arena_state state)
{
    return nvm_fee_state_mark_table[state];
}

static enum arena_state nvm_fee_mark_2_arena_state(uint32_t mark)
{
    if (mark == nvm_fee_state_mark_table[ARENA_STATE_UNUSED])
        return ARENA_STATE_UNUSED;
    else if (mark == nvm_fee_state_mark_table[ARENA_STATE_ACTIVE])
        return ARENA_STATE_ACTIVE;
    else if (mark == nvm_fee_state_mark_table[ARENA_STATE_FROZEN])
        return ARENA_STATE_FROZEN;

    return ARENA_STATE_UNKNOWN;
}

static enum arena_state nvm_fee_arena_state_get(NVMFeeDriver* nvmfeep,
        uint32_t arena)
{
    chDbgCheck((nvmfeep != NULL));

    const uint32_t addr = arena *
            nvmfeep->arena_num_sectors * nvmfeep->llnvmdi.sector_size;

    struct arena_header header;

    bool result = nvmRead(nvmfeep->config->nvmp, addr,
            sizeof(header), (uint8_t*)&header);
    if (result != HAL_SUCCESS)
        return ARENA_STATE_UNKNOWN;

    if (header.magic != nvm_fee_magic)
        return ARENA_STATE_UNKNOWN;

    return nvm_fee_mark_2_arena_state(header.state_mark);
}

static bool nvm_fee_arena_state_update(NVMFeeDriver* nvmfeep, uint32_t arena,
        enum arena_state state)
{
    chDbgCheck((nvmfeep != NULL));

    const uint32_t addr = arena *
            nvmfeep->arena_num_sectors * nvmfeep->llnvmdi.sector_size;

    const struct arena_header header =
    {
        .state_mark = nvm_fee_arena_state_2_mark(state),
    };

    bool result = nvmWrite(nvmfeep->config->nvmp,
            addr + offsetof(struct arena_header, state_mark),
            sizeof(header.state_mark), (uint8_t*)&header.state_mark);
    if (result != HAL_SUCCESS)
        return result;

    return HAL_SUCCESS;
}

static bool nvm_fee_arena_load(NVMFeeDriver* nvmfeep, uint32_t arena)
{
    chDbgCheck((nvmfeep != NULL));

    nvmfeep->arena_slots[arena] = 0;

    bool result;

    /* Walk through active slots. */
    for (uint32_t slot = 0;
            slot < nvmfeep->arena_num_slots;
            ++slot)
    {
        struct slot temp_slot;

        /* Read slot. */
        result = nvm_fee_slot_read(nvmfeep, arena, slot, &temp_slot);
        if (result != HAL_SUCCESS)
            return result;

        if (nvm_fee_mark_2_slot_state(temp_slot.state_mark) !=
                SLOT_STATE_UNUSED)
        {
            nvmfeep->arena_slots[arena] = slot + 1;
        }
    }

    return HAL_SUCCESS;
}

static bool nvm_fee_arena_erase(NVMFeeDriver* nvmfeep, uint32_t arena)
{
    chDbgCheck((nvmfeep != NULL));

    const uint32_t addr = arena *
            nvmfeep->arena_num_sectors * nvmfeep->llnvmdi.sector_size;

    bool result;

    /* Mass erase underlying nvm device. */
    result = nvmErase(nvmfeep->config->nvmp, addr,
            nvmfeep->arena_num_sectors * nvmfeep->llnvmdi.sector_size);
    if (result != HAL_SUCCESS)
        return result;

    /* Set magic. */
    const struct arena_header header =
    {
        .magic = nvm_fee_magic,
        .state_mark = nvm_fee_arena_state_2_mark(ARENA_STATE_UNUSED),
    };

    result = nvmWrite(nvmfeep->config->nvmp, addr,
            sizeof(header), (uint8_t*)&header);
    if (result != HAL_SUCCESS)
        return result;

    nvmfeep->arena_slots[arena] = 0;

    return HAL_SUCCESS;
}

static bool nvm_fee_gc(NVMFeeDriver* nvmfeep, uint32_t omit_addr)
{
    chDbgCheck((nvmfeep != NULL));

    uint32_t src_arena;
    uint32_t dst_arena;

    if (nvmfeep->arena_active == 0)
    {
        src_arena = 0;
        dst_arena = 1;
    }
    else
    {
        src_arena = 1;
        dst_arena = 0;
    }

    nvmfeep->arena_slots[dst_arena] = 0;

    bool result;

    /* stage 1: Freeze source arena. */
    result = nvm_fee_arena_state_update(nvmfeep, src_arena, ARENA_STATE_FROZEN);
    if (result != HAL_SUCCESS)
        return result;

    /* stage 2: Copy active slots to destination arena. */
    for (uint32_t addr = 0;
            addr < nvmfeep->fee_size;
            addr += NVM_FEE_SLOT_PAYLOAD_SIZE)
    {
        /* Skip one slot to allow full write. */
        if (addr == omit_addr)
            continue;

        /* Look for existing slot. */
        bool found;
        uint32_t slot;

        result = nvm_fee_slot_lookup(nvmfeep, src_arena, addr, &slot, &found);
        if (result != HAL_SUCCESS)
            return result;

        if (found)
        {
            /* Read slot. */
            struct slot temp_slot;
            result = nvm_fee_slot_read(nvmfeep, src_arena, slot, &temp_slot);
            if (result != HAL_SUCCESS)
                return result;

            /* Write new slot. */
            result = nvm_fee_slot_write(nvmfeep, dst_arena,
                    nvmfeep->arena_slots[dst_arena], &temp_slot);
            if (result != HAL_SUCCESS)
                return result;

            ++nvmfeep->arena_slots[dst_arena];
        }
    }

    /* stage 3: Activate destination arena. */
    result = nvm_fee_arena_state_update(nvmfeep, dst_arena, ARENA_STATE_ACTIVE);
    if (result != HAL_SUCCESS)
        return result;

    /* stage 4: Reinit source arena. */
    result = nvm_fee_arena_erase(nvmfeep, src_arena);
    if (result != HAL_SUCCESS)
        return result;

    /* Update driver state. */
    nvmfeep->arena_active = dst_arena;

    return HAL_SUCCESS;
}

static bool nvm_fee_read(NVMFeeDriver* nvmfeep, uint32_t arena,
        uint32_t startaddr, uint32_t n, uint8_t* buffer)
{
    chDbgCheck((nvmfeep != NULL));

    const uint32_t first_slot_addr = startaddr -
            (startaddr % NVM_FEE_SLOT_PAYLOAD_SIZE);
    const uint32_t last_slot_addr = (startaddr + n) -
            ((startaddr + n) % NVM_FEE_SLOT_PAYLOAD_SIZE);
    const uint32_t pre_pad = startaddr % NVM_FEE_SLOT_PAYLOAD_SIZE;
    const uint32_t post_pad = NVM_FEE_SLOT_PAYLOAD_SIZE -
            ((startaddr + n) % NVM_FEE_SLOT_PAYLOAD_SIZE);

    /* Initially set all content to 0xff. */
    for (uint32_t i = 0; i < n; ++i)
        buffer[i] = 0xff;

    /* Walk through active slots. */
    for (uint32_t slot = 0;
            slot < nvmfeep->arena_slots[nvmfeep->arena_active];
            ++slot)
    {
        bool result;

        /* Read slot. */
        struct slot temp_slot;
        result = nvm_fee_slot_read(nvmfeep, arena, slot, &temp_slot);
        if (result != HAL_SUCCESS)
            return result;

        /* Skip if slot is not valid. */
        if (nvm_fee_mark_2_slot_state(temp_slot.state_mark) !=
                SLOT_STATE_VALID)
            continue;

        /* Check if slot's data is within our desired range. */
        if (temp_slot.address == first_slot_addr)
        {
            /* First (partial) slot */
            uint32_t n_slot = NVM_FEE_SLOT_PAYLOAD_SIZE - pre_pad;
            if (n_slot > n)
                n_slot = n;
            memcpy(buffer,
                    temp_slot.payload + pre_pad,
                    n_slot);
        }
        else if (temp_slot.address == last_slot_addr)
        {
            /* Last (partial) slot */
            memcpy(buffer + n - (NVM_FEE_SLOT_PAYLOAD_SIZE - post_pad),
                    temp_slot.payload,
                    NVM_FEE_SLOT_PAYLOAD_SIZE - post_pad);
        }
        else if (temp_slot.address > first_slot_addr &&
                temp_slot.address < last_slot_addr)
        {
            /* Full slot */
            memcpy(buffer + temp_slot.address - startaddr,
                    temp_slot.payload,
                    NVM_FEE_SLOT_PAYLOAD_SIZE);
        }
    }

    return HAL_SUCCESS;
}

static int memtst(const void* block, int c, size_t size)
{
    for (size_t i = 0; i < size; ++i)
        if (((uint8_t*)block)[i] != c)
            return 1;
    return 0;
}

static bool nvm_fee_write(NVMFeeDriver* nvmfeep, uint32_t arena,
        uint32_t startaddr, uint32_t n, const uint8_t* buffer)
{
    chDbgCheck((nvmfeep != NULL));

    const uint32_t first_slot_addr = startaddr -
            (startaddr % NVM_FEE_SLOT_PAYLOAD_SIZE);
    const uint32_t pre_pad = startaddr % NVM_FEE_SLOT_PAYLOAD_SIZE;

    bool result;
    uint32_t n_remaining = n;
    uint32_t addr = startaddr;

    /* First (partial) slot */
    if (pre_pad)
    {
        /* Look for existing slot. */
        struct slot temp_slot;
        bool found;
        uint32_t slot;

        result = nvm_fee_slot_lookup(nvmfeep, arena, first_slot_addr,
                &slot, &found);
        if (result != HAL_SUCCESS)
            return result;

        if (found == true)
        {
            /* Existing slot so read it. */
            result = nvm_fee_slot_read(nvmfeep, arena, slot, &temp_slot);
            if (result != HAL_SUCCESS)
                return result;
        }
        else
        {
            /* No existing slot so initialize a pristine one. */
            temp_slot.state_mark = nvm_fee_slot_state_2_mark(SLOT_STATE_VALID);
            temp_slot.address = first_slot_addr;
            memset(temp_slot.payload, 0xff, sizeof(temp_slot.payload));
        }

        uint32_t n_slot = NVM_FEE_SLOT_PAYLOAD_SIZE - pre_pad;
        if (n_slot > n_remaining)
            n_slot = n_remaining;

        /* Compare slot data. */
        if (memcmp(temp_slot.payload + pre_pad, buffer, n_slot) != 0)
        {
            /* Update slot data. */
            memcpy(temp_slot.payload + pre_pad, buffer, n_slot);

            /* Check if arena is full and execute garbage collection. */
            if (nvmfeep->arena_slots[nvmfeep->arena_active] ==
                    nvmfeep->arena_num_slots)
            {
                result = nvm_fee_gc(nvmfeep, temp_slot.address);
                if (result != HAL_SUCCESS)
                    return result;
            }

            /* Write new slot. */
            result = nvm_fee_slot_write(nvmfeep, nvmfeep->arena_active,
                    nvmfeep->arena_slots[nvmfeep->arena_active], &temp_slot);
            if (result != HAL_SUCCESS)
                return result;

            ++nvmfeep->arena_slots[nvmfeep->arena_active];
        }

        addr += n_slot;
        n_remaining -= n_slot;
    }

    /* Full slots */
    while (n_remaining >= NVM_FEE_SLOT_PAYLOAD_SIZE)
    {
        /* Look for existing slot. */
        struct slot temp_slot;
        bool found;
        uint32_t slot;

        result = nvm_fee_slot_lookup(nvmfeep, arena, addr,
                &slot, &found);
        if (result != HAL_SUCCESS)
            return result;

        if (found == true)
        {
            /* Existing slot so read it. */
            result = nvm_fee_slot_read(nvmfeep, arena, slot, &temp_slot);
            if (result != HAL_SUCCESS)
                return result;
        }
        else
        {
            /* No existing slot so initialize a pristine one. */
            temp_slot.state_mark = nvm_fee_slot_state_2_mark(SLOT_STATE_VALID);
            temp_slot.address = addr;
            memset(temp_slot.payload, 0xff, sizeof(temp_slot.payload));
        }

        uint32_t n_slot = NVM_FEE_SLOT_PAYLOAD_SIZE;

        /* Compare slot data. */
        if (memcmp(temp_slot.payload, buffer + (addr - startaddr), n_slot) != 0)
        {
            /* Update slot data. */
            memcpy(temp_slot.payload, buffer + (addr - startaddr), n_slot);

            /* Check if arena is full and execute garbage collection. */
            if (nvmfeep->arena_slots[nvmfeep->arena_active] ==
                    nvmfeep->arena_num_slots)
            {
                result = nvm_fee_gc(nvmfeep, temp_slot.address);
                if (result != HAL_SUCCESS)
                    return result;
            }

            /* Write new slot. */
            result = nvm_fee_slot_write(nvmfeep, nvmfeep->arena_active,
                    nvmfeep->arena_slots[nvmfeep->arena_active], &temp_slot);
            if (result != HAL_SUCCESS)
                return result;

            ++nvmfeep->arena_slots[nvmfeep->arena_active];
        }

        addr += n_slot;
        n_remaining -= n_slot;
    }

    /* Last (partial) slot */
    if (n_remaining)
    {
        /* Look for existing slot. */
        struct slot temp_slot;
        bool found;
        uint32_t slot;

        result = nvm_fee_slot_lookup(nvmfeep, arena, addr, &slot, &found);
        if (result != HAL_SUCCESS)
            return result;

        if (found == true)
        {
            /* Existing slot so read it. */
            result = nvm_fee_slot_read(nvmfeep, arena, slot, &temp_slot);
            if (result != HAL_SUCCESS)
                return result;
        }
        else
        {
            /* No existing slot so initialize a pristine one. */
            temp_slot.state_mark = nvm_fee_slot_state_2_mark(SLOT_STATE_VALID);
            temp_slot.address = addr;
            memset(temp_slot.payload, 0xff, sizeof(temp_slot.payload));
        }

        uint32_t n_slot = n_remaining;

        /* Compare slot data. */
        if (memcmp(temp_slot.payload, buffer + (addr - startaddr), n_slot) != 0)
        {
            /* Update slot data. */
            memcpy(temp_slot.payload, buffer + (addr - startaddr), n_slot);

            /* Check if arena is full and execute garbage collection. */
            if (nvmfeep->arena_slots[nvmfeep->arena_active] ==
                    nvmfeep->arena_num_slots)
            {
                result = nvm_fee_gc(nvmfeep, temp_slot.address);
                if (result != HAL_SUCCESS)
                    return result;
            }

            /* Write new slot. */
            result = nvm_fee_slot_write(nvmfeep, nvmfeep->arena_active,
                    nvmfeep->arena_slots[nvmfeep->arena_active], &temp_slot);
            if (result != HAL_SUCCESS)
                return result;

            ++nvmfeep->arena_slots[nvmfeep->arena_active];
        }

        addr += n_slot;
        n_remaining -= n_slot;
    }

    return HAL_SUCCESS;
}

static bool nvm_fee_write_pattern(NVMFeeDriver* nvmfeep, uint32_t arena,
        uint32_t startaddr, uint32_t n, uint8_t pattern)
{
    chDbgCheck((nvmfeep != NULL));

    const uint32_t first_slot_addr = startaddr -
            (startaddr % NVM_FEE_SLOT_PAYLOAD_SIZE);
    const uint32_t pre_pad = startaddr % NVM_FEE_SLOT_PAYLOAD_SIZE;

    bool result;
    uint32_t n_remaining = n;
    uint32_t addr = startaddr;

    /* First (partial) slot */
    if (pre_pad)
    {
        /* Look for existing slot. */
        struct slot temp_slot;
        bool found;
        uint32_t slot;

        result = nvm_fee_slot_lookup(nvmfeep, arena, first_slot_addr,
                &slot, &found);
        if (result != HAL_SUCCESS)
            return result;

        if (found == true)
        {
            /* Existing slot so read it. */
            result = nvm_fee_slot_read(nvmfeep, arena, slot, &temp_slot);
            if (result != HAL_SUCCESS)
                return result;
        }
        else
        {
            /* No existing slot so initialize a pristine one. */
            temp_slot.state_mark = nvm_fee_slot_state_2_mark(SLOT_STATE_VALID);
            temp_slot.address = first_slot_addr;
            memset(temp_slot.payload, 0xff, sizeof(temp_slot.payload));
        }

        uint32_t n_slot = NVM_FEE_SLOT_PAYLOAD_SIZE - pre_pad;
        if (n_slot > n_remaining)
            n_slot = n_remaining;

        /* Compare slot data. */
        if (memtst(temp_slot.payload + pre_pad, pattern, n_slot) != 0)
        {
            /* Update slot data. */
            memset(temp_slot.payload + pre_pad, pattern, n_slot);

            /* Check if arena is full and execute garbage collection. */
            if (nvmfeep->arena_slots[nvmfeep->arena_active] ==
                    nvmfeep->arena_num_slots)
            {
                result = nvm_fee_gc(nvmfeep, temp_slot.address);
                if (result != HAL_SUCCESS)
                    return result;
            }

            /* Write new slot. */
            result = nvm_fee_slot_write(nvmfeep, nvmfeep->arena_active,
                    nvmfeep->arena_slots[nvmfeep->arena_active], &temp_slot);
            if (result != HAL_SUCCESS)
                return result;

            ++nvmfeep->arena_slots[nvmfeep->arena_active];
        }

        addr += n_slot;
        n_remaining -= n_slot;
    }

    /* Full slots */
    while (n_remaining >= NVM_FEE_SLOT_PAYLOAD_SIZE)
    {
        /* Look for existing slot. */
        struct slot temp_slot;
        bool found;
        uint32_t slot;

        result = nvm_fee_slot_lookup(nvmfeep, arena, addr,
                &slot, &found);
        if (result != HAL_SUCCESS)
            return result;

        if (found == true)
        {
            /* Existing slot so read it. */
            result = nvm_fee_slot_read(nvmfeep, arena, slot, &temp_slot);
            if (result != HAL_SUCCESS)
                return result;
        }
        else
        {
            /* No existing slot so initialize a pristine one. */
            temp_slot.state_mark = nvm_fee_slot_state_2_mark(SLOT_STATE_VALID);
            temp_slot.address = addr;
            memset(temp_slot.payload, 0xff, sizeof(temp_slot.payload));
        }

        uint32_t n_slot = NVM_FEE_SLOT_PAYLOAD_SIZE;

        /* Compare slot data. */
        if (memtst(temp_slot.payload, pattern, n_slot) != 0)
        {
            /* Update slot data. */
            memset(temp_slot.payload, pattern, n_slot);

            /* Check if arena is full and execute garbage collection. */
            if (nvmfeep->arena_slots[nvmfeep->arena_active] ==
                    nvmfeep->arena_num_slots)
            {
                result = nvm_fee_gc(nvmfeep, temp_slot.address);
                if (result != HAL_SUCCESS)
                    return result;
            }

            /* Write new slot. */
            result = nvm_fee_slot_write(nvmfeep, nvmfeep->arena_active,
                    nvmfeep->arena_slots[nvmfeep->arena_active], &temp_slot);
            if (result != HAL_SUCCESS)
                return result;

            ++nvmfeep->arena_slots[nvmfeep->arena_active];
        }

        addr += n_slot;
        n_remaining -= n_slot;
    }

    /* Last (partial) slot */
    if (n_remaining)
    {
        /* Look for existing slot. */
        struct slot temp_slot;
        bool found;
        uint32_t slot;

        result = nvm_fee_slot_lookup(nvmfeep, arena, addr,
                &slot, &found);
        if (result != HAL_SUCCESS)
            return result;

        if (found == true)
        {
            /* Existing slot so read it. */
            result = nvm_fee_slot_read(nvmfeep, arena, slot, &temp_slot);
            if (result != HAL_SUCCESS)
                return result;
        }
        else
        {
            /* No existing slot so initialize a pristine one. */
            temp_slot.state_mark = nvm_fee_slot_state_2_mark(SLOT_STATE_VALID);
            temp_slot.address = addr;
            memset(temp_slot.payload, 0xff, sizeof(temp_slot.payload));
        }

        uint32_t n_slot = n_remaining;

        /* Compare slot data. */
        if (memtst(temp_slot.payload, pattern, n_slot) != 0)
        {
            /* Update slot data. */
            memset(temp_slot.payload, pattern, n_slot);

            /* Check if arena is full and execute garbage collection. */
            if (nvmfeep->arena_slots[nvmfeep->arena_active] ==
                    nvmfeep->arena_num_slots)
            {
                result = nvm_fee_gc(nvmfeep, temp_slot.address);
                if (result != HAL_SUCCESS)
                    return result;
            }

            /* Write new slot. */
            result = nvm_fee_slot_write(nvmfeep, nvmfeep->arena_active,
                    nvmfeep->arena_slots[nvmfeep->arena_active], &temp_slot);
            if (result != HAL_SUCCESS)
                return result;

            ++nvmfeep->arena_slots[nvmfeep->arena_active];
        }

        addr += n_slot;
        n_remaining -= n_slot;
    }

    return HAL_SUCCESS;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   NVM fee driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void nvmfeeInit(void)
{
}

/**
 * @brief   Initializes an instance.
 *
 * @param[out] nvmfeep      pointer to the @p NVMFeeDriver object
 *
 * @init
 */
void nvmfeeObjectInit(NVMFeeDriver* nvmfeep)
{
    nvmfeep->vmt = &nvm_fee_vmt;
    nvmfeep->state = NVM_STOP;
    nvmfeep->config = NULL;
#if NVM_FEE_USE_MUTUAL_EXCLUSION
#if CH_CFG_USE_MUTEXES
    chMtxInit(&nvmfeep->mutex);
#else
    chSemInit(&nvmfeep->semaphore, 1);
#endif
#endif /* NVM_JEDEC_SPI_USE_MUTUAL_EXCLUSION */
    nvmfeep->arena_active = 0;
    nvmfeep->arena_slots[0] = 0;
    nvmfeep->arena_slots[1] = 0;
}

/**
 * @brief   Configures and activates the NVM fee.
 *
 * @param[in] nvmfeep       pointer to the @p NVMFeeDriver object
 * @param[in] config        pointer to the @p NVMFeeConfig object.
 *
 * @api
 */
void nvmfeeStart(NVMFeeDriver* nvmfeep, const NVMFeeConfig* config)
{
    chDbgCheck((nvmfeep != NULL) && (config != NULL));
    /* Verify device status. */
    chDbgAssert((nvmfeep->state == NVM_STOP) || (nvmfeep->state == NVM_READY),
            "invalid state");

    nvmfeep->config = config;

    /* Calculate and cache often reused values. */
    nvmGetInfo(nvmfeep->config->nvmp, &nvmfeep->llnvmdi);

    nvmfeep->arena_num_sectors = nvmfeep->llnvmdi.sector_num / 2;
    nvmfeep->arena_num_slots =
            (nvmfeep->arena_num_sectors * nvmfeep->llnvmdi.sector_size
                    - sizeof(struct arena_header)) /
            sizeof(struct slot);
    nvmfeep->fee_size = nvmfeep->arena_num_slots * NVM_FEE_SLOT_PAYLOAD_SIZE;

    /* Check state and recover if necessary. */

    /* Examine active arena. */
    const enum arena_state states[2] =
    {
        [0] = nvm_fee_arena_state_get(nvmfeep, 0),
        [1] = nvm_fee_arena_state_get(nvmfeep, 1),
    };

    /* Possible states are:
     *
     * - active / unused
     * - unused / active
     * - frozen / any
     * - any / frozen
     * - anything else
     *
     */

    bool result;

    if (states[0] == ARENA_STATE_ACTIVE && states[1] == ARENA_STATE_UNUSED)
    {
        /* Load arena 0 as active arena. */
        nvmfeep->arena_active = 0;
        result = nvm_fee_arena_load(nvmfeep, 0);
        if (result != HAL_SUCCESS)
            goto out_error;
    }
    else if (states[0] == ARENA_STATE_UNUSED && states[1] == ARENA_STATE_ACTIVE)
    {
        /* Load arena 0 as active arena. */
        nvmfeep->arena_active = 1;
        result = nvm_fee_arena_load(nvmfeep, 1);
        if (result != HAL_SUCCESS)
            goto out_error;
    }
    else if (states[0] == ARENA_STATE_FROZEN)
    {
        /* Clear arena 1. */
        result = nvm_fee_arena_erase(nvmfeep, 1);
        if (result != HAL_SUCCESS)
            goto out_error;

        /* Load arena 0 as active arena. */
        nvmfeep->arena_active = 0;
        result = nvm_fee_arena_load(nvmfeep, 0);
        if (result != HAL_SUCCESS)
            goto out_error;

        /* Restart garbage collection without omitting any address. */
        result = nvm_fee_gc(nvmfeep, 0xffffffff);
        if (result != HAL_SUCCESS)
            goto out_error;
    }
    else if (states[1] == ARENA_STATE_FROZEN)
    {
        /* Clear arena 0. */
        result = nvm_fee_arena_erase(nvmfeep, 0);
        if (result != HAL_SUCCESS)
            goto out_error;

        /* Load arena 1 as active arena. */
        nvmfeep->arena_active = 1;
        result = nvm_fee_arena_load(nvmfeep, 1);
        if (result != HAL_SUCCESS)
            goto out_error;

        /* Restart garbage collection without omitting any address. */
        result = nvm_fee_gc(nvmfeep, 0xffffffff);
        if (result != HAL_SUCCESS)
            goto out_error;
    }
    else
    {
        /* Pristine or totally broken memory. */
        result = nvm_fee_arena_erase(nvmfeep, 0);
        if (result != HAL_SUCCESS)
            goto out_error;

        result = nvm_fee_arena_erase(nvmfeep, 1);
        if (result != HAL_SUCCESS)
            goto out_error;

        result = nvm_fee_arena_state_update(nvmfeep, 0, ARENA_STATE_ACTIVE);
        if (result != HAL_SUCCESS)
            goto out_error;

        nvmfeep->arena_active = 0;
        nvmfeep->arena_slots[0] = 0;
    }

    nvmfeep->state = NVM_READY;
    return;

out_error:
    nvmfeep->state = NVM_STOP;
    return;
}

/**
 * @brief   Disables the NVM fee.
 *
 * @param[in] nvmfeep       pointer to the @p NVMFeeDriver object
 *
 * @api
 */
void nvmfeeStop(NVMFeeDriver* nvmfeep)
{
    chDbgCheck(nvmfeep != NULL);
    /* Verify device status. */
    chDbgAssert((nvmfeep->state == NVM_STOP) || (nvmfeep->state == NVM_READY),
            "invalid state");

    nvmfeep->state = NVM_STOP;
}

/**
 * @brief   Reads data crossing sector boundaries if required.
 *
 * @param[in] nvmfeep       pointer to the @p NVMFeeDriver object
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
bool nvmfeeRead(NVMFeeDriver* nvmfeep, uint32_t startaddr,
        uint32_t n, uint8_t* buffer)
{
    chDbgCheck(nvmfeep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfeep->state >= NVM_READY, "invalid state");
    /* Verify range is within fee size. */
    chDbgAssert(startaddr + n <= nvmfeep->fee_size, "invalid parameters");

    /* Read operation in progress. */
    nvmfeep->state = NVM_READING;

    bool result = nvm_fee_read(nvmfeep, nvmfeep->arena_active, startaddr,
            n, buffer);
    if (result != HAL_SUCCESS)
        return result;

    /* Read operation finished. */
    nvmfeep->state = NVM_READY;

    return HAL_SUCCESS;
}

/**
 * @brief   Writes data crossing sector boundaries if required.
 *
 * @param[in] nvmfeep       pointer to the @p NVMFeeDriver object
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
bool nvmfeeWrite(NVMFeeDriver* nvmfeep, uint32_t startaddr,
        uint32_t n, const uint8_t* buffer)
{
    chDbgCheck(nvmfeep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfeep->state >= NVM_READY, "invalid state");
    /* Verify range is within fee size. */
    chDbgAssert(startaddr + n <= nvmfeep->fee_size, "invalid parameters");

    /* Write operation in progress. */
    nvmfeep->state = NVM_WRITING;

    bool result = nvm_fee_write(nvmfeep, nvmfeep->arena_active, startaddr,
            n, buffer);
    if (result != HAL_SUCCESS)
        return result;

    return HAL_SUCCESS;
}

/**
 * @brief   Erases one or more sectors.
 *
 * @param[in] nvmfeep       pointer to the @p NVMFeeDriver object
 * @param[in] startaddr     address within to be erased sector
 * @param[in] n             number of bytes to erase
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfeeErase(NVMFeeDriver* nvmfeep, uint32_t startaddr, uint32_t n)
{
    chDbgCheck(nvmfeep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfeep->state >= NVM_READY, "invalid state");
    /* Verify range is within fee size. */
    chDbgAssert(startaddr + n <= nvmfeep->fee_size, "invalid parameters");

    /* Erase operation in progress. */
    nvmfeep->state = NVM_ERASING;

    bool result = nvm_fee_write_pattern(nvmfeep, nvmfeep->arena_active,
            startaddr, n, 0xff);
    if (result != HAL_SUCCESS)
        return result;

    return HAL_SUCCESS;
}

/**
 * @brief   Erases all sectors.
 *
 * @param[in] nvmfeep       pointer to the @p NVMFeeDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfeeMassErase(NVMFeeDriver* nvmfeep)
{
    chDbgCheck(nvmfeep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfeep->state >= NVM_READY, "invalid state");

    /* Erase operation in progress. */
    nvmfeep->state = NVM_ERASING;

    bool result;

    result = nvm_fee_arena_erase(nvmfeep, 0);
    if (result != HAL_SUCCESS)
        return result;

    result = nvm_fee_arena_erase(nvmfeep, 1);
    if (result != HAL_SUCCESS)
        return result;

    result = nvm_fee_arena_state_update(nvmfeep, 0, ARENA_STATE_ACTIVE);
    if (result != HAL_SUCCESS)
        return result;

    nvmfeep->arena_active = 0;

    return HAL_SUCCESS;
}

/**
 * @brief   Waits for idle condition.
 *
 * @param[in] nvmfeep       pointer to the @p NVMFeeDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfeeSync(NVMFeeDriver* nvmfeep)
{
    chDbgCheck(nvmfeep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfeep->state >= NVM_READY, "invalid state");

    if (nvmfeep->state == NVM_READY)
        return HAL_SUCCESS;

    bool result = nvmSync(nvmfeep->config->nvmp);
    if (result != HAL_SUCCESS)
        return result;

    /* No more operation in progress. */
    nvmfeep->state = NVM_READY;

    return HAL_SUCCESS;
}

/**
 * @brief   Returns media info.
 *
 * @param[in] nvmfeep       pointer to the @p NVMFeeDriver object
 * @param[out] nvmdip       pointer to a @p NVMDeviceInfo structure
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfeeGetInfo(NVMFeeDriver* nvmfeep, NVMDeviceInfo* nvmdip)
{
    chDbgCheck(nvmfeep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfeep->state >= NVM_READY, "invalid state");

    nvmdip->sector_size = NVM_FEE_SLOT_PAYLOAD_SIZE;
    nvmdip->sector_num = nvmfeep->arena_num_slots;
    memcpy(nvmdip->identification, nvmfeep->llnvmdi.identification,
           sizeof(nvmdip->identification));
    /* Note: The virtual address room can be written byte wise */
    nvmdip->write_alignment = 0;

    return HAL_SUCCESS;
}

/**
 * @brief   Gains exclusive access to the nvm device.
 * @details This function tries to gain ownership to the nvm device, if the
 *          device is already being used then the invoking thread is queued.
 * @pre     In order to use this function the option
 *          @p NVM_FEE_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] nvmfeep       pointer to the @p NVMFeeDriver object
 *
 * @api
 */
void nvmfeeAcquireBus(NVMFeeDriver* nvmfeep)
{
    chDbgCheck(nvmfeep != NULL);

#if NVM_FEE_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES
    chMtxLock(&nvmfeep->mutex);
#elif CH_CFG_USE_SEMAPHORES
    chSemWait(&nvmfeep->semaphore);
#endif

    /* Lock the underlying device as well */
    nvmAcquire(nvmfeep->config->nvmp);
#endif /* NVM_FEE_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Releases exclusive access to the nvm device.
 * @pre     In order to use this function the option
 *          @p NVM_FEE_USE_MUTUAL_EXCLUSION must be enabled.
 *
 * @param[in] nvmfeep       pointer to the @p NVMFeeDriver object
 *
 * @api
 */
void nvmfeeReleaseBus(NVMFeeDriver* nvmfeep)
{
    chDbgCheck(nvmfeep != NULL);

#if NVM_FEE_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES
    chMtxUnlock();
#elif CH_CFG_USE_SEMAPHORES
    chSemSignal(&nvmfeep->semaphore);
#endif

    /* Release the underlying device as well */
    nvmRelease(nvmfeep->config->nvmp);
#endif /* NVM_FEE_USE_MUTUAL_EXCLUSION */
}

/**
 * @brief   Write protects one or more sectors.
 *
 * @param[in] nvmfeep       pointer to the @p NVMFeeDriver object
 * @param[in] startaddr     address within to be protected sector
 * @param[in] n             number of bytes to protect
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfeeWriteProtect(NVMFeeDriver* nvmfeep,
        uint32_t startaddr, uint32_t n)
{
    chDbgCheck(nvmfeep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfeep->state >= NVM_READY, "invalid state");

    /* TODO: add implementation */

    return HAL_SUCCESS;
}

/**
 * @brief   Write protects the whole device.
 *
 * @param[in] nvmfeep       pointer to the @p NVMFeeDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfeeMassWriteProtect(NVMFeeDriver* nvmfeep)
{
    chDbgCheck(nvmfeep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfeep->state >= NVM_READY, "invalid state");

    /* TODO: add implementation */

    return HAL_SUCCESS;
}

/**
 * @brief   Write unprotects one or more sectors.
 *
 * @param[in] nvmfeep       pointer to the @p NVMFeeDriver object
 * @param[in] startaddr     address within to be unprotected sector
 * @param[in] n             number of bytes to unprotect
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfeeWriteUnprotect(NVMFeeDriver* nvmfeep,
        uint32_t startaddr, uint32_t n)
{
    chDbgCheck(nvmfeep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfeep->state >= NVM_READY, "invalid state");

    /* TODO: add implementation */

    return HAL_SUCCESS;
}

/**
 * @brief   Write unprotects the whole device.
 *
 * @param[in] nvmfeep       pointer to the @p NVMFeeDriver object
 *
 * @return                  The operation status.
 * @retval HAL_SUCCESS      the operation succeeded.
 * @retval HAL_FAILED       the operation failed.
 *
 * @api
 */
bool nvmfeeMassWriteUnprotect(NVMFeeDriver* nvmfeep)
{
    chDbgCheck(nvmfeep != NULL);
    /* Verify device status. */
    chDbgAssert(nvmfeep->state >= NVM_READY, "invalid state");

    /* TODO: add implementation */

    return HAL_SUCCESS;
}

#endif /* HAL_USE_NVM_FEE */

/** @} */
