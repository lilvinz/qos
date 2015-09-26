/**
 * @file    qnvm_file.h
 * @brief   NVM emulation through binary file.
 *
 * @addtogroup NVM_FILE
 * @{
 */

#ifndef _QNVM_FILE_H_
#define _QNVM_FILE_H_

#if HAL_USE_NVM_FILE || defined(__DOXYGEN__)

#include <stdio.h>

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    NVM_FILE configuration options
 * @{
 */
/**
 * @brief   Enables the @p nvmfileAcquireBus() and @p nvmfileReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(NVM_FILE_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define NVM_FILE_USE_MUTUAL_EXCLUSION       TRUE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if NVM_FILE_USE_MUTUAL_EXCLUSION && (!CH_CFG_USE_MUTEXES && !CH_CFG_USE_SEMAPHORES)
#error "NVM_FILE_USE_MUTUAL_EXCLUSION requires CH_CFG_USE_MUTEXES and/or "
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
    * @brief Name of the underlying binary file.
    */
    const char* file_name;
    /**
     * @brief Smallest erasable sector size in bytes.
     */
    uint32_t sector_size;
    /**
     * @brief Total number of sectors.
     */
    uint32_t sector_num;
} NVMFileConfig;

/**
 * @brief   @p NVMFileDriver specific methods.
 */
#define _nvm_file_driver_methods                                              \
    _base_nvm_device_methods

/**
 * @extends BaseNVMDeviceVMT
 *
 * @brief   @p NVMFileDriver virtual methods table.
 */
struct NVMFileDriverVMT
{
    _nvm_file_driver_methods
};

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a NVM file driver.
 */
typedef struct
{
    /**
    * @brief Virtual Methods Table.
    */
    const struct NVMFileDriverVMT* vmt;
    _base_nvm_device_data
    /**
    * @brief Current configuration data.
    */
    const NVMFileConfig* config;
    /**
     * @brief Current configuration data.
    */
    FILE* file;
#if NVM_FILE_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief Mutex protecting the device.
     */
    Mutex mutex;
#elif CH_CFG_USE_SEMAPHORES
    Semaphore semaphore;
#endif
#endif /* NVM_FILE_USE_MUTUAL_EXCLUSION */
} NVMFileDriver;

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
    void nvmfileInit(void);
    void nvmfileObjectInit(NVMFileDriver* nvmfilep);
    void nvmfileStart(NVMFileDriver* nvmfilep, const NVMFileConfig* config);
    void nvmfileStop(NVMFileDriver* nvmfilep);
    bool_t nvmfileRead(NVMFileDriver* nvmfilep, uint32_t startaddr,
            uint32_t n, uint8_t* buffer);
    bool_t nvmfileWrite(NVMFileDriver* nvmfilep, uint32_t startaddr,
            uint32_t n, const uint8_t* buffer);
    bool_t nvmfileErase(NVMFileDriver* nvmfilep, uint32_t startaddr,
            uint32_t n);
    bool_t nvmfileMassErase(NVMFileDriver* nvmfilep);
    bool_t nvmfileSync(NVMFileDriver* nvmfilep);
    bool_t nvmfileGetInfo(NVMFileDriver* nvmfilep, NVMDeviceInfo* nvmdip);
    void nvmfileAcquireBus(NVMFileDriver* nvmfilep);
    void nvmfileReleaseBus(NVMFileDriver* nvmfilep);
    bool_t nvmfileWriteProtect(NVMFileDriver* nvmfilep,
            uint32_t startaddr, uint32_t n);
    bool_t nvmfileMassWriteProtect(NVMFileDriver* nvmfilep);
    bool_t nvmfileWriteUnprotect(NVMFileDriver* nvmfilep,
            uint32_t startaddr, uint32_t n);
    bool_t nvmfileMassWriteUnprotect(NVMFileDriver* nvmfilep);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_NVM_FILE */

#endif /* _QNVM_FILE_H_ */

/** @} */
