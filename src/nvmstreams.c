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
 * @file    nvmstreams.c
 * @brief   NVM streams code.
 *
 * @addtogroup nvm_streams
 * @{
 */

#include "ch.h"
#include "nvmstreams.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static size_t writes(void *ip, const uint8_t *bp, size_t n)
{
    NVMStream *nvmsp = ip;

    if (nvmsp->size - nvmsp->eos < n)
        n = nvmsp->size - nvmsp->eos;

    nvmAcquire(nvmsp->nvmdp);
    if (nvmWrite(nvmsp->nvmdp, nvmsp->eos, n, bp) != HAL_SUCCESS)
    {
        nvmRelease(nvmsp->nvmdp);
        return 0;
    }
    nvmRelease(nvmsp->nvmdp);

    nvmsp->eos += n;

    return n;
}

static size_t reads(void *ip, uint8_t *bp, size_t n)
{
    NVMStream *nvmsp = ip;

    if (nvmsp->eos - nvmsp->offset < n)
        n = nvmsp->eos - nvmsp->offset;

    nvmAcquire(nvmsp->nvmdp);
    if (nvmRead(nvmsp->nvmdp, nvmsp->offset, n, bp) != HAL_SUCCESS)
    {
        nvmRelease(nvmsp->nvmdp);
        return 0;
    }
    nvmRelease(nvmsp->nvmdp);

    nvmsp->offset += n;

    return n;
}

static msg_t put(void *ip, uint8_t b)
{
    NVMStream *nvmsp = ip;

    if (nvmsp->size - nvmsp->eos <= 0)
        return RDY_RESET;

    nvmAcquire(nvmsp->nvmdp);
    if (nvmWrite(nvmsp->nvmdp, nvmsp->eos, 1, &b) != HAL_SUCCESS)
    {
        nvmRelease(nvmsp->nvmdp);
        return RDY_RESET;
    }
    nvmRelease(nvmsp->nvmdp);

    nvmsp->eos += 1;

    return RDY_OK;
}

static msg_t get(void *ip)
{
    uint8_t b;
    NVMStream *nvmsp = ip;

    if (nvmsp->eos - nvmsp->offset <= 0)
        return RDY_RESET;

    nvmAcquire(nvmsp->nvmdp);
    if (nvmRead(nvmsp->nvmdp, nvmsp->offset, 1, &b) != HAL_SUCCESS)
    {
        nvmRelease(nvmsp->nvmdp);
        return RDY_RESET;
    }
    nvmRelease(nvmsp->nvmdp);

    nvmsp->offset += 1;

    return b;
}

static const struct NVMStreamVMT vmt =
{
    writes,
    reads,
    put,
    get,
};

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   NVM stream object initialization.
 *
 * @param[in] nvmsp     pointer to the @p NVMStream object to be initialized
 * @param[in] dev       pointer to the @p BaseNVMDevice for the stream
 * @param[in] eos       Initial End Of Stream offset. Normally you need to
 *                      put this to zero for output streams or equal to @p size
 *                      for input streams.
 *
 */
void nvmsObjectInit(NVMStream *nvmsp, BaseNVMDevice *nvmdp, size_t eos)
{
    nvmsp->vmt = &vmt;
    nvmsp->nvmdp = nvmdp;
    nvmsp->eos = eos;
    nvmsp->offset = 0;

    /* Set size. */
    {
        NVMDeviceInfo di;
        if (nvmGetInfo(nvmdp, &di) == HAL_SUCCESS)
        {
            nvmsp->size = di.sector_size * di.sector_num;
        }
        else
        {
            nvmsp->size = 0;
        }
    }

    /* Verify device status. */
    chDbgAssert(nvmsp->size > 0,
            "nvmsObjectInit(), #1", "invalid size");
}

/** @} */
