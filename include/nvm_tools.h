/*
 * nvm_tools.h
 *
 *  Created on: 18.01.2014
 *      Author: vke
 */

#ifndef NVM_TOOLS_H_
#define NVM_TOOLS_H_

#include "qhal.h"

#include <stdbool.h>
#include <stdint.h>

int nvmcmp(BaseNVMDevice* devap, BaseNVMDevice* devbp, uint32_t n);
bool nvmcpy(BaseNVMDevice* dstp, BaseNVMDevice* srcp, uint32_t n);
bool nvmset(BaseNVMDevice* dstp, uint8_t pattern, uint32_t n);

#endif /* NVM_TOOLS_H_ */
