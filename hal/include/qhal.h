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
 * @file    qhal.h
 * @brief   Quantec HAL subsystem header.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef _QHAL_H_
#define _QHAL_H_


#include "hal.h"

#include "qhalconf.h"

#include "qhal_lld.h"

/* Abstract interfaces.*/
#include "qhal_io_nvm.h"
#include "qhal_gd.h"
#include "qhal_serial_virtual.h"

/* Shared headers.*/

/* Layered drivers.*/
#include "qhal_flash.h"
#include "qhal_serial_485.h"
#include "qhal_gd_sim.h"
#include "qhal_rtc_lld.h"

/* Complex drivers.*/
#include "qhal_flash_jedec_spi.h"
#include "qhal_nvm_partition.h"
#include "qhal_nvm_file.h"
#include "qhal_nvm_memory.h"
#include "qhal_nvm_mirror.h"
#include "qhal_nvm_fee.h"
#include "qhal_nvm_ioblock.h"
#include "qhal_led.h"
#include "qhal_gd_ili9341.h"
#include "qhal_ms5541.h"
#include "qhal_serial_fdx.h"
#include "qhal_ms58xx.h"
#include "qhal_bq275xx.h"
#include "qhal_ads111x.h"

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
    void qhalInit(void);
#ifdef __cplusplus
}
#endif

#endif /* _QHAL_H_ */

/** @} */

