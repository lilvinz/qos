
# ChibiOS default path if not specified
CHIBIOS_DIR ?= $(ROOT_DIR)/submodules/chibios
CHIBIOS := $(CHIBIOS_DIR)

QHAL_POSIX_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CSRC += $(wildcard $(QHAL_POSIX_DIR)/*.c)
EXTRAINCDIRS += $(QHAL_POSIX_DIR)

include $(QHAL_POSIX_DIR)/../library.mk
