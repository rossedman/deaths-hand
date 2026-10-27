CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wshadow -Werror=float-conversion
BUILD    := build

$(BUILD)/host_tests: host_tests.cpp patches.hpp $(wildcard ../dsp/*.hpp)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BUILD)/render: render.cpp patches.hpp $(wildcard ../dsp/*.hpp)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: run clean analyze renders
run: $(BUILD)/host_tests
	@mkdir -p out
	./$(BUILD)/host_tests out

# The docs' listening page: the six patches and the showcase clips as WAVs.
renders: $(BUILD)/render
	@mkdir -p out/listen
	./$(BUILD)/render out/listen

analyze: run
	python3 analyze.py out

clean:
	rm -rf $(BUILD) out
