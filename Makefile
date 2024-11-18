CC = gcc
CFLAGS = -Wall -Werror -g
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

SRC = $(wildcard $(SRC_DIR)/*.c)  
OBJ = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))  

TARGET = boop

$(BUILD_DIR)/$(TARGET): $(OBJ)  
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c  
	@mkdir -p $(BUILD_DIR)  
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

clean:  
	rm -rf $(BUILD_DIR)

.PHONY: clean
