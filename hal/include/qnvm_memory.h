/**
 * @file    qnvm_memory.h
 * @brief   NVM emulation through memory block.
 *
 * @addtogroup NVM_MEMORY
 * @{
 */

#ifndef _QNVM_MEMORY_H_
#define _QNVM_MEMORY_H_

#if HAL_USE_NVM_MEMORY || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    NVM_MEMORY configuration options
 * @{
 */
/**
 * @brief   Enables the @p nvmmemoryAcquireBus() and
 *          @p nvmmemoryReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(NVM_MEMORY_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define NVM_MEMORY_USE_MUTUAL_EXCLUSION       TRUE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if NVM_MEMORY_USE_MUTUAL_EXCLUSION && (!CH_CFG_USE_MUTEXES && !CH_CFG_USE_SEMAPHORES)
#error "NVM_MEMORY_USE_MUTUAL_EXCLUSION requires CH_CFG_USE_MUTEXES and/or "
       "CH_CFG_USE_SEMAPHORES"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Non volatile memory emulation driver configuration structure.
 */
typedef struct
{
    /**
    * @brief Pointer to memory block.
    */
    uint8_t* memoryp;
    /**
     * @brief Smallest erasable sector size in bytes.
     */
    uint32_t sector_size;
    /**
     * @brief Total number of sectors.
     */
    uint32_t sector_num;
} NVMMemoryConfig;

/**
 * @brief   @p NVMMemoryDriver specific methods.
 */
#define _nvm_memory_driver_methods                                            \
    _base_nvm_device_methods

/**
 * @extends BaseNVMDeviceVMT
 *
 * @brief   @p NVMMemoryDriver virtual methods table.
 */
struct NVMMemoryDriverVMT
{
    _nvm_memory_driver_methods
};

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a NVM memory driver.
 */
typedef struct
{
    /**
    * @brief Virtual Methods Table.
    */
    const struct NVMMemoryDriverVMT* vmt;
    _base_nvm_device_data
    /**
    * @brief Current configuration data.
    */
    const NVMMemoryConfig* config;
#if NVM_MEMORY_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief mutex_t protecting the device.
     */
    mutex_t mutex;
#elif CH_CFG_USE_SEMAPHORES
    Semaphore semaphore;
#endif
#endif /* NVM_MEMORY_USE_MUTUAL_EXCLUSION */
} NVMMemoryDriver;

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
    void nvmmemoryInit(void);
    void nvmmemoryObjectInit(NVMMemoryDriver* nvmmemoryp);
    void nvmmemoryStart(NVMMemoryDriver* nvmmemoryp,
            const NVMMemoryConfig* config);
    void nvmmemoryStop(NVMMemoryDriver* nvmmemoryp);
    bool_t nvmmemoryRead(NVMMemoryDriver* nvmmemoryp, uint32_t startaddr,
            uint32_t n, uint8_t* buffer);
    bool_t nvmmemoryWrite(NVMMemoryDriver* nvmmemoryp, uint32_t startaddr,
            uint32_t n, const uint8_t* buffer);
    bool_t nvmmemoryErase(NVMMemoryDriver* nvmmemoryp, uint32_t startaddr,
            uint32_t n);
    bool_t nvmmemoryMassErase(NVMMemoryDriver* nvmmemoryp);
    bool_t nvmmemorySync(NVMMemoryDriver* nvmmemoryp);
    bool_t nvmmemoryGetInfo(NVMMemoryDriver* nvmmemoryp,
            NVMDeviceInfo* nvmdip);
    void nvmmemoryAcquireBus(NVMMemoryDriver* nvmmemoryp);
    void nvmmemoryReleaseBus(NVMMemoryDriver* nvmmemoryp);
    bool_t nvmmemoryWriteProtect(NVMMemoryDriver* nvmmemoryp,
            uint32_t startaddr, uint32_t n);
    bool_t nvmmemoryMassWriteProtect(NVMMemoryDriver* nvmmemoryp);
    bool_t nvmmemoryWriteUnprotect(NVMMemoryDriver* nvmmemoryp,
            uint32_t startaddr, uint32_t n);
    bool_t nvmmemoryMassWriteUnprotect(NVMMemoryDriver* nvmmemoryp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_NVM_MEMORY */

#endif /* _QNVM_MEMORY_H_ */

/** @} */
