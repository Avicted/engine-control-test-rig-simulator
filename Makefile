CC = clang
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2
BUILD_DIR = ./build
TARGET = $(BUILD_DIR)/testrig
VISUALIZER_TARGET = $(BUILD_DIR)/visualizer
VISUALIZER_SRC = visualization/visualizer.c
RAYLIB_LIBS = -lraylib -lm -lpthread -ldl
SRC_DIR = src
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/engine.c \
       $(SRC_DIR)/sensors.c \
       $(SRC_DIR)/control.c \
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

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -f $(TARGET)

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

.PHONY: all clean debug run-script run-script-json run-scenarios visualizer run-visualizer analyze-cppcheck analyze-clang-tidy
