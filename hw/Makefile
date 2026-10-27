# patch.Init() firmware. Requires the ARM toolchain (arm-none-eabi-gcc) and
# libDaisy checked out at ../lib/libDaisy (git submodule, see README).
# Two programs share this directory; APP picks one.
#
#   make -C lib/libDaisy                     # once
#   make -C hw                               # Death's Hand -> build/deaths-hand.bin
#   make -C hw APP=doombringer               # Doombringer  -> build/doombringer.bin
#   make -C hw APP=doombringer program-dfu   # flash over USB (hold BOOT, tap RESET first)

APP ?= deaths-hand
ifeq ($(APP),deaths-hand)
CPP_SOURCES = main.cpp
else ifeq ($(APP),doombringer)
CPP_SOURCES = doombringer.cpp
else
$(error APP must be deaths-hand or doombringer, not $(APP))
endif

TARGET        = $(APP)
CPP_STANDARD  = -std=gnu++17
OPT           = -O2

# make CPU_METER=1 prints DSP load over USB serial once a second.
CPU_METER    ?= 0
C_DEFS       += -DCPU_METER=$(CPU_METER)

LIBDAISY_DIR     = ../lib/libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
