
QOS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

CSRC += $(wildcard $(QOS_DIR)/src/*.c)
EXTRAINCDIRS += $(QOS_DIR)/include
