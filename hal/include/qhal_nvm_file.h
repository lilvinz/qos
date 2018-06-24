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

#if HAS_FATFS
#include "ff.h"
#endif /* HAS_FATFS */

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
#if HAS_FATFS
    FIL file;
#else /* HAS_FATFS */
    FILE* file;
#endif /* HAS_FATFS */
#if NVM_FILE_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
    /**
     * @brief mutex_t protecting the device.
     */
    mutex_t mutex;
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
    bool nvmfileRead(NVMFileDriver* nvmfilep, uint32_t startaddr,
            uint32_t n, uint8_t* buffer);
    bool nvmfileWrite(NVMFileDriver* nvmfilep, uint32_t startaddr,
            uint32_t n, const uint8_t* buffer);
    bool nvmfileErase(NVMFileDriver* nvmfilep, uint32_t startaddr,
            uint32_t n);
    bool nvmfileMassErase(NVMFileDriver* nvmfilep);
    bool nvmfileSync(NVMFileDriver* nvmfilep);
    bool nvmfileGetInfo(NVMFileDriver* nvmfilep, NVMDeviceInfo* nvmdip);
    void nvmfileAcquireBus(NVMFileDriver* nvmfilep);
    void nvmfileReleaseBus(NVMFileDriver* nvmfilep);
    bool nvmfileWriteProtect(NVMFileDriver* nvmfilep,
            uint32_t startaddr, uint32_t n);
    bool nvmfileMassWriteProtect(NVMFileDriver* nvmfilep);
    bool nvmfileWriteUnprotect(NVMFileDriver* nvmfilep,
            uint32_t startaddr, uint32_t n);
    bool nvmfileMassWriteUnprotect(NVMFileDriver* nvmfilep);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_NVM_FILE */

#endif /* _QNVM_FILE_H_ */

/** @} */
