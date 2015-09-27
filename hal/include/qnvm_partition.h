/**
 * @file    qnvm_partition.h
 * @brief   NVM partition driver header.
 *
 * @addtogroup NVM_PARTITION
 * @{
 */

#ifndef _QNVM_PARTITION_H_
#define _QNVM_PARTITION_H_

#if HAL_USE_NVM_PARTITION || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    NVM_PARTITION configuration options
 * @{
 */
/**
 * @brief   Enables the @p nvmpartAcquireBus() and @p nvmpartReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(NVM_PARTITION_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define NVM_PARTITION_USE_MUTUAL_EXCLUSION     TRUE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Non volatile memory partition driver configuration structure.
 */
typedef struct
{
    /**
    * @brief NVM driver associated to this partition.
    */
    BaseNVMDevice* nvmp;
    /**
     * @brief Offset in sectors.
     */
    uint32_t sector_offset;
    /**
     * @brief Total number of sectors.
     */
    uint32_t sector_num;
} NVMPartitionConfig;

/**
 * @brief   @p NVMPartitionDriver specific methods.
 */
#define _nvm_partition_driver_methods                                         \
    _base_nvm_device_methods

/**
 * @extends BaseNVMDeviceVMT
 *
 * @brief   @p NVMPartitionDriver virtual methods table.
 */
struct NVMPartitionDriverVMT
{
    _nvm_partition_driver_methods
};

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a NVM partition driver.
 */
typedef struct
{
    /**
    * @brief Virtual Methods Table.
    */
    const struct NVMPartitionDriverVMT* vmt;
    _base_nvm_device_data
    /**
    * @brief Current configuration data.
    */
    const NVMPartitionConfig* config;
    /**
    * @brief Device info of underlying nvm device.
    */
    NVMDeviceInfo llnvmdi;
    /**
    * @brief Partition origin relative to llnvm.
    */
    uint32_t part_org;
    /**
    * @brief Partition size in bytes.
    */
    uint32_t part_size;
#if NVM_PARTITION_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
    /**
     * @brief mutex_t protecting the device.
     */
    mutex_t mutex;
#endif /* NVM_PARTITION_USE_MUTUAL_EXCLUSION */
} NVMPartitionDriver;

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
    void nvmpartInit(void);
    void nvmpartObjectInit(NVMPartitionDriver* nvmpartp);
    void nvmpartStart(NVMPartitionDriver* nvmpartp,
            const NVMPartitionConfig* config);
    void nvmpartStop(NVMPartitionDriver* nvmpartp);
    bool nvmpartRead(NVMPartitionDriver* nvmpartp, uint32_t startaddr,
            uint32_t n, uint8_t* buffer);
    bool nvmpartWrite(NVMPartitionDriver* nvmpartp, uint32_t startaddr,
            uint32_t n, const uint8_t* buffer);
    bool nvmpartErase(NVMPartitionDriver* nvmpartp, uint32_t startaddr,
            uint32_t n);
    bool nvmpartMassErase(NVMPartitionDriver* nvmpartp);
    bool nvmpartSync(NVMPartitionDriver* nvmpartp);
    bool nvmpartGetInfo(NVMPartitionDriver* nvmpartp, NVMDeviceInfo* nvmdip);
    void nvmpartAcquireBus(NVMPartitionDriver* nvmpartp);
    void nvmpartReleaseBus(NVMPartitionDriver* nvmpartp);
    bool nvmpartWriteProtect(NVMPartitionDriver* nvmpartp,
            uint32_t startaddr, uint32_t n);
    bool nvmpartMassWriteProtect(NVMPartitionDriver* nvmpartp);
    bool nvmpartWriteUnprotect(NVMPartitionDriver* nvmpartp,
            uint32_t startaddr, uint32_t n);
    bool nvmpartMassWriteUnprotect(NVMPartitionDriver* nvmpartp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_NVM_PARTITION */

#endif /* _QNVM_PARTITION_H_ */

/** @} */
