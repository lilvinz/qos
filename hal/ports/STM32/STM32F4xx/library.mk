
# ChibiOS default path if not specified
CHIBIOS_DIR ?= $(ROOT_DIR)/submodules/chibios
CHIBIOS := $(CHIBIOS_DIR)

QHAL_STM32F4XX_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CSRC += $(wildcard $(QHAL_STM32F4XX_DIR)/*.c)
CSRC += $(wildcard $(QHAL_STM32F4XX_DIR)/../LLD/FLASHv2/*.c)
CSRC += $(wildcard $(QHAL_STM32F4XX_DIR)/../LLD/WDGv1/*.c)
CSRC += $(wildcard $(QHAL_STM32F4XX_DIR)/../LLD/USARTv1/*.c)
CSRC += $(wildcard $(QHAL_STM32F4XX_DIR)/../LLD/RTCv2/*.c)

EXTRAINCDIRS += $(QHAL_STM32F4XX_DIR)
EXTRAINCDIRS += $(QHAL_STM32F4XX_DIR)/../LLD/FLASHv2
EXTRAINCDIRS += $(QHAL_STM32F4XX_DIR)/../LLD/WDGv1
EXTRAINCDIRS += $(QHAL_STM32F4XX_DIR)/../LLD/USARTv1
EXTRAINCDIRS += $(QHAL_STM32F4XX_DIR)/../LLD/RTCv2

LDFLAGS += -T$(QHAL_STM32F4XX_DIR)/ld/sections.ld

# ChibiOS
include $(CHIBIOS_DIR)/os/common/ports/ARMCMx/compilers/GCC/mk/startup_stm32f4xx.mk
include $(CHIBIOS_DIR)/os/hal/ports/STM32/STM32F4xx/platform.mk

include $(QHAL_STM32F4XX_DIR)/../library.mk
