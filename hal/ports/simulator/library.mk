
QHAL_SIMULATOR_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CSRC += $(wildcard $(QHAL_SIMULATOR_DIR)/*.c)
EXTRAINCDIRS += $(QHAL_SIMULATOR_DIR)

include $(QHAL_SIMULATOR_DIR)/../../library.mk
