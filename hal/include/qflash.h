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

#if FLASH_USE_MUTUAL_EXCLUSION && !CH_CFG_USE_MUTEXES && !CH_CFG_USE_SEMAPHORES
#error "FLASH_USE_MUTUAL_EXCLUSION requires CH_CFG_USE_MUTEXES and/or "
       "CH_CFG_USE_SEMAPHORES"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

#include "qflash_lld.h"

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
    bool_t flashRead(FLASHDriver* flashp, uint32_t startaddr, uint32_t n,
            uint8_t* buffer);
    bool_t flashWrite(FLASHDriver* flashp, uint32_t startaddr, uint32_t n,
            const uint8_t* buffer);
    bool_t flashErase(FLASHDriver* flashp, uint32_t startaddr, uint32_t n);
    bool_t flashMassErase(FLASHDriver* flashp);
    bool_t flashSync(FLASHDriver* flashp);
    bool_t flashGetInfo(FLASHDriver* flashp, NVMDeviceInfo* nvmdip);
    void flashAcquireBus(FLASHDriver* flashp);
    void flashReleaseBus(FLASHDriver* flashp);
    bool_t flashWriteProtect(FLASHDriver* flashp, uint32_t startaddr,
            uint32_t n);
    bool_t flashMassWriteProtect(FLASHDriver* flashp);
    bool_t flashWriteUnprotect(FLASHDriver* flashp, uint32_t startaddr,
            uint32_t n);
    bool_t flashMassWriteUnprotect(FLASHDriver* flashp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_FLASH */

#endif /* _QFLASH_H_ */

/** @} */
