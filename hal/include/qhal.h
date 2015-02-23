
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
#include "qio_nvm.h"
#include "qgd.h"
#include "qserial_virtual.h"

/* Shared headers.*/

/* Layered drivers.*/
#include "qflash.h"
#include "qwdg.h"
#include "qserial_485.h"
#include "qgd_sim.h"
#include "qrtc_lld.h"

/* Complex drivers.*/
#include "qflash_jedec_spi.h"
#include "qnvm_partition.h"
#include "qnvm_file.h"
#include "qnvm_memory.h"
#include "qnvm_mirror.h"
#include "qnvm_fee.h"
#include "qled.h"
#include "qgd_ili9341.h"
#include "qms5541.h"
#include "qserial_fdx.h"
#include "qms58xx.h"

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

