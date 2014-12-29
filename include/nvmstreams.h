/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

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
 * @file    nvmstreams.h
 * @brief   NVM streams structures and macros.
 
 * @addtogroup nvm_streams
 * @{
 */

#ifndef _NVMSTREAMS_H_
#define _NVMSTREAMS_H_

#include "qhal.h"

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
 * @brief   @p NVMStream specific data.
 */
#define _nvm_stream_data                                                      \
    _base_sequential_stream_data                                              \
    /* Pointer to the nvm device. */                                          \
    BaseNVMDevice *nvmdp;                                                     \
    /* Size of the stream.*/                                                  \
    size_t size;                                                              \
    /* Current end of stream.*/                                               \
    size_t eos;                                                               \
    /* Current read / write offset.*/                                         \
    size_t offset;

/**
 * @brief   @p NVMStream virtual methods table.
 */
struct NVMStreamVMT
{
    _base_sequential_stream_methods
};

/**
 * @extends BaseSequentialStream
 *
 * @brief Memory stream object.
 */
typedef struct
{
    /** @brief Virtual Methods Table.*/
    const struct NVMStreamVMT *vmt;
    _nvm_stream_data
} NVMStream;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
    void nvmsObjectInit(NVMStream *nvmsp, BaseNVMDevice *nvmdp, size_t eos);
#ifdef __cplusplus
}
#endif

#endif /* _NVMSTREAMS_H_ */

/** @} */
