CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wshadow -Werror=float-conversion
BUILD    := build

$(BUILD)/host_tests: host_tests.cpp $(wildcard ../dsp/*.hpp)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: run clean analyze
run: $(BUILD)/host_tests
	@mkdir -p out
	./$(BUILD)/host_tests out

analyze: run
	python3 analyze.py out

clean:
	rm -rf $(BUILD) out
