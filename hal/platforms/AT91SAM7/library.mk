
# ChibiOS default path if not specified
CHIBIOS_DIR ?= $(ROOT_DIR)/submodules/chibios
CHIBIOS := $(CHIBIOS_DIR)

QHAL_AT91SAM7_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CSRC += $(wildcard $(QHAL_AT91SAM7_DIR)/*.c)
CSRC += $(wildcard $(QHAL_AT91SAM7_DIR)/EFC/*.c)

EXTRAINCDIRS += $(QHAL_AT91SAM7_DIR)
EXTRAINCDIRS += $(QHAL_AT91SAM7_DIR)/EFC

LDFLAGS += -T$(QHAL_AT91SAM7_DIR)/ld/sections.ld

# ChibiOS
include $(CHIBIOS_DIR)/os/hal/platforms/AT91SAM7/platform.mk
include $(CHIBIOS_DIR)/os/ports/GCC/ARM/AT91SAM7/port.mk

include $(QHAL_AT91SAM7_DIR)/../../library.mk
