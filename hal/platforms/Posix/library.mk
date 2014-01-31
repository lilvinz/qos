

QHAL_POSIX_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
CSRC += $(wildcard $(QHAL_POSIX_DIR)/*.c)
EXTRAINCDIRS += $(QHAL_POSIX_DIR)

# ChibiOS
include $(ROOT_DIR)/src/lib/ChibiOS/os/hal/platforms/Posix/platform.mk
include $(QHAL_POSIX_DIR)/../../../ports/GCC/SIMIA32/port.mk

include $(QHAL_POSIX_DIR)/../../library.mk
