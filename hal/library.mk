

QHAL_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
CSRC += $(wildcard $(QHAL_DIR)/src/*.c)
EXTRAINCDIRS += $(QHAL_DIR)/include

# ChibiOS
CHIBIOS := $(ROOT_DIR)/src/lib/ChibiOS/

CSRC += $(PLATFORMSRC)
CSRC += $(HALSRC)
CSRC += $(PORTSRC)
CSRC += $(KERNSRC)

EXTRAINCDIRS += $(PLATFORMINC)
EXTRAINCDIRS += $(HALINC)
EXTRAINCDIRS += $(PORTINC)
EXTRAINCDIRS += $(KERNINC)

include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/kernel/kernel.mk

CHIBIOS_VARIOUS_DIR := $(CHIBIOS)/os/various
CSRC += $(wildcard $(CHIBIOS_VARIOUS_DIR)/*.c)
EXTRAINCDIRS += $(CHIBIOS_VARIOUS_DIR)

