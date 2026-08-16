# Build configuration
CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -I./include
LDFLAGS = -ldl

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

# Targets
STATIC_LIB = $(BUILD_DIR)/libmath.a
SHARED_LIB = $(BUILD_DIR)/libmath.so
ERROR_DEMO = $(BUILD_DIR)/error_handling_demo
STATIC_APP = $(BUILD_DIR)/app_static
SHARED_APP = $(BUILD_DIR)/app_shared
DLOPEN_APP = $(BUILD_DIR)/app_dlopen

all: $(BUILD_DIR) $(STATIC_LIB) $(SHARED_LIB) $(ERROR_DEMO) $(STATIC_APP) $(SHARED_APP) $(DLOPEN_APP)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(ERROR_DEMO): $(SRC_DIR)/error_handling_demo.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/math_lib.o: $(SRC_DIR)/math_lib.c $(INC_DIR)/math_lib.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(STATIC_LIB): $(BUILD_DIR)/math_lib.o | $(BUILD_DIR)
	ar rcs $@ $<

$(SHARED_LIB): $(BUILD_DIR)/math_lib.o | $(BUILD_DIR)
	$(CC) -shared -o $@ $<

$(STATIC_APP): $(SRC_DIR)/library_demo.c $(STATIC_LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< ./$(STATIC_LIB) -o $@

$(SHARED_APP): $(SRC_DIR)/library_demo.c $(SHARED_LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lmath -Wl,-rpath=./$(BUILD_DIR) -o $@

$(DLOPEN_APP): $(SRC_DIR)/dlopen_demo.c $(SHARED_LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
