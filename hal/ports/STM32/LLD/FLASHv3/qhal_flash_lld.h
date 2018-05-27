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
 * @file    STM32/FLASHv3/qflash.c
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
 * @brief   User option bits
 *          OB_USER_no_RST_STOP:
 *            No reset generated when entering in STOP
 *          OB_USER_RST_STOP:
 *            Reset generated when entering STOP
 *          OB_USER_no_RST_STDBY:
 *            No reset generated when entering in STANDBY
 *          OB_USER_RST_STDBY:
 *            Reset generated when entering in STANDBY
 *          OB_USER_no_RST_SHDW:
 *            No reset generated when entering in SHUTDOWN
 *          OB_USER_RST_SHDW:
 *            Reset generated when entering in SHUTDOWN
 *          OB_USER_no_IWDG_HW:
 *            Software IWDG selected (user has to start watchdog)
 *          OB_USER_IWDG_HW:
 *            Hardware IWDG selected (watchdog start automatically)
 *          OB_USER_no_IWDG_STOP:
 *            IWDG is frozen in stop mode
 *          OB_USER_IWDG_STOP:
 *            IWDG is running in stop mode
 *          OB_USER_no_IWDG_STDBY:
 *            IWDG is frozen in standby mode
 *          OB_USER_IWDG_STDBY:
 *            IWDG is running in standby mode
 *          OB_USER_no_IWDG_STDBY:
 *            IWDG is frozen in standby mode
 *          OB_USER_IWDG_STDBY:
 *            IWDG is running in standby mode
 *          OB_USER_no_WWDG_HW:
 *            Software WWDG selected (user has to start watchdog)
 *          OB_USER_WWDG_HW:
 *            Hardware WWDG selected (watchdog start automatically)
 *          OB_USER_no_BFB2:
 *            Dual-bank boot disable
 *          OB_USER_BFB2:
 *            Dual-bank boot enable
 *          OB_USER_no_DUALBANK:
 *            Configure flash memory as single bank
 *          OB_USER_DUALBANK:
 *            Configure flash memory as dual bank
 *          OB_USER_no_BOOT1:
 *            Assume boot mode bit 1 low
 *          OB_USER_BOOT1
 *            Assume boot mode bit 1 high
 *          OB_USER_no_SRAM2_RST:
 *            SRAM2 is not erased when a system reset occurs
 *          OB_USER_SRAM2_RST:
 *            SRAM2 erased when a system reset occurs
 *          OB_USER_no_SRAM2_PE:
 *            SRAM2 parity check disable
 *          OB_USER_SRAM2_PE:
 *            SRAM2 parity check enable
 * @{
 */
typedef enum
{
    OB_USER_RST_STOP = 0,
    OB_USER_no_RST_STOP = FLASH_OPTR_nRST_STOP,
    OB_USER_RST_STDBY = 0,
    OB_USER_no_RST_STDBY = FLASH_OPTR_nRST_STDBY,
    OB_USER_RST_SHDW = 0,
    OB_USER_no_RST_SHDW = FLASH_OPTR_nRST_STDBY << 1, /* Missing in st header. */
    OB_USER_no_IWDG_HW = FLASH_OPTR_IWDG_SW,
    OB_USER_IWDG_HW = 0,
    OB_USER_no_IWDG_STOP = 0,
    OB_USER_IWDG_STOP = FLASH_OPTR_IWDG_STOP,
    OB_USER_no_IWDG_STDBY = 0,
    OB_USER_IWDG_STDBY = FLASH_OPTR_IWDG_STDBY,
    OB_USER_no_WWDG_HW = FLASH_OPTR_WWDG_SW,
    OB_USER_WWDG_HW = 0,
    OB_USER_no_BFB2 = 0,
    OB_USER_BFB2 = FLASH_OPTR_BFB2,
    OB_USER_no_DUALBANK = 0,
    OB_USER_DUALBANK = FLASH_OPTR_DUALBANK,
    OB_USER_no_BOOT1 = FLASH_OPTR_nBOOT1,
    OB_USER_BOOT1 = 0,
    OB_USER_no_SRAM2_PE = 0,
    OB_USER_SRAM2_PE = FLASH_OPTR_SRAM2_PE,
    OB_USER_no_SRAM2_RST = 0,
    OB_USER_SRAM2_RST = FLASH_OPTR_SRAM2_RST,
} ob_user_e;

/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Check for supported devices. */
#if defined(STM32L471xx) ||             \
        defined(STM32L475xx) ||             \
        defined(STM32L476xx) ||             \
        defined(STM32L485xx) ||             \
        defined(STM32L486xx)
#else
#error "device unsupported by FLASHv3 driver"
#endif

/**
 * @brief   FLASH readout protection options
 *
 *          LEVEL 0: no read protection
 *          When the read protection level is set to LEVEL 0 by writing 0xa5 into the read protection
 *          option byte (RDP), all read/write operations (if no write protection is set) from/to the
 *          Flash memory or the backup SRAM are possible in all boot configurations (Flash user
 *          boot, debug or boot from RAM).
 *
 *          LEVEL 1: memory read protection.
 *          It is the default read protection level after option byte erase. The read protection LEVEL 1
 *          is activated by writing any value (except for 0x45 used to set LEVEL 0
 *          into the RDP option byte. When the read protection LEVEL 1 is set:
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
 *            protections remain unchanged from before the mass-erase operation.
 *
 *          Mass erase is performed only when LEVEL 1 is active and LEVEL 0 requested. When
 *          the protection level is increased (0->1) there is no mass erase.
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
    OB_BOR_LEVEL_0 = FLASH_OPTR_BOR_LEV_0,
    OB_BOR_LEVEL_1 = FLASH_OPTR_BOR_LEV_1,
    OB_BOR_LEVEL_2 = FLASH_OPTR_BOR_LEV_2,
    OB_BOR_LEVEL_3 = FLASH_OPTR_BOR_LEV_3,
    OB_BOR_LEVEL_4 = FLASH_OPTR_BOR_LEV_4,
} ob_bor_level_e;
/** @} */

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
    uint8_t wrp_bit;
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
    void flash_lld_ob_user(FLASHDriver* flashp, ob_user_e user);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_FLASH */

#endif /* _QFLASH_LLD_H_ */

/** @} */
