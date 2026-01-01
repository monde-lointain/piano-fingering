.PHONY: help build test docs package clean distclean reconfig
.PHONY: format format-patch
.PHONY: cppcheck cppcheck-xml scan-build tidy
.PHONY: complexity complexity-full complexity-xml
.PHONY: coverage
.PHONY: cloc cloc-full cloc-report cloc-report-full
.PHONY: vale

BUILD_DIR := build

.DEFAULT_GOAL := help

# Help target
help:
	@echo "Piano Fingering Generator - Build Targets"
	@echo ""
	@echo "Core:"
	@echo "  help              Show this help message (default)"
	@echo "  build             Build the project"
	@echo "  test              Build and run unit tests"
	@echo "  docs              Generate Doxygen documentation"
	@echo "  package           Build release package"
	@echo "  clean             Clean build artifacts"
	@echo "  distclean         Remove build directory"
	@echo "  reconfig          Reconfigure CMake build"
	@echo ""
	@echo "Formatting:"
	@echo "  format            Run clang-format on sources"
	@echo "  format-patch      Generate patch with formatting changes"
	@echo ""
	@echo "Static Analysis:"
	@echo "  cppcheck          Run cppcheck"
	@echo "  cppcheck-xml      Run cppcheck with XML report"
	@echo "  scan-build        Run clang static analyzer"
	@echo "  tidy              Run clang-tidy linter"
	@echo ""
	@echo "Complexity:"
	@echo "  complexity        Run lizard (violations only)"
	@echo "  complexity-full   Run lizard (full report)"
	@echo "  complexity-xml    Run lizard with XML report"
	@echo ""
	@echo "Coverage:"
	@echo "  coverage          Generate code coverage report (Linux/macOS only)"
	@echo ""
	@echo "SLOC:"
	@echo "  cloc              Count lines of code (src only)"
	@echo "  cloc-full         Count lines of code (all files)"
	@echo "  cloc-report       Generate SLOC CSV report (src only)"
	@echo "  cloc-report-full  Generate SLOC CSV report (all files)"
	@echo ""
	@echo "Documentation:"
	@echo "  vale              Run Vale documentation linter"

build: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR)

test: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR)
	@cd $(BUILD_DIR) && ctest --output-on-failure

docs: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target docs

package: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR)
	@cd $(BUILD_DIR) && cpack

clean:
	@if [ -d $(BUILD_DIR) ]; then cmake --build $(BUILD_DIR) --target clean; fi

distclean:
	@rm -rf $(BUILD_DIR)

reconfig:
	@rm -rf $(BUILD_DIR)
	@cmake -B $(BUILD_DIR) -S .

# Formatting targets
format: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target format

format-patch: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target format-patch

# Static analysis targets
cppcheck: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target cppcheck

cppcheck-xml: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target cppcheck-xml

scan-build: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target scan-build

tidy: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target tidy

# Complexity targets
complexity: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target complexity

complexity-full: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target complexity-full

complexity-xml: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target complexity-xml

# Coverage target (requires reconfiguration with ENABLE_COVERAGE=ON)
coverage:
	@cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
	@cmake --build $(BUILD_DIR)
	@cmake --build $(BUILD_DIR) --target coverage

# SLOC targets
cloc: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target cloc

cloc-full: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target cloc-full

cloc-report: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target cloc-report

cloc-report-full: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target cloc-report-full

# Documentation linting
vale: $(BUILD_DIR)/Makefile
	@cmake --build $(BUILD_DIR) --target vale

# Build directory setup
$(BUILD_DIR)/Makefile:
	@cmake -B $(BUILD_DIR) -S .
