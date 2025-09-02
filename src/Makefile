# makefile for booplang

# compiler
CC = gcc

# detect llvm-config
LLVM_CONFIG := $(shell which llvm-config 2>/dev/null || echo /opt/homebrew/opt/llvm/bin/llvm-config)

# test if llvm-config is executable
LLVM_CONFIG_OK := $(shell [ -x $(LLVM_CONFIG) ] && echo yes || echo no)

# llvm flags
ifeq ($(LLVM_CONFIG_OK),yes)
    LLVM_CFLAGS  := $(shell $(LLVM_CONFIG) --cflags)
    LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags --libs core --system-libs)
else
    $(warning "llvm-config not found! You may need to 'brew install llvm' and set PATH correctly.")
    LLVM_CFLAGS  :=
    LLVM_LDFLAGS :=
endif

# flags
CFLAGS_DEBUG   = -g -Wall -Wextra -pedantic $(LLVM_CFLAGS)
CFLAGS_RELEASE = -O2 -Wall -Wextra -pedantic $(LLVM_CFLAGS)

# build directory
BUILD_DIR = build

# source and object files
SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC))

# output binary
TARGET = $(BUILD_DIR)/boopc

# create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# default target: release
all: release

# release build
release: CFLAGS = $(CFLAGS_RELEASE)
release: $(TARGET)

# debug build
debug: CFLAGS = $(CFLAGS_DEBUG)
debug: $(TARGET)

# link step
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LLVM_LDFLAGS)

# compile step
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# clean generated files
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all release debug clean
