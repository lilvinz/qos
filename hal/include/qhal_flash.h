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
 * @file    qflash.h
 * @brief   MCU internal flash header.
 *
 * @addtogroup FLASH
 * @{
 */

#ifndef _QFLASH_H_
#define _QFLASH_H_

#if HAL_USE_FLASH || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    FLASH configuration options
 * @{
 */
/**
 * @brief   Delays insertions.
 * @details If enabled this options inserts delays into the flash waiting
 *          routines releasing some extra CPU time for the threads with
 *          lower priority, this may slow down the driver a bit however.
 * @note    This does only make sense if code is being executed from RAM.
 */
#if !defined(FLASH_NICE_WAITING) || defined(__DOXYGEN__)
#define FLASH_NICE_WAITING                      FALSE
#endif

/**
 * @brief   Enables the @p flashAcquireBus() and @p flashReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(FLASH_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define FLASH_USE_MUTUAL_EXCLUSION              TRUE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

#include "qhal_flash_lld.h"

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
    void flashInit(void);
    void flashObjectInit(FLASHDriver* flashp);
    void flashStart(FLASHDriver* flashp, const FLASHConfig* config);
    void flashStop(FLASHDriver* flashp);
    bool flashRead(FLASHDriver* flashp, uint32_t startaddr, uint32_t n,
            uint8_t* buffer);
    bool flashWrite(FLASHDriver* flashp, uint32_t startaddr, uint32_t n,
            const uint8_t* buffer);
    bool flashErase(FLASHDriver* flashp, uint32_t startaddr, uint32_t n);
    bool flashMassErase(FLASHDriver* flashp);
    bool flashSync(FLASHDriver* flashp);
    bool flashGetInfo(FLASHDriver* flashp, NVMDeviceInfo* nvmdip);
    void flashAcquireBus(FLASHDriver* flashp);
    void flashReleaseBus(FLASHDriver* flashp);
    bool flashWriteProtect(FLASHDriver* flashp, uint32_t startaddr,
            uint32_t n);
    bool flashMassWriteProtect(FLASHDriver* flashp);
    bool flashWriteUnprotect(FLASHDriver* flashp, uint32_t startaddr,
            uint32_t n);
    bool flashMassWriteUnprotect(FLASHDriver* flashp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_FLASH */

#endif /* _QFLASH_H_ */

/** @} */
