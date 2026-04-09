#
# Command examples:
#
#   make                        Build the project (debug is enabled by default)
#   make DEBUG=0                Build the project without debug symbols and with optimizations
#   make SANITIZER=1            Build the project with AddressSanitizer enabled
#   make clean                  Clean build artifacts
#
# Authors: nsix
#

TARGET := samples

# Directories

BUILD_DIR := build

SRC_DIR := src
INC_DIR := src

# Options

DEBUG ?= 1
SANITIZER ?= 0

# Compiler

CC = gcc

LDFLAGS = -lSDL3 -lSDL3_ttf -lm

CFLAGS = -std=c99 -Wall

INC_FLAGS := -I$(INC_DIR)

ifeq ($(DEBUG), 1)
CFLAGS += -g -O0 -DDEBUG $(INC_FLAGS)
else
CFLAGS += -O2 $(INC_FLAGS)
endif

ifeq ($(SANITIZER), 1)
CFLAGS += -fsanitize=address -fno-omit-frame-pointer
CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address
endif

# Source Files

SRCS := $(wildcard $(SRC_DIR)/*.c)

# Object Files

OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Targets

.PHONY: all clean
.DEFAULT_GOAL := all

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
