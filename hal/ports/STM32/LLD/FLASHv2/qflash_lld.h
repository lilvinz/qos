/**
 * @file    STM32/FLASHv2/qflash.c
 * @brief   STM32 low level FLASH driver header.
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

/**
 * @brief   FLASH readout protection options
 *
 *          LEVEL 0: no read protection
 *          When the read protection level is set to LEVEL 0 by writing 0xAA into the read protection
 *          option byte (RDP), all read/write operations (if no write protection is set) from/to the
 *          Flash memory or the backup SRAM are possible in all boot configurations (Flash user
 *          boot, debug or boot from RAM).
 *
 *          LEVEL 1: memory read protection.
 *          It is the default read protection level after option byte erase. The read protection LEVEL 1
 *          is activated by writing any value (except for 0xAA and 0xCC used to set LEVEL 0 and
 *          LEVEL 2, respectively) into the RDP option byte. When the read protection LEVEL 1 is set:
 *
 *          - No Flash memory access (read, erase, program) is performed while the debug
 *            features are connected or boot is executed from RAM. A bus error is generated in
 *            case of a Flash memory read request. Otherwise all operations are possible when
 *            Flash user boot is used or when operating in System memory boot mode.
 *
 *          - When LEVEL 1 is active, programming the protection option byte (RDP) to LEVEL 0
 *            causes the Flash memory and the backup SRAM to be mass-erased. As a result
 *            the user code area is cleared before the read protection is removed. The mass
 *            erase only erases the user code area. The other option bytes including write
 *            protections remain unchanged from before the mass-erase operation. The OTP
 *            area is not affected by mass erase and remains unchanged.
 *
 *          Mass erase is performed only when LEVEL 1 is active and LEVEL 0 requested. When
 *          the protection level is increased (0->1, 1->2, 0->2) there is no mass erase.
 *
 *          LEVEL 2: Disable debug/chip read protection
 *          When the read protection LEVEL 2 is activated by writing 0xCC to the RDP option byte,
 *          all protections provided by LEVEL 1 are active, system memory and all debug features
 *          (CPU JTAG and single-wire) are disabled when booting from SRAM or from system
 *          memory, and user options can no longer be changed.
 *          Memory read protection LEVEL 2 is an irreversible operation. When LEVEL 2 is activated,
 *          the level of protection cannot be decreased to LEVEL 0 or LEVEL 1
 * @{
 */
typedef enum
{
    OB_RDP_LEVEL_0 = 0xAA,
    OB_RDP_LEVEL_1 = 0x55,
    OB_RDP_LEVEL_2 = 0xCC,
} ob_rdp_level_e;
/** @} */

/**
 * @brief   LEVEL of brown-out detection circuit
 * @{
 */
typedef enum
{
    OB_BOR_LEVEL_OFF = FLASH_OPTCR_BOR_LEV_1 | FLASH_OPTCR_BOR_LEV_0,
    OB_BOR_LEVEL_1 = FLASH_OPTCR_BOR_LEV_1,
    OB_BOR_LEVEL_2 = FLASH_OPTCR_BOR_LEV_0,
    OB_BOR_LEVEL_3 = 0x00,
} ob_bor_level_e;
/** @} */

/**
 * @brief   User option bits
 *          OB_USER_no_WDG_HW:
 *            Software IWDG selected (user has to start watchdog)
 *          OB_USER_WDG_HW:
 *            Hardware IWDG selected (watchdog start automatically)
 *          OB_USER_no_RST_STOP:
 *            No reset generated when entering in STOP
 *          OB_USER_RST_STOP:
 *            Reset generated when entering STOP
 *          OB_USER_no_RST_STDBY:
 *            No reset generated when entering in STANDBY
 *          OB_USER_RST_STDBY:
 *            Reset generated when entering in STANDBY
 *          OB_USER_no_BFB2:
 *            The device boots from Flash memory bank 1 or bank 2, depending on
 *            the activation of the bank. The active banks are checked in the
 *            following order: bank 2, followed by bank 1. The active bank is
 *            identified by the value programmed at the base address of the bank
 *            ( corresponding to the initial stack pointer value in the
 *            interrupt vector table). Refer to application note AN2606 for
 *            further details. In this case, the boot pins can only select user
 *            Flash or RAM boot.
 *          OB_USER_BFB2:
 *            The device will boot from Flash memory bank 1 when boot pins are
 *            set in "boot from user Flash" position (default).
 * @note    When OB_USER_no_WDG_HW is not selected, JTAG / SWD programming will
 *          not be possible anymore due to the watchdog interrupting the programming
 *          process.
 * @{
 */
enum
{
    OB_USER_no_WDG_HW = FLASH_OPTCR_WDG_SW,
    OB_USER_WDG_HW = 0,
    OB_USER_no_RST_STOP = FLASH_OPTCR_nRST_STOP,
    OB_USER_RST_STOP = 0,
    OB_USER_no_RST_STDBY = FLASH_OPTCR_nRST_STDBY,
    OB_USER_RST_STDBY = 0,
#if defined(STM32F427_437xx) || defined(STM32F429_439xx) || defined(__DOXYGEN)
    OB_USER_no_BFB2 = 0,
    OB_USER_BFB2 = (uint32_t)0x00000020, /* Definition is missing in st header. */
#endif /* defined(STM32F427_437xx) || defined(STM32F429_439xx) */
};
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

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
    FLASH_TypeDef* flash;
#if FLASH_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_CFG_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief mutex_t protecting the device.
     */
    mutex_t mutex;
#elif CH_CFG_USE_SEMAPHORES
    semaphore_t semaphore;
#endif
#endif /* FLASH_USE_MUTUAL_EXCLUSION */
} FLASHDriver;

typedef struct
{
    uint32_t sector;
    uint32_t origin;
    uint32_t size;
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
    void flash_lld_ob_rdp(FLASHDriver* flashp, ob_rdp_level_e level);
    void flash_lld_ob_user(FLASHDriver* flashp, uint8_t user);
    void flash_lld_ob_bor(FLASHDriver* flashp, ob_bor_level_e level);
#if defined(STM32F427_437xx) || defined(STM32F429_439xx) || defined(__DOXYGEN)
    void flash_lld1_writeprotect_sector(FLASHDriver* flashp,
            uint32_t startaddr);
    void flash_lld1_writeprotect_mass(FLASHDriver* flashp);
    void flash_lld1_writeunprotect_sector(FLASHDriver* flashp,
            uint32_t startaddr);
    void flash_lld1_writeunprotect_mass(FLASHDriver* flashp);
    void flash_lld1_ob_rdp(FLASHDriver* flashp, ob_rdp_level_e level);
    void flash_lld1_ob_user(FLASHDriver* flashp, uint8_t user);
    void flash_lld1_ob_bor(FLASHDriver* flashp, ob_bor_level_e level);
#endif /* defined(STM32F427_437xx) || defined(STM32F429_439xx) */
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_FLASH */

#endif /* _QFLASH_LLD_H_ */

/** @} */
