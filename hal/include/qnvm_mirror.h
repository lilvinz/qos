/**
 * @file    qnvm_mirror.h
 * @brief   NVM mirror driver header.
 *
 * @addtogroup NVM_MIRROR
 * @{
 */

#ifndef _QNVM_MIRROR_H_
#define _QNVM_MIRROR_H_

#if HAL_USE_NVM_MIRROR || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    NVM_MIRROR configuration options
 * @{
 */
/**
 * @brief   Enables the @p nvmmirrorAcquireBus() and @p nvmmirrorReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(NVM_MIRROR_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define NVM_MIRROR_USE_MUTUAL_EXCLUSION     TRUE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if NVM_MIRROR_USE_MUTUAL_EXCLUSION && (!CH_USE_MUTEXES && !CH_USE_SEMAPHORES)
#error "NVM_MIRROR_USE_MUTUAL_EXCLUSION requires CH_USE_MUTEXES and/or "
       "CH_USE_SEMAPHORES"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   NVM mirror driver configuration structure.
 */
typedef struct
{
    /**
    * @brief NVM driver associated to this mirror.
    */
    BaseNVMDevice* nvmp;
    /*
     * @brief number of sectors to assign to metadata header
     */
    uint32_t sector_header_num;
} NVMMirrorConfig;

/**
 * @brief   @p NVMMirrorDriver specific methods.
 */
#define _nvm_mirror_driver_methods                                            \
    _base_nvm_device_methods

/**
 * @extends BaseNVMDeviceVMT
 *
 * @brief   @p NVMMirrorDriver virtual methods table.
 */
struct NVMMirrorDriverVMT
{
    _nvm_mirror_driver_methods
};

/**
 * @brief   Enum representing the internal state of the mirror
 */
typedef enum
{
    STATE_INVALID = 0,
    STATE_DIRTY_A,
    STATE_DIRTY_B,
    STATE_SYNCED,
    STATE_COUNT,
} NVMMirrorState;

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a NVM mirror driver.
 */
typedef struct
{
    /**
    * @brief Virtual Methods Table.
    */
    const struct NVMMirrorDriverVMT* vmt;
    _base_nvm_device_data
    /**
    * @brief Current configuration data.
    */
    const NVMMirrorConfig* config;
    /**
    * @brief Device info of underlying nvm device.
    */
    NVMDeviceInfo llnvmdi;
    /**
    * @brief Current state of the mirror.
    */
    NVMMirrorState mirror_state;
    /**
    * @brief Address of the current used state mark.
    */
    uint32_t mirror_state_addr;
    /**
    * @brief Mirror size cached for performance.
    */
    uint32_t mirror_size;
    /**
    * @brief Origin address of mirror a cached for performance.
    */
    uint32_t mirror_a_org;
    /**
    * @brief Origin address of mirror b cached for performance.
    */
    uint32_t mirror_b_org;
#if NVM_MIRROR_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief Mutex protecting the device.
     */
    Mutex mutex;
#elif CH_USE_SEMAPHORES
    Semaphore semaphore;
#endif
#endif /* NVM_MIRROR_USE_MUTUAL_EXCLUSION */
} NVMMirrorDriver;

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
    void nvmmirrorInit(void);
    void nvmmirrorObjectInit(NVMMirrorDriver* nvmmirrorp);
    void nvmmirrorStart(NVMMirrorDriver* nvmmirrorp,
            const NVMMirrorConfig* config);
    void nvmmirrorStop(NVMMirrorDriver* nvmmirrorp);
    bool_t nvmmirrorRead(NVMMirrorDriver* nvmmirrorp, uint32_t startaddr,
            uint32_t n, uint8_t* buffer);
    bool_t nvmmirrorWrite(NVMMirrorDriver* nvmmirrorp, uint32_t startaddr,
            uint32_t n, const uint8_t* buffer);
    bool_t nvmmirrorErase(NVMMirrorDriver* nvmmirrorp, uint32_t startaddr,
            uint32_t n);
    bool_t nvmmirrorMassErase(NVMMirrorDriver* nvmmirrorp);
    bool_t nvmmirrorSync(NVMMirrorDriver* nvmmirrorp);
    bool_t nvmmirrorGetInfo(NVMMirrorDriver* nvmmirrorp,
            NVMDeviceInfo* nvmdip);
    void nvmmirrorAcquireBus(NVMMirrorDriver* nvmmirrorp);
    void nvmmirrorReleaseBus(NVMMirrorDriver* nvmmirrorp);
    bool_t nvmmirrorWriteProtect(NVMMirrorDriver* nvmmirrorp,
            uint32_t startaddr, uint32_t n);
    bool_t nvmmirrorMassWriteProtect(NVMMirrorDriver* nvmmirrorp);
    bool_t nvmmirrorWriteUnprotect(NVMMirrorDriver* nvmmirrorp,
            uint32_t startaddr, uint32_t n);
    bool_t nvmmirrorMassWriteUnprotect(NVMMirrorDriver* nvmmirrorp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_NVM_MIRROR */

#endif /* _QNVM_MIRROR_H_ */

/** @} */
