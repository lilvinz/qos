/**
 * @file    qnvm_fee.h
 * @brief   NVM fee driver header.
 *
 * @addtogroup NVM_FEE
 * @{
 */

#ifndef _QNVM_FEE_H_
#define _QNVM_FEE_H_

#if HAL_USE_NVM_FEE || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    NVM_FEE configuration options
 * @{
 */

/**
 * @brief   Enables the @p nvmfeeAcquireBus() and @p nvmfeeReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(NVM_FEE_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define NVM_FEE_USE_MUTUAL_EXCLUSION    TRUE
#endif

/**
 * @brief   Sets the number of payload bytes per slot.
 */
#if !defined(NVM_FEE_SLOT_PAYLOAD_SIZE) || defined(__DOXYGEN__)
#define NVM_FEE_SLOT_PAYLOAD_SIZE       8
#endif

/**
 * @brief   Sets the smallest writable unit of the underlying flash device.
 */
#if !defined(NVM_FEE_WRITE_UNIT_SIZE) || defined(__DOXYGEN__)
#define NVM_FEE_WRITE_UNIT_SIZE         2
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (4 + NVM_FEE_SLOT_PAYLOAD_SIZE) % NVM_FEE_WRITE_UNIT_SIZE
#error "payload size + 4 must be a multiple of write unit size."
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   NVM fee driver configuration structure.
 */
typedef struct
{
    /**
    * @brief NVM driver associated to this fee.
    */
    BaseNVMDevice* nvmp;
    /*
     * @brief number of sectors to assign to metadata header
     */
    uint32_t sector_header_num;
} NVMFeeConfig;

/**
 * @brief   @p NVMFeeDriver specific methods.
 */
#define _nvm_fee_driver_methods                                               \
    _base_nvm_device_methods

/**
 * @extends BaseNVMDeviceVMT
 *
 * @brief   @p NVMFeeDriver virtual methods table.
 */
struct NVMFeeDriverVMT
{
    _nvm_fee_driver_methods
};

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a NVM fee driver.
 */
typedef struct
{
    /**
    * @brief Virtual Methods Table.
    */
    const struct NVMFeeDriverVMT* vmt;
    _base_nvm_device_data
    /**
    * @brief Current configuration data.
    */
    const NVMFeeConfig* config;
    /**
    * @brief Device info of underlying nvm device.
    */
    NVMDeviceInfo llnvmdi;
    /**
    * @brief Current active arena.
    */
    uint32_t arena_active;
    /**
     * @brief Used slots in arena.
     */
    uint32_t arena_slots[2];
    /**
    * @brief Cached values.
    */
    uint32_t arena_num_sectors;
    uint32_t arena_num_slots;
    uint32_t fee_size;
#if NVM_FEE_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
    /**
     * @brief mutex_t protecting the device.
     */
    mutex_t mutex;
#endif /* NVM_FEE_USE_MUTUAL_EXCLUSION */
} NVMFeeDriver;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
    void nvmfeeInit(void);
    void nvmfeeObjectInit(NVMFeeDriver* nvmfeep);
    void nvmfeeStart(NVMFeeDriver* nvmfeep,
            const NVMFeeConfig* config);
    void nvmfeeStop(NVMFeeDriver* nvmfeep);
    bool nvmfeeRead(NVMFeeDriver* nvmfeep, uint32_t startaddr,
            uint32_t n, uint8_t* buffer);
    bool nvmfeeWrite(NVMFeeDriver* nvmfeep, uint32_t startaddr,
            uint32_t n, const uint8_t* buffer);
    bool nvmfeeErase(NVMFeeDriver* nvmfeep, uint32_t startaddr,
            uint32_t n);
    bool nvmfeeMassErase(NVMFeeDriver* nvmfeep);
    bool nvmfeeSync(NVMFeeDriver* nvmfeep);
    bool nvmfeeGetInfo(NVMFeeDriver* nvmfeep,
            NVMDeviceInfo* nvmdip);
    void nvmfeeAcquireBus(NVMFeeDriver* nvmfeep);
    void nvmfeeReleaseBus(NVMFeeDriver* nvmfeep);
    bool nvmfeeWriteProtect(NVMFeeDriver* nvmfeep,
            uint32_t startaddr, uint32_t n);
    bool nvmfeeMassWriteProtect(NVMFeeDriver* nvmfeep);
    bool nvmfeeWriteUnprotect(NVMFeeDriver* nvmfeep,
            uint32_t startaddr, uint32_t n);
    bool nvmfeeMassWriteUnprotect(NVMFeeDriver* nvmfeep);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_NVM_FEE */

#endif /* _QNVM_FEE_H_ */

/** @} */
