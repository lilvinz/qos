

QHAL_AT91SAM7X_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
CSRC += $(wildcard $(QHAL_AT91SAM7X_DIR)/*.c)
EXTRAINCDIRS += $(QHAL_AT91SAM7X_DIR)

# ChibiOS
include $(ROOT_DIR)/src/lib/ChibiOS/os/hal/platforms/STM32F4xx/platform.mk
include $(ROOT_DIR)/src/lib/ChibiOS/os/ports/GCC/ARM/AT91SAM7/port.mk

include ../../library.mk
