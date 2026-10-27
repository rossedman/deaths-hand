# VCV Rack 2 plugin. Point RACK_DIR at an unpacked Rack SDK:
#   make RACK_DIR=~/Rack-SDK install
RACK_DIR ?= ../../Rack-SDK

FLAGS    += -I../dsp
CXXFLAGS += -std=c++17

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += res/ComplexOsc.svg
DISTRIBUTABLES += $(wildcard LICENSE*)

include $(RACK_DIR)/plugin.mk
