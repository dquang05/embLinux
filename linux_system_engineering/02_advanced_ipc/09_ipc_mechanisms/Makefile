CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -I./include
LDFLAGS = -pthread
LDLIBS = -lrt

SRC_DIR = src
OBJ_DIR = build
SOURCES = $(wildcard $(SRC_DIR)/*.c)
TARGETS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%)

# Do not build if no source files exist
ifeq ($(strip $(SOURCES)),)
all:
	@echo "No source files found in $(SRC_DIR). Nothing to build."
else
all: dir $(TARGETS)
endif

dir:
	@mkdir -p $(OBJ_DIR)

# Build each .c file into its own executable
$(OBJ_DIR)/%: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -rf $(OBJ_DIR)

.PHONY: all clean dir
