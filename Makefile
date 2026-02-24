CC = clang
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2
BUILD_DIR = ./build
TARGET = $(BUILD_DIR)/testrig
SRC_DIR = src
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/engine.c \
       $(SRC_DIR)/sensors.c \
       $(SRC_DIR)/control.c \
       $(SRC_DIR)/test_runner.c \
       $(SRC_DIR)/logger.c

all: $(TARGET)

$(TARGET): $(BUILD_DIR) $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

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

.PHONY: all clean run-script run-script-json run-scenarios
