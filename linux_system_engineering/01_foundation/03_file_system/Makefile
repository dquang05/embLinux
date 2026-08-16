# Build configuration
CC = gcc
CFLAGS = -Wall -Wextra -Werror -g
BUILD_DIR = build
SRC_DIR = src

# Targets
FILE_IO = $(BUILD_DIR)/file_io_demo
LINKS   = $(BUILD_DIR)/links_demo
INOTIFY = $(BUILD_DIR)/inotify_demo

all: $(BUILD_DIR) $(FILE_IO) $(LINKS) $(INOTIFY)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(FILE_IO): $(SRC_DIR)/file_io_demo.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(LINKS): $(SRC_DIR)/links_demo.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(INOTIFY): $(SRC_DIR)/inotify_demo.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
