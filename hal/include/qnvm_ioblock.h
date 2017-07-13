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
 * @file    qnvm_ioblock.h
 * @brief   qnvm to ioblock wrapper driver.
 *
 * @addtogroup NVM_IOBLOCK
 * @{
 */

#ifndef _QNVM_IOBLOCK_H_
#define _QNVM_IOBLOCK_H_

#if HAL_USE_NVM_IOBLOCK || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

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
 * @brief   NVM to ioblock wrapper driver configuration structure.
 */
typedef struct
{
    /**
    * @brief Pointer to BaseNVMDevice.
    */
    BaseNVMDevice* nvmp;
    /**
    * @brief Size of emulated blocks.
    */
    size_t block_size;
} NVMIOBlockConfig;

/**
 * @brief   @p NVMIOBlockDriver specific methods.
 */
#define _nvm_ioblock_driver_methods                                         \
    _base_block_device_methods

/**
 * @extends BaseNVMDeviceVMT
 *
 * @brief   @p NVMIOBlockDriver virtual methods table.
 */
struct NVMIOBlockDriverVMT
{
    _nvm_ioblock_driver_methods
};

/**
 * @extends BaseNVMDevice
 *
 * @brief   Structure representing a NVM ioblock driver.
 */
typedef struct
{
    /**
    * @brief Virtual Methods Table.
    */
    const struct NVMIOBlockDriverVMT* vmt;
    _base_block_device_data
    /**
    * @brief Current configuration data.
    */
    const NVMIOBlockConfig* config;
} NVMIOBlockDriver;

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
    void nvmioblockInit(void);
    void nvmioblockObjectInit(NVMIOBlockDriver* nvmioblockp);
    void nvmioblockStart(NVMIOBlockDriver* nvmioblockp,
            const NVMIOBlockConfig* config);
    void nvmioblockStop(NVMIOBlockDriver* nvmioblockp);
    bool nvmioblockRead(NVMIOBlockDriver* nvmioblockp, uint32_t startblk,
            uint8_t* buffer, uint32_t n);
    bool nvmioblockWrite(NVMIOBlockDriver* nvmioblockp, uint32_t startblk,
            const uint8_t* buffer, uint32_t n);
    bool nvmioblockSync(NVMIOBlockDriver* nvmioblockp);
    bool nvmioblockGetInfo(NVMIOBlockDriver* nvmioblockp, BlockDeviceInfo* bdip);
    bool nvmioblockIsInserted(NVMIOBlockDriver* nvmioblockp);
    bool nvmioblockIsProtected(NVMIOBlockDriver* nvmioblockp);
    bool nvmioblockConnect(NVMIOBlockDriver* nvmioblockp);
    bool nvmioblockDisconnect(NVMIOBlockDriver* nvmioblockp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_NVM_IOBLOCK */

#endif /* _QNVM_IOBLOCK_H_ */

/** @} */
