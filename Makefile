CC = clang
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2
BUILD_DIR = ./build
TARGET = $(BUILD_DIR)/testrig
VISUALIZER_TARGET = $(BUILD_DIR)/visualizer
UNIT_TEST_TARGET = $(BUILD_DIR)/unit_tests
VISUALIZER_SRC = visualization/visualizer.c
UNIT_TEST_SRC = tests/unit/test_engine_control.c
UNIT_TEST_DEPS = $(SRC_DIR)/engine.c $(SRC_DIR)/control.c $(SRC_DIR)/hal.c
RAYLIB_LIBS = -lraylib -lm -lpthread -ldl
SRC_DIR = src
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/engine.c \
       $(SRC_DIR)/sensors.c \
       $(SRC_DIR)/control.c \
	$(SRC_DIR)/script_parser.c \
       $(SRC_DIR)/test_runner.c \
	$(SRC_DIR)/logger.c \
	$(SRC_DIR)/hal.c

DEBUG_CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer

all: $(TARGET)

debug: $(BUILD_DIR) $(SRCS)
	$(CC) $(DEBUG_CFLAGS) -o $(TARGET) $(SRCS)

$(TARGET): $(BUILD_DIR) $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

$(VISUALIZER_TARGET): $(BUILD_DIR) $(VISUALIZER_SRC)
	$(CC) $(CFLAGS) -o $(VISUALIZER_TARGET) $(VISUALIZER_SRC) $(RAYLIB_LIBS)

$(UNIT_TEST_TARGET): $(BUILD_DIR) $(UNIT_TEST_SRC) $(UNIT_TEST_DEPS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -o $(UNIT_TEST_TARGET) $(UNIT_TEST_SRC) $(UNIT_TEST_DEPS)

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
		--suppress=missingIncludeSystem \
		--suppress=normalCheckLevelMaxBranches \
		--suppress=checkersReport \
		--suppress=unusedFunction \
		--suppress=staticFunction \
		src/

analyze-clang-tidy:
	clang-tidy src/*.c -checks='-*,clang-analyzer-*,-clang-analyzer-security.*,-clang-analyzer-alpha.*' -warnings-as-errors='*' -- -std=c11

test-unit: $(UNIT_TEST_TARGET)
	$(UNIT_TEST_TARGET)

validate-json-contract: $(TARGET)
	@tmp_json="$(BUILD_DIR)/contract-check.json"; \
	$(TARGET) --run-all --json > "$$tmp_json"; \
	grep -q '"schema_version": "1.0"' "$$tmp_json"; \
	grep -q '"software_version":' "$$tmp_json"; \
	grep -q '"requirement_id":' "$$tmp_json"; \
	grep -q '"ticks":' "$$tmp_json"; \
	grep -q '"summary": {"passed":' "$$tmp_json"

ci-check: all debug analyze-cppcheck analyze-clang-tidy test-unit validate-json-contract
	$(TARGET) --run-all --json > /dev/null

.PHONY: all clean debug run-script run-script-json run-scenarios visualizer run-visualizer analyze-cppcheck analyze-clang-tidy test-unit validate-json-contract ci-check
