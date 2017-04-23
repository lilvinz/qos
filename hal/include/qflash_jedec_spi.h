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
 * @file    qflash_jedec_spi.h
 * @brief   FLASH JEDEC over SPI driver header.
 *
 * @addtogroup FLASH_JEDEC_SPI
 * @{
 */

#ifndef _QFLASH_JEDEC_SPI_H_
#define _QFLASH_JEDEC_SPI_H_

#if HAL_USE_FLASH_JEDEC_SPI || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    FLASH_JEDEC_SPI configuration options
 * @{
 */
/**
 * @brief   Delays insertions.
 * @details If enabled this options inserts delays into the flash waiting
 *          routines releasing some extra CPU time for the threads with
 *          lower priority, this may slow down the driver a bit however.
 *          This option is recommended also if the SPI driver does not
 *          use a DMA channel and heavily loads the CPU.
 */
#if !defined(FLASH_JEDEC_SPI_NICE_WAITING) || defined(__DOXYGEN__)
#define FLASH_JEDEC_SPI_NICE_WAITING             TRUE
#endif

/**
 * @brief   Enables the @p fjsAcquireBus() and @p fjsReleaseBus() APIs.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(FLASH_JEDEC_SPI_USE_MUTUAL_EXCLUSION) || defined(__DOXYGEN__)
#define FLASH_JEDEC_SPI_USE_MUTUAL_EXCLUSION     TRUE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !HAL_USE_SPI || !SPI_USE_WAIT
#error "FLASH_JEDEC_SPI driver requires HAL_USE_SPI and SPI_USE_WAIT"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Flash JEDEC over SPI driver configuration structure.
 */
typedef struct
{
    /**
    * @brief SPI driver associated to this Flash JEDEC driver.
    */
    SPIDriver* spip;
    /**
    * @brief SPI driver configuration for multi slave buses or NULL.
    */
    const SPIConfig* spi_cfgp;
    /**
     * @brief Smallest erasable sector size in bytes.
     */
    uint32_t sector_size;
    /**
     * @brief Total number of sectors.
     */
    uint32_t sector_num;
    /**
     * @brief Maximum amount of data programmable through page program command.
     */
    uint32_t page_size;
    /**
     * @brief Required alignment of page program address.
     */
    uint8_t page_alignment;
    /**
     * @brief Number of address bytes used in commands.
     */
    uint8_t addrbytes_num;
    /**
     * @brief Number of block protection bits.
     */
    uint8_t bpbits_num;
    /**
     * @brief Sector erase command.
     * Set to erase command matching sector size.
     * Set to 0x00 if erase is not required.
     */
    uint8_t cmd_sector_erase;
    /**
     * @brief Page program command.
     * Commands:
     * - 0x02 (BYTE program)
     * - 0xad (AAI WORD program)
     */
    uint8_t cmd_page_program;
    /**
     * @brief Read command.
     * Commands:
     * - 0x03 (READ)
     * - 0x0b (FAST READ)
     */
    uint8_t cmd_read;
} FlashJedecSPIConfig;

/**
 * @brief   @p FlashJedecSPIDriver specific methods.
 */
#define _flash_jedec_spi_driver_methods                                       \
    _base_nvm_device_methods

/**
 * @extends BaseNVMDeviceVMT
 *
 * @brief   @p FlashJedecSPIDriver virtual methods table.
 */
struct FlashJedecSPIDriverVMT
{
    _flash_jedec_spi_driver_methods
};

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a FLASH JEDEC over SPI driver.
 */
typedef struct
{
    /**
    * @brief Virtual Methods Table.
    */
    const struct FlashJedecSPIDriverVMT* vmt;
    _base_nvm_device_data
    /**
    * @brief Current configuration data.
    */
    const FlashJedecSPIConfig* config;
#if FLASH_JEDEC_SPI_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
    /**
     * @brief mutex_t protecting the device.
     */
    mutex_t mutex;
#endif /* FLASH_JEDEC_SPI_USE_MUTUAL_EXCLUSION */
} FlashJedecSPIDriver;

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
    void fjsInit(void);
    void fjsObjectInit(FlashJedecSPIDriver* fjsp);
    void fjsStart(FlashJedecSPIDriver* fjsp,
            const FlashJedecSPIConfig* config);
    void fjsStop(FlashJedecSPIDriver* fjsp);
    bool fjsRead(FlashJedecSPIDriver* fjsp, uint32_t startaddr, uint32_t n,
            uint8_t* buffer);
    bool fjsWrite(FlashJedecSPIDriver* fjsp, uint32_t startaddr, uint32_t n,
            const uint8_t* buffer);
    bool fjsErase(FlashJedecSPIDriver* fjsp, uint32_t startaddr, uint32_t n);
    bool fjsMassErase(FlashJedecSPIDriver* fjsp);
    bool fjsSync(FlashJedecSPIDriver* fjsp);
    bool fjsGetInfo(FlashJedecSPIDriver* fjsp, NVMDeviceInfo* nvmdip);
    void fjsAcquireBus(FlashJedecSPIDriver* fjsp);
    void fjsReleaseBus(FlashJedecSPIDriver* fjsp);
    bool fjsWriteProtect(FlashJedecSPIDriver* fjsp, uint32_t startaddr, uint32_t n);
    bool fjsMassWriteProtect(FlashJedecSPIDriver* fjsp);
    bool fjsWriteUnprotect(FlashJedecSPIDriver* fjsp, uint32_t startaddr, uint32_t n);
    bool fjsMassWriteUnprotect(FlashJedecSPIDriver* fjsp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_FLASH_JEDEC_SPI */

#endif /* _QFLASH_JEDEC_SPI_H_ */

/** @} */
