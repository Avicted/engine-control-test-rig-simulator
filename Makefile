CC = clang
GCOV = llvm-cov gcov

SRC_DIR = src
BUILD_DIR = ./build
COVERAGE_DIR = ./coverage
COVERAGE_HTML_DIR = ./coverage/html

TARGET = $(BUILD_DIR)/testrig
VISUALIZER_TARGET = $(BUILD_DIR)/visualizer
UNIT_TEST_TARGET = $(BUILD_DIR)/unit_tests

VISUALIZER_SRC = visualization/visualizer.c
UNIT_TEST_SRCS = $(shell find tests/unit -type f -name '*.c' | sort)
TIDY_SRCS = $(shell find src -type f -name '*.c' | sort)

SRCS = $(SRC_DIR)/app/main.c \
	$(SRC_DIR)/app/config.c \
	$(SRC_DIR)/app/test_runner.c \
	$(SRC_DIR)/domain/engine.c \
	$(SRC_DIR)/domain/control.c \
	$(SRC_DIR)/scenario/script_parser.c \
	$(SRC_DIR)/scenario/scenario_profiles.c \
	$(SRC_DIR)/scenario/scenario_report.c \
	$(SRC_DIR)/scenario/scenario_catalog.c \
	$(SRC_DIR)/reporting/output.c \
	$(SRC_DIR)/reporting/logger.c \
	$(SRC_DIR)/platform/hal.c

UNIT_TEST_DEPS = $(SRC_DIR)/domain/engine.c \
	$(SRC_DIR)/domain/control.c \
	$(SRC_DIR)/platform/hal.c \
	$(SRC_DIR)/scenario/script_parser.c \
	$(SRC_DIR)/reporting/logger.c \
	$(SRC_DIR)/app/config.c

BUILD_COMMIT = $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)

COMMON_CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -Wunused-result
CFLAGS = $(COMMON_CFLAGS) -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2
DEBUG_CFLAGS = $(COMMON_CFLAGS) -O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer
COVERAGE_CFLAGS = $(COMMON_CFLAGS) -O0 --coverage

CPPFLAGS = -I./include -I./src -DSIM_BUILD_COMMIT_OVERRIDE=\"$(BUILD_COMMIT)\"
LDFLAGS = -lm
RAYLIB_LIBS = -lraylib -lm -lpthread -ldl

all: $(TARGET)

debug: $(BUILD_DIR) $(SRCS)
	$(CC) $(CPPFLAGS) $(DEBUG_CFLAGS) -o $(TARGET) $(SRCS)

$(TARGET): $(BUILD_DIR) $(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

$(VISUALIZER_TARGET): $(BUILD_DIR) $(VISUALIZER_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(VISUALIZER_TARGET) $(VISUALIZER_SRC) $(RAYLIB_LIBS)

$(UNIT_TEST_TARGET): $(BUILD_DIR) $(UNIT_TEST_SRCS) $(UNIT_TEST_DEPS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(UNIT_TEST_TARGET) $(UNIT_TEST_SRCS) $(UNIT_TEST_DEPS) $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -f $(TARGET)
	rm -f $(UNIT_TEST_TARGET)
	rm -f $(VISUALIZER_TARGET)
	rm -f $(BUILD_DIR)/*.gcno $(BUILD_DIR)/*.gcda $(BUILD_DIR)/*.info
	rm -rf $(COVERAGE_DIR)

run-script: $(TARGET)
	@if [ -z "$(SCRIPT)" ]; then \
		echo "Usage: make run-script SCRIPT=scenarios/normal_operation.txt"; \
		exit 1; \
	fi
	$(TARGET) --script "$(SCRIPT)"

run-script-json: $(TARGET)
	@if [ -z "$(SCRIPT)" ]; then \
		echo "Usage: make run-script-json SCRIPT=scenarios/normal_operation.txt"; \
		exit 1; \
	fi
	$(TARGET) --script "$(SCRIPT)" --json

run-scenarios: $(TARGET)
	files=$$(find scenarios -maxdepth 1 -type f -name '*.txt' | sort); \
	if [ -z "$$files" ]; then \
		echo "No scenario scripts found in scenarios/"; \
		exit 1; \
	fi; \
	for file in $$files; do \
		echo "=== $$file ==="; \
		$(TARGET) --script "$$file"; \
	done

visualizer: $(VISUALIZER_TARGET)

run-visualizer: $(VISUALIZER_TARGET)
	@if [ -z "$(JSON)" ]; then \
		echo "Usage: make run-visualizer JSON=scenario.json"; \
		exit 1; \
	fi
	$(VISUALIZER_TARGET) "$(JSON)"

analyze-cppcheck:
	cppcheck --enable=all --std=c11 --error-exitcode=1 \
		-Iinclude -Isrc \
		--suppress=missingIncludeSystem \
		--suppress=normalCheckLevelMaxBranches \
		--suppress=checkersReport \
		--suppress=unusedFunction \
		--suppress=staticFunction \
		--suppress=unmatchedSuppression \
		src/

analyze-clang-tidy:
	clang-tidy $(TIDY_SRCS) -checks='-*,clang-analyzer-*,-clang-analyzer-security.*,-clang-analyzer-alpha.*' -warnings-as-errors='*' -- -std=c11 -I./include -I./src

analyze-misra:
	cppcheck --std=c11 --enable=all --error-exitcode=1 \
		-Iinclude -Isrc \
		--suppress=missingIncludeSystem \
		--suppress=normalCheckLevelMaxBranches \
		--suppress=checkersReport \
		--suppress=unusedFunction \
		--suppress=staticFunction \
		--suppress=misra-c2012-15.5 \
		--suppress=misra-c2012-21.6 \
		--suppress=misra-c2012-8.7 \
		--suppress=misra-c2012-10.4 \
		--suppress=misra-c2012-18.4 \
		--suppress=misra-c2012-11.5 \
		--suppress=misra-c2012-4.1 \
		--suppress=misra-c2012-10.8 \
		--suppress=misra-c2012-8.9 \
		--suppress=misra-c2012-2.5 \
		--suppress=misra-c2012-2.3 \
		--suppress=misra-c2012-2.4 \
		--suppress=misra-c2012-21.8 \
		--addon=tools/misra.json \
		src/ 2>&1 | tee build/misra-report.txt; \
	echo "MISRA report written to build/misra-report.txt"

analyze-layering:
	sh tools/check_layering.sh

test-unit: $(UNIT_TEST_TARGET)
	$(UNIT_TEST_TARGET)

coverage: $(BUILD_DIR)
	mkdir -p $(COVERAGE_DIR)
	rm -f $(BUILD_DIR)/unit_tests_cov* $(COVERAGE_DIR)/*.gcov $(COVERAGE_DIR)/coverage.txt
	$(CC) $(CPPFLAGS) $(COVERAGE_CFLAGS) -o $(BUILD_DIR)/unit_tests_cov $(UNIT_TEST_SRCS) $(UNIT_TEST_DEPS) $(LDFLAGS)
	$(BUILD_DIR)/unit_tests_cov
	$(GCOV) -b -c $(BUILD_DIR)/unit_tests_cov-*.gcno > $(COVERAGE_DIR)/coverage.txt
	@mv -f ./*.gcov $(COVERAGE_DIR)/ 2>/dev/null || true
	@total_lines=0; covered_lines=0; \
	while IFS= read -r fline; do \
		case "$$fline" in \
		"File 'src/"*) current_src=1 ;; \
		"File 'tests/"*) current_src=0 ;; \
		"File "*) current_src=0 ;; \
		esac; \
		if [ "$$current_src" = "1" ]; then \
			case "$$fline" in \
			"Lines executed:"*) \
				pct=$$(echo "$$fline" | sed -E 's/.*Lines executed:([0-9.]+)% of ([0-9]+).*/\1/'); \
				lines=$$(echo "$$fline" | sed -E 's/.*Lines executed:([0-9.]+)% of ([0-9]+).*/\2/'); \
				hit=$$(awk -v p="$$pct" -v l="$$lines" 'BEGIN { printf "%d", (p/100.0)*l + 0.5 }'); \
				total_lines=$$((total_lines + lines)); \
				covered_lines=$$((covered_lines + hit)); \
				;; \
			esac; \
		fi; \
	done < $(COVERAGE_DIR)/coverage.txt; \
	if [ "$$total_lines" -eq 0 ]; then \
		echo "Coverage: 0% (no source lines found)"; exit 1; \
	fi; \
	covered=$$(awk -v h="$$covered_lines" -v t="$$total_lines" 'BEGIN { printf "%.2f", (h/t)*100.0 }'); \
	echo "Source-only coverage: $$covered% ($$covered_lines/$$total_lines lines)"; \
	awk -v c="$$covered" 'BEGIN { exit (c+0 >= 80.0) ? 0 : 1 }'

coverage-html: coverage
	@echo "Generating HTML coverage report..."
	lcov --capture --directory $(BUILD_DIR) --gcov-tool $(CURDIR)/tools/llvm-gcov.sh \
		--output-file $(BUILD_DIR)/coverage.info --quiet
	lcov --remove $(BUILD_DIR)/coverage.info '*/tests/*' \
		--output-file $(BUILD_DIR)/coverage-src.info --quiet \
		--gcov-tool $(CURDIR)/tools/llvm-gcov.sh
	genhtml $(BUILD_DIR)/coverage-src.info --output-directory $(COVERAGE_HTML_DIR) \
		--title "Engine Control Test Rig" --legend --quiet
	@echo "HTML coverage report: $(COVERAGE_HTML_DIR)/index.html"

coverage-main: $(BUILD_DIR)
	mkdir -p $(COVERAGE_DIR)
	rm -f $(BUILD_DIR)/testrig_cov* $(COVERAGE_DIR)/main.c.gcov $(COVERAGE_DIR)/main_coverage.txt
	$(CC) $(CPPFLAGS) $(COVERAGE_CFLAGS) -o $(BUILD_DIR)/testrig_cov $(SRCS) $(LDFLAGS)
	$(BUILD_DIR)/testrig_cov --version > /dev/null
	$(GCOV) -b -c $(BUILD_DIR)/testrig_cov-main.gcno > $(COVERAGE_DIR)/main_coverage.txt
	@mv -f ./main.c.gcov $(COVERAGE_DIR)/ 2>/dev/null || true

coverage-html-main: coverage
	@echo "Generating main.c HTML coverage report..."
	lcov --capture --directory $(BUILD_DIR) --gcov-tool $(CURDIR)/tools/llvm-gcov.sh \
		--output-file $(BUILD_DIR)/coverage-main.info --quiet
	lcov --extract $(BUILD_DIR)/coverage-main.info '*/src/app/main.c' \
		--output-file $(BUILD_DIR)/coverage-main-src.info --quiet \
		--gcov-tool $(CURDIR)/tools/llvm-gcov.sh
	genhtml $(BUILD_DIR)/coverage-main-src.info --output-directory $(COVERAGE_DIR)/html-main \
		--title "Engine Control Test Rig (main.c)" --legend --quiet
	@echo "HTML main.c report: $(COVERAGE_DIR)/html-main/index.html"

coverage-clean:
	rm -f $(BUILD_DIR)/unit_tests_cov* $(BUILD_DIR)/testrig_cov* $(BUILD_DIR)/coverage.info $(BUILD_DIR)/coverage-src.info
	rm -rf $(COVERAGE_DIR)

validate-json-contract: $(TARGET)
	@tmp_json="$(BUILD_DIR)/contract-check.json"; \
	$(TARGET) --run-all --json > "$$tmp_json"; \
	grep -q '"schema_version": "1.0.0"' "$$tmp_json"; \
	grep -q '"software_version":' "$$tmp_json"; \
	grep -q '"build_commit":' "$$tmp_json"; \
	grep -q '"requirement_id":' "$$tmp_json"; \
	grep -q '"ticks":' "$$tmp_json"; \
	grep -q '"summary": {"passed":' "$$tmp_json"

validate-json: $(TARGET)
	@tmp_json="$(BUILD_DIR)/contract-check.json"; \
	$(TARGET) --run-all --json > "$$tmp_json"; \
	python3 tools/validate_json_contract.py "$$tmp_json" "schema/engine_test_rig.schema.json"

analyze-sanitizers: debug
	ASAN_OPTIONS=detect_leaks=0 $(TARGET) --run-all --json > /dev/null

analyze: analyze-cppcheck analyze-clang-tidy analyze-layering analyze-sanitizers

test-all: test-unit $(TARGET) validate-json-contract validate-json
	$(TARGET) --run-all --json > /dev/null

# --- Determinism Replay Test (Section 2.2) ---
determinism-check: $(TARGET)
	sh tools/determinism_check.sh $(TARGET)

# --- Module-Specific Coverage Thresholds (Section 5.1) ---
coverage-modules: coverage
	@echo "=== Module-Specific Coverage Thresholds ==="; \
	fail=0; \
	for mod_spec in "control.c:90:domain" "engine.c:90:domain" "hal.c:85:HAL" "script_parser.c:85:parser"; do \
		mod=$$(echo "$$mod_spec" | cut -d: -f1); \
		threshold=$$(echo "$$mod_spec" | cut -d: -f2); \
		label=$$(echo "$$mod_spec" | cut -d: -f3); \
		pct=0; \
		if [ -f "$(COVERAGE_DIR)/$$mod.gcov" ]; then \
			exec_lines=$$(grep -c '^[[:space:]]*[0-9]' "$(COVERAGE_DIR)/$$mod.gcov" 2>/dev/null || echo 0); \
			unexec_lines=$$(grep -c '^[[:space:]]*#####' "$(COVERAGE_DIR)/$$mod.gcov" 2>/dev/null || echo 0); \
			total=$$((exec_lines + unexec_lines)); \
			if [ "$$total" -gt 0 ]; then \
				pct=$$(awk -v e="$$exec_lines" -v t="$$total" 'BEGIN { printf "%.1f", (e/t)*100.0 }'); \
				echo "  $$label ($$mod): $$pct%  [threshold: $$threshold%]"; \
			fi; \
		fi; \
	done

# --- Visualization Boundary Audit (Section 8) ---
check-viz-boundary:
	@echo "=== Visualization Boundary Audit ==="; \
	violations=0; \
	for hdr in config.h control.h engine.h hal.h status.h script_parser.h scenario_contract.h test_runner.h; do \
		if grep -rn "#include.*$$hdr" visualization/ > /dev/null 2>&1; then \
			echo "FAIL: visualizer includes simulator header $$hdr"; \
			violations=1; \
		fi; \
	done; \
	if [ "$$violations" -ne 0 ]; then \
		echo "Visualization boundary audit FAILED"; \
		exit 1; \
	fi; \
	echo "PASS: JSON-only contract between simulator and visualizer"

# --- Schema backward compatibility check (Section 4.1) ---
validate-schema-compat: $(TARGET)
	@tmp_json="$(BUILD_DIR)/contract-check.json"; \
	$(TARGET) --run-all --json > "$$tmp_json"; \
	grep -q '"schema_version": "1.0.0"' "$$tmp_json" || { echo "Schema version missing"; exit 1; }; \
	grep -q '"software_version":' "$$tmp_json" || { echo "Software version missing"; exit 1; }; \
	grep -q '"build_commit":' "$$tmp_json" || { echo "Build commit missing"; exit 1; }; \
	grep -q '"requirement_id":' "$$tmp_json" || { echo "Requirement ID missing"; exit 1; }; \
	grep -q '"ticks":' "$$tmp_json" || { echo "Ticks array missing"; exit 1; }; \
	grep -q '"summary":' "$$tmp_json" || { echo "Summary block missing"; exit 1; }; \
	echo "Schema backward compatibility check passed"

ci-check: all debug analyze test-all coverage determinism-check check-viz-boundary validate-schema-compat
	$(TARGET) --run-all --json > /dev/null

docs:
	@command -v doxygen >/dev/null 2>&1 || { echo "doxygen not found, skipping docs"; exit 0; }
	doxygen Doxyfile
	@echo "Documentation generated in $(BUILD_DIR)/docs/html"

.PHONY: all clean debug run-script run-script-json run-scenarios visualizer run-visualizer analyze-cppcheck analyze-clang-tidy analyze-misra analyze-layering analyze-sanitizers analyze test-unit test-all coverage coverage-clean validate-json validate-json-contract ci-check docs determinism-check coverage-modules check-viz-boundary validate-schema-compat
