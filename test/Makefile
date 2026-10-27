CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wshadow -Werror=float-conversion
BUILD    := build

$(BUILD)/host_tests: host_tests.cpp patches.hpp $(wildcard ../dsp/*.hpp)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BUILD)/doom_tests: doom_tests.cpp doom_patches.hpp $(wildcard ../dsp/*.hpp)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BUILD)/render: render.cpp patches.hpp doom_patches.hpp $(wildcard ../dsp/*.hpp)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: run run-osc run-doom clean analyze renders
# Both suites run even if the first fails, and either failing fails the target.
run: $(BUILD)/host_tests $(BUILD)/doom_tests
	@mkdir -p out
	@rc=0; ./$(BUILD)/host_tests out || rc=1; ./$(BUILD)/doom_tests out || rc=1; exit $$rc

run-osc: $(BUILD)/host_tests
	@mkdir -p out
	./$(BUILD)/host_tests out

run-doom: $(BUILD)/doom_tests
	@mkdir -p out
	./$(BUILD)/doom_tests out

# The docs' listening page: the patches and the showcase clips of both modules as WAVs.
renders: $(BUILD)/render
	@mkdir -p out/listen
	./$(BUILD)/render out/listen

analyze: run
	python3 analyze.py out

clean:
	rm -rf $(BUILD) out
