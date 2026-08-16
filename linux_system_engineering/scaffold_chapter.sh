#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

SCRIPT_NAME=$(basename "$0")

usage() {
    echo "Usage: ./$SCRIPT_NAME <phase_folder> <chapter_folder>"
    echo "Example: ./$SCRIPT_NAME 01_foundation 01_linux_basic_knowledge"
    exit 1
}

if [ "$#" -ne 2 ]; then
    usage
fi

PHASE_DIR=$1
CHAPTER_DIR=$2
TARGET_DIR="${PHASE_DIR}/${CHAPTER_DIR}"

# Remove numeric prefix and format to Title Case for headings (e.g., "01_linux_basic_knowledge" -> "Linux Basic Knowledge")
CHAPTER_TITLE=$(echo "$CHAPTER_DIR" | sed -E 's/^[0-9]+_//' | tr '_' ' ' | awk '{for(i=1;i<=NF;i++)sub(/./,toupper(substr($i,1,1)),$i)}1')

if [ -d "$TARGET_DIR" ]; then
    echo "❌ Error: Directory '$TARGET_DIR' already exists."
    exit 1
fi

echo "🚀 Scaffolding chapter framework in: $TARGET_DIR ..."

# 1. Create directory structure
mkdir -p "${TARGET_DIR}/docs"
mkdir -p "${TARGET_DIR}/src"
mkdir -p "${TARGET_DIR}/include"

# 2. Create README.md template
cat << EOF > "${TARGET_DIR}/README.md"
# Chapter: $CHAPTER_TITLE

## Objectives
- [Add detailed chapter objectives here]

## References
- **OSC**: Chapter [X]
- **TLPI**: Chapter [Y]

## Usage Instructions
- Read detailed theory in the \`docs/\` directory.
- Review and run practical examples in the \`src/\` directory.
- Use the \`make\` command to compile all examples.
EOF

# 3. Create docs/concepts.md template
cat << EOF > "${TARGET_DIR}/docs/concepts.md"
# Core Concepts: $CHAPTER_TITLE

## 1. First Concept
- Theory notes and API explanations go here...

## 2. Second Concept
- ...
EOF

# 4. Create standard Makefile
cat << 'EOF' > "${TARGET_DIR}/Makefile"
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
EOF

echo "✅ Successfully created chapter framework at: $TARGET_DIR"
