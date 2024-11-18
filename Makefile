# compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -g

# directories
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

# source and object files
SRC = $(wildcard $(SRC_DIR)/*.c)  # find all .c files in src/
OBJ = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))  # map src/*.c -> build/*.o

# target executable
TARGET = boop

# rules
$(BUILD_DIR)/$(TARGET): $(OBJ)  # build final executable
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c  # compile .c -> .o in build/
	@mkdir -p $(BUILD_DIR)  # ensure build/ directory exists
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

clean:  # clean build artifacts
	rm -rf $(BUILD_DIR)

.PHONY: clean
