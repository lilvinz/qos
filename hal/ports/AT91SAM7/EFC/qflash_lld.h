/**
 * @file    AT91SAM7/qflash_lld.c
 * @brief   AT91SAM7 low level FLASH driver header.
 *
 * @addtogroup FLASH
 * @{
 */

#ifndef _QFLASH_LLD_H_
#define _QFLASH_LLD_H_

#if HAL_USE_FLASH || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Check for supported devices. */
#if (SAM7_PLATFORM == SAM7X128) ||              \
        (SAM7_PLATFORM == SAM7X256)
#else
#error "device unsupported by EFC driver"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/* Define efc interface if not included in chip header. */
#if !defined(AT91C_BASE_EFC)
typedef struct _AT91S_EFC
{
    AT91_REG EFC_FMR;   /* MC Flash Mode Register */
    AT91_REG EFC_FCR;   /* MC Flash Command Register */
    AT91_REG EFC_FSR;   /* MC Flash Status Register */
    AT91_REG EFC_VR;    /* MC Flash Version Register */
} AT91S_EFC;

#define AT91C_BASE_EFC       (AT91_CAST(AT91S_EFC*)0xFFFFFF60)
#endif

/**
 * @brief   Flash internal driver configuration structure.
 */
typedef struct
{
} FLASHConfig;

/**
 * @brief   @p FLASHDriver specific methods.
 */
#define _flash_driver_methods                                                 \
    _base_nvm_device_methods

/**
 * @extends BaseNVMDeviceVMT
 *
 * @brief   @p FLASHDriver virtual methods table.
 */
struct FLASHDriverVMT
{
    _flash_driver_methods
};

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a FLASH driver.
 */
typedef struct
{
    /**
    * @brief Virtual Methods Table.
    */
    const struct FLASHDriverVMT* vmt;
    _base_nvm_device_data
    /**
    * @brief Current configuration data.
    */
    const FLASHConfig* config;
    /**
     * @brief Pointer to the FLASH registers block.
     */
    AT91S_EFC* flash;
#if FLASH_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief mutex_t protecting the device.
     */
    mutex_t mutex;
#elif CH_CFG_USE_SEMAPHORES
    Semaphore semaphore;
#endif
#endif /* FLASH_USE_MUTUAL_EXCLUSION */
} FLASHDriver;

typedef struct
{
    uint32_t sector;
    uint32_t origin;
    uint32_t size;
    uint8_t lock_bit;
} FLASHSectorInfo;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

extern FLASHDriver FLASHD;

#ifdef __cplusplus
extern "C" {
#endif
    void flash_lld_init(void);
    void flash_lld_start(FLASHDriver* flashp);
    void flash_lld_stop(FLASHDriver* flashp);
    bool flash_lld_addr_to_sector(uint32_t addr, FLASHSectorInfo* sinfo);
    void flash_lld_read(FLASHDriver* flashp, uint32_t startaddr, uint32_t n,
            uint8_t* buffer);
    void flash_lld_write(FLASHDriver* flashp, uint32_t startaddr, uint32_t n,
            const uint8_t* buffer);
    void flash_lld_erase_sector(FLASHDriver* flashp, uint32_t startaddr);
    void flash_lld_erase_mass(FLASHDriver* flashp);
    void flash_lld_sync(FLASHDriver* flashp);
    void flash_lld_get_info(FLASHDriver* flashp, NVMDeviceInfo* nvmdip);
    void flash_lld_writeprotect_sector(FLASHDriver* flashp,
            uint32_t startaddr);
    void flash_lld_writeprotect_mass(FLASHDriver* flashp);
    void flash_lld_writeunprotect_sector(FLASHDriver* flashp,
            uint32_t startaddr);
    void flash_lld_writeunprotect_mass(FLASHDriver* flashp);
    void flash_lld_gpnvm(FLASHDriver* flashp, uint8_t newbits);
    void flash_lld_security(FLASHDriver* flashp, bool newstate);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_FLASH */

#endif /* _QFLASH_LLD_H_ */

/** @} */
