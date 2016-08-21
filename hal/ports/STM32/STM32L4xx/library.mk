
# ChibiOS default path if not specified
CHIBIOS_DIR ?= $(ROOT_DIR)/submodules/chibios
CHIBIOS := $(CHIBIOS_DIR)
CHIBIOS_CONTRIB := $(CHIBIOS)/community

QHAL_STM32L4XX_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CSRC += $(wildcard $(QHAL_STM32L4XX_DIR)/*.c)
CSRC += $(wildcard $(QHAL_STM32L4XX_DIR)/../LLD/FLASHv3/*.c)
CSRC += $(wildcard $(QHAL_STM32L4XX_DIR)/../LLD/USARTv2/*.c)
CSRC += $(wildcard $(QHAL_STM32L4XX_DIR)/../LLD/RTCv2/*.c)

EXTRAINCDIRS += $(QHAL_STM32L4XX_DIR)
EXTRAINCDIRS += $(QHAL_STM32L4XX_DIR)/../LLD/FLASHv3
EXTRAINCDIRS += $(QHAL_STM32L4XX_DIR)/../LLD/USARTv2
EXTRAINCDIRS += $(QHAL_STM32L4XX_DIR)/../LLD/RTCv2

# ChibiOS
include $(CHIBIOS_DIR)/os/common/ports/ARMCMx/compilers/GCC/mk/startup_stm32l4xx.mk
include $(CHIBIOS_DIR)/community/os/hal/ports/STM32/STM32L4xx/platform.mk

include $(QHAL_STM32L4XX_DIR)/../library.mk
