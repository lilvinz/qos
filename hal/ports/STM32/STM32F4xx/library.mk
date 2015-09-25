
# ChibiOS default path if not specified
CHIBIOS_DIR ?= $(ROOT_DIR)/submodules/chibios
CHIBIOS := $(CHIBIOS_DIR)

QHAL_STM32F4XX_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CSRC += $(wildcard $(QHAL_STM32F4XX_DIR)/*.c)
CSRC += $(wildcard $(QHAL_STM32F4XX_DIR)/../STM32/FLASHv2/*.c)
CSRC += $(wildcard $(QHAL_STM32F4XX_DIR)/../STM32/WDGv1/*.c)
CSRC += $(wildcard $(QHAL_STM32F4XX_DIR)/../STM32/USARTv1/*.c)
CSRC += $(wildcard $(QHAL_STM32F4XX_DIR)/../STM32/RTCv2/*.c)

EXTRAINCDIRS += $(QHAL_STM32F4XX_DIR)
EXTRAINCDIRS += $(QHAL_STM32F4XX_DIR)/../STM32/FLASHv2
EXTRAINCDIRS += $(QHAL_STM32F4XX_DIR)/../STM32/WDGv1
EXTRAINCDIRS += $(QHAL_STM32F4XX_DIR)/../STM32/USARTv1
EXTRAINCDIRS += $(QHAL_STM32F4XX_DIR)/../STM32/RTCv2

LDFLAGS += -T$(QHAL_STM32F4XX_DIR)/ld/sections.ld

# ChibiOS hal
include $(CHIBIOS_DIR)/os/hal/platforms/STM32F4xx/platform.mk
# Chibios port
include $(CHIBIOS_DIR)/os/ports/GCC/ARMCMx/STM32F4xx/port.mk

include $(QHAL_STM32F4XX_DIR)/../STM32/library.mk
