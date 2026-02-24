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

.PHONY: all clean
