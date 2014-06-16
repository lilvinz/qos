/*
 * nvm_tools.c
 *
 *  Created on: 18.01.2014
 *      Author: vke
 */

#include "nvm_tools.h"

/**
 * @brief   Compares content of two BaseNVMDevice objects.
 *
 * @param[in] devap     pointer to the first @p BaseNVMDevice object
 * @param[in] devb      pointer to the second @p BaseNVMDevice object
 * @param[in] n         number of bytes to compare
 *
 * @return              The result of the comparison.
 * @retval true         The compared data is equal.
 * @retval false        The compared data is not equal or an error occurred.
 *
 * @api
 */
int nvmcmp(BaseNVMDevice* devap, BaseNVMDevice* devbp, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i)
    {
        uint8_t bytea;
        if (nvmRead(devap, i, 1, &bytea) != CH_SUCCESS)
            return -1;
        uint8_t byteb;
        if (nvmRead(devbp, i, 1, &byteb) != CH_SUCCESS)
            return -1;

        if (bytea != byteb)
            return 1;
    }
    return 0;
}

/**
 * @brief   Copies data between BaseNVMDevice objects.
 *
 * @param[in] dstp      pointer to the first @p BaseNVMDevice object
 * @param[in] srcp      pointer to the second @p BaseNVMDevice object
 * @param[in] n         number of bytes to copy
 *
 * @return              The result of the operation.
 * @retval true         The operation was successful.
 * @retval false        An error occurred.
 *
 * @api
 */
bool nvmcpy(BaseNVMDevice* dstp, BaseNVMDevice* srcp, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i)
    {
        uint8_t byte;
        if (nvmRead(srcp, i, 1, &byte) != CH_SUCCESS)
            return false;
        if (nvmWrite(dstp, i, 1, &byte) != CH_SUCCESS)
            return false;
    }
    return true;
}

/**
 * @brief   Sets data of a BaseNVMDevice object to a desired pattern.
 *
 * @param[in] dstp      pointer to the first @p BaseNVMDevice object
 * @param[in] pattern   pattern to set memory to
 * @param[in] n         number of bytes to set
 *
 * @return              The result of the operation.
 * @retval true         The operation was successful.
 * @retval false        An error occurred.
 *
 * @api
 */
bool nvmset(BaseNVMDevice* dstp, uint8_t pattern, uint32_t n)
{
    if (nvmErase(dstp, 0, n) != CH_SUCCESS)
        return false;
    for (uint32_t i = 0; i < n; ++i)
    {
        if (nvmWrite(dstp, i, 1, &pattern) != CH_SUCCESS)
            return false;
    }
    return true;
}
