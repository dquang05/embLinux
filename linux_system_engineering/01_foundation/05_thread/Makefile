CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -I./include -pthread
LDFLAGS = -pthread

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

$(OBJ_DIR)/%: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJ_DIR)

.PHONY: all clean dir
