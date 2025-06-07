# makefile for booplang

# compiler and flags
CC = gcc
CFLAGS_DEBUG   = -g -Wall -Wextra -pedantic
CFLAGS_RELEASE = -O2 -Wall -Wextra -pedantic

# source and object files
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

# output binary
TARGET = ./boopc

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
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# compile step
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# clean generated files
clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all release debug clean
