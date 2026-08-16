CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -I./include

SRC_DIR = src
OBJ_DIR = build
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = $(OBJ_DIR)/demo_app

# Do not build if no source files exist
ifeq ($(strip $(SOURCES)),)
all:
	@echo "No source files found in $(SRC_DIR). Nothing to build."
else
all: dir $(TARGET)
endif

dir:
	@mkdir -p $(OBJ_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

.PHONY: all clean dir
