/**
 * @file    qio_nvm.h
 * @brief   I/O non volatile memory devices access.
 * @details This header defines an abstract interface useful to access generic
 *          I/O with non volatile memory devices in a standardized way.
 *
 * @addtogroup QIO_NVM
 * @details This module defines an abstract interface for accessing generic
 *          non volatile memory devices.<br>
 *          Note that no code is present, just abstract interfaces-like
 *          structures, you should look at the system as to a set of
 *          abstract C++ classes (even if written in C). This system
 *          has then advantage to make the access to nvm devices
 *          independent from the implementation logic.
 * @{
 */

#ifndef _QIO_NVM_H_
#define _QIO_NVM_H_

/**
 * @brief   Driver state machine possible states.
 */
typedef enum
{
    NVM_UNINIT = 0,                 /**< Not initialized.                   */
    NVM_STOP = 1,                   /**< Stopped.                           */
    NVM_READY = 2,                  /**< Device ready.                      */
    NVM_READING = 3,                /**< Read operation in progress.        */
    NVM_WRITING = 4,                /**< Write operation in progress.       */
    NVM_ERASING = 5,                /**< Erase operation in progress.       */
} nvmstate_t;

/**
 * @brief   Non volatile memory device info.
 */
typedef struct
{
    uint32_t      sector_size;        /**< @brief Sector size in bytes.       */
    uint32_t      sector_num;         /**< @brief Total number of sectors.    */
    uint8_t       identification[3];  /**< @brief Jedec device identification.*/
} NVMDeviceInfo;

/**
 * @brief   @p BaseNVMDevice specific methods.
 */
#define _base_nvm_device_methods                                              \
    /* Reads one or more bytes crossing sectors when required.*/              \
    bool_t (*read)(void *instance, uint32_t startaddr,                        \
            uint32_t n, uint8_t *buffer);                                     \
    /* Writes one or more bytes crossing sectors when required.*/             \
    bool_t (*write)(void *instance, uint32_t startaddr,                       \
            uint32_t n, const uint8_t *buffer);                               \
    /* Erase one or more sectors.*/                                           \
    bool_t (*erase)(void *instance, uint32_t startaddr,                       \
            uint32_t n);                                                      \
    /* Erase all sectors.*/                                                   \
    bool_t (*mass_erase)(void *instance);                                     \
    /* Write / erase operations synchronization.*/                            \
    bool_t (*sync)(void *instance);                                           \
    /* Obtains info about the media.*/                                        \
    bool_t (*get_info)(void *instance, NVMDeviceInfo *nvmdip);                \
    /* End of mandatory functions. */                                         \
    /* Acquire device if supported by underlying driver.*/                    \
    void (*acquire)(void *instance);                                          \
    /* Release device if supported by underlying driver.*/                    \
    void (*release)(void *instance);                                          \
    /* Write protect one or more sectors crossing sectors when required. */   \
    bool_t (*writeprotect)(void *instance, uint32_t startaddr,                \
            uint32_t n);                                                      \
    /* Write protect whole device. */                                         \
    bool_t (*mass_writeprotect)(void *instance);                              \
    /* Write unprotect one or more sectors crossing sectors when required. */ \
    bool_t (*writeunprotect)(void *instance, uint32_t startaddr,              \
            uint32_t n);                                                      \
    /* Write unprotect whole device. */                                       \
    bool_t (*mass_writeunprotect)(void *instance);

/**
 * @brief   @p BaseNVMDevice specific data.
 */
#define _base_nvm_device_data                                                 \
    /* Driver state.*/                                                        \
    nvmstate_t state;

/**
 * @brief   @p BaseNVMDevice virtual methods table.
 */
struct BaseNVMDeviceVMT
{
    _base_nvm_device_methods
};

/**
 * @brief   Base nvm device class.
 * @details This class represents a generic, block-accessible, device.
 */
typedef struct
{
    /** @brief Virtual Methods Table.*/
    const struct BaseNVMDeviceVMT* vmt;
    _base_nvm_device_data
} BaseNVMDevice;

/**
 * @name    Macro Functions (BaseNVMDevice)
 * @{
 */
/**
 * @brief   Returns the driver state.
 * @note    Can be called in ISR context.
 *
 * @param[in] ip        pointer to a @p BaseNVMDevice or derived class
 *
 * @return              The driver state.
 *
 * @special
 */
#define nvmGetDriverState(ip) ((ip)->state)

/**
 * @brief   Determines if the device is transferring data.
 * @note    Can be called in ISR context.
 *
 * @param[in] ip        pointer to a @p BaseNVMDevice or derived class
 *
 * @return              The driver state.
 * @retval FALSE        the device is not transferring data.
 * @retval TRUE         the device not transferring data.
 *
 * @special
 */
#define nvmIsTransferring(ip) ((((ip)->state) == NVM_READING) ||              \
                               (((ip)->state) == NVM_WRITING) ||              \
                               (((ip)->state) == NVM_ERASING))

/**
 * @brief   Reads one or more bytes crossing sectors when required.
 *
 * @param[in] ip        pointer to a @p BaseNVMDevice or derived class
 * @param[in] startaddr first address to read
 * @param[in] n         number of bytes to read
 * @param[out] buffer   pointer to the read buffer
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define nvmRead(ip, startaddr, n, buffer)                                     \
    ((ip)->vmt->read(ip, startaddr, n, buffer))

/**
 * @brief   Writes one or more bytes crossing sectors when required.
 *
 * @param[in] ip        pointer to a @p BaseNVMDevice or derived class
 * @param[in] startaddr first address to write
 * @param[in] n         number of bytes to write
 * @param[out] buffer   pointer to the write buffer
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define nvmWrite(ip, startaddr, n, buffer)                                    \
    ((ip)->vmt->write(ip, startaddr, n, buffer))

/**
 * @brief   Erases one or more sectors.
 *
 * @param[in] ip        pointer to a @p BaseNVMDevice or derived class
 * @param[in] startaddr address of sector to erase
 * @param[in] n         number of bytes to erase
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define nvmErase(ip, startaddr, n)                                            \
    ((ip)->vmt->erase(ip, startaddr, n))

/**
 * @brief   Erases all sectors.
 *
 * @param[in] ip        pointer to a @p BaseNVMDevice or derived class
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define nvmMassErase(ip)                                                      \
    ((ip)->vmt->mass_erase(ip))

/**
 * @brief   Ensures write / erase synchronization.
 *
 * @param[in] ip        pointer to a @p BaseNVMDevice or derived class
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define nvmSync(ip) ((ip)->vmt->sync(ip))

/**
 * @brief   Returns a media information structure.
 *
 * @param[in] ip        pointer to a @p BaseNVMDevice or derived class
 * @param[out] nvmdip   pointer to a @p NVMDeviceInfo structure
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define nvmGetInfo(ip, nvmdip) ((ip)->vmt->get_info(ip, nvmdip))

/**
 * @brief   Acquires device for exclusive access if implemented.
 *
 * @param[in] ip        pointer to a @p BaseNVMDevice or derived class
 *
 * @api
 */
#define nvmAcquire(ip) ((ip)->vmt->acquire)(ip)

/**
 * @brief   Releases exclusive access from device if implemented.
 *
 * @param[in] ip        pointer to a @p BaseNVMDevice or derived class
 *
 * @api
 */
#define nvmRelease(ip) ((ip)->vmt->release)(ip)

/**
 * @brief   Write protects one or more sectors if implemented.
 *
 * @param[in] ip        pointer to a @p BaseNVMDevice or derived class
 * @param[in] startaddr address of sector to protect
 * @param[in] n         number of bytes to protect
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded or not implemented.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define nvmWriteProtect(ip, startaddr, n)                                     \
        ((ip)->vmt->writeprotect)(ip, startaddr, n)

/**
 * @brief   Write protects whole device if implemented.
 *
 * @param[in] ip        pointer to a @p BaseNVMDevice or derived class
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded or not implemented.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define nvmMassWriteProtect(ip)                                               \
        ((ip)->vmt->mass_writeprotect)(ip)

/**
 * @brief   Write unprotects one or more sectors if implemented.
 *
 * @param[in] ip        pointer to a @p BaseNVMDevice or derived class
 * @param[in] startaddr address of sector to unprotect
 * @param[in] n         number of bytes to unprotect
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded or not implemented.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define nvmWriteUnprotect(ip, startaddr, n)                                   \
        ((ip)->vmt->writeunprotect)(ip, startaddr, n)

/**
 * @brief   Write unprotects whole device if implemented.
 *
 * @param[in] ip        pointer to a @p BaseNVMDevice or derived class
 *
 * @return              The operation status.
 * @retval CH_SUCCESS   operation succeeded or not implemented.
 * @retval CH_FAILED    operation failed.
 *
 * @api
 */
#define nvmMassWriteUnprotect(ip)                                             \
        ((ip)->vmt->mass_writeunprotect)(ip)

/** @} */

#endif /* _QIO_NVM_H_ */

/** @} */
