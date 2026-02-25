CC = clang
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2
BUILD_COMMIT = $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
CPPFLAGS = -I./include -I./src -DSIM_BUILD_COMMIT_OVERRIDE=\"$(BUILD_COMMIT)\"
BUILD_DIR = ./build
TARGET = $(BUILD_DIR)/testrig
VISUALIZER_TARGET = $(BUILD_DIR)/visualizer
UNIT_TEST_TARGET = $(BUILD_DIR)/unit_tests
VISUALIZER_SRC = visualization/visualizer.c
UNIT_TEST_SRC = tests/unit/test_engine_control.c
UNIT_TEST_DEPS = $(SRC_DIR)/domain/engine.c $(SRC_DIR)/domain/control.c $(SRC_DIR)/platform/hal.c $(SRC_DIR)/scenario/script_parser.c
RAYLIB_LIBS = -lraylib -lm -lpthread -ldl
SRC_DIR = src
SRCS = $(SRC_DIR)/app/main.c \
       $(SRC_DIR)/domain/engine.c \
	$(SRC_DIR)/legacy/sensors.c \
	$(SRC_DIR)/domain/control.c \
	$(SRC_DIR)/scenario/script_parser.c \
	$(SRC_DIR)/scenario/scenario_profiles.c \
	$(SRC_DIR)/scenario/scenario_report.c \
	$(SRC_DIR)/scenario/scenario_catalog.c \
	$(SRC_DIR)/reporting/output.c \
       $(SRC_DIR)/app/test_runner.c \
	$(SRC_DIR)/reporting/logger.c \
	$(SRC_DIR)/platform/hal.c

TIDY_SRCS = $(shell find src -type f -name '*.c' | sort)

DEBUG_CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer

all: $(TARGET)

debug: $(BUILD_DIR) $(SRCS)
	$(CC) $(CPPFLAGS) $(DEBUG_CFLAGS) -o $(TARGET) $(SRCS)

$(TARGET): $(BUILD_DIR) $(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(TARGET) $(SRCS)

$(VISUALIZER_TARGET): $(BUILD_DIR) $(VISUALIZER_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(VISUALIZER_TARGET) $(VISUALIZER_SRC) $(RAYLIB_LIBS)

$(UNIT_TEST_TARGET): $(BUILD_DIR) $(UNIT_TEST_SRC) $(UNIT_TEST_DEPS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(UNIT_TEST_TARGET) $(UNIT_TEST_SRC) $(UNIT_TEST_DEPS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -f $(TARGET)
	rm -f $(UNIT_TEST_TARGET)
	rm -f $(VISUALIZER_TARGET)

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
	@set -e; \
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
		src/

analyze-clang-tidy:
	clang-tidy $(TIDY_SRCS) -checks='-*,clang-analyzer-*,-clang-analyzer-security.*,-clang-analyzer-alpha.*' -warnings-as-errors='*' -- -std=c11 -I./include -I./src

analyze-layering:
	sh tools/check_layering.sh

test-unit: $(UNIT_TEST_TARGET)
	$(UNIT_TEST_TARGET)

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

ci-check: all debug analyze test-all
	$(TARGET) --run-all --json > /dev/null

.PHONY: all clean debug run-script run-script-json run-scenarios visualizer run-visualizer analyze-cppcheck analyze-clang-tidy analyze-layering analyze-sanitizers analyze test-unit test-all validate-json validate-json-contract ci-check
