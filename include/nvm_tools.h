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

#ifndef NVM_TOOLS_H_
#define NVM_TOOLS_H_

#include "qhal.h"

#include <stdbool.h>
#include <stdint.h>

int nvmcmp(BaseNVMDevice* devap, BaseNVMDevice* devbp, uint32_t n);
bool nvmcpy(BaseNVMDevice* dstp, BaseNVMDevice* srcp, uint32_t n);
bool nvmset(BaseNVMDevice* dstp, uint8_t pattern, uint32_t n);

#endif /* NVM_TOOLS_H_ */
