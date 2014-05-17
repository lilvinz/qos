
# ChibiOS default path if not specified
CHIBIOS_DIR ?= $(ROOT_DIR)/submodules/chibios
CHIBIOS := $(CHIBIOS_DIR)

QHAL_AT91SAM7X_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CSRC += $(wildcard $(QHAL_AT91SAM7X_DIR)/*.c)
EXTRAINCDIRS += $(QHAL_AT91SAM7X_DIR)

# ChibiOS
include $(CHIBIOS_DIR)/os/hal/platforms/STM32F4xx/platform.mk
include $(CHIBIOS_DIR)/os/ports/GCC/ARM/AT91SAM7/port.mk

include $(QHAL_AT91SAM7X_DIR)/../../library.mk
