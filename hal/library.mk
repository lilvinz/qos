
QHAL_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CSRC += $(wildcard $(QHAL_DIR)/src/*.c)
EXTRAINCDIRS += $(QHAL_DIR)/include

CSRC += $(PLATFORMSRC)
CSRC += $(HALSRC)
CSRC += $(PORTSRC)
CSRC += $(KERNSRC)

EXTRAINCDIRS += $(PLATFORMINC)
EXTRAINCDIRS += $(HALINC)
EXTRAINCDIRS += $(PORTINC)
EXTRAINCDIRS += $(KERNINC)

include $(CHIBIOS_DIR)/os/hal/hal.mk
include $(CHIBIOS_DIR)/os/kernel/kernel.mk

CHIBIOS_VARIOUS_DIR := $(CHIBIOS_DIR)/os/various
CSRC += $(wildcard $(CHIBIOS_VARIOUS_DIR)/*.c)
EXTRAINCDIRS += $(CHIBIOS_VARIOUS_DIR)

include $(QHAL_DIR)/../library.mk
