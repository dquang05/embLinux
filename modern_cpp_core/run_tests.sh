#!/bin/bash
set -e

echo "Running tests according to modern_cpp_core/test_rules.md"

# Go to build directory
mkdir -p build
cd build

# Remove CMakeCache to switch sanitizers cleanly
rm -f CMakeCache.txt

# Configure with TSan
echo "Configuring with ThreadSanitizer..."
cmake -DENABLE_ASAN=OFF -DENABLE_TSAN=ON ..

# Build
echo "Building..."
make -j$(nproc)

# Run tests with strict logging and ASLR workaround
echo "Running tests..."
mkdir -p Testing/Temporary
setarch x86_64 -R ctest -V --output-log Testing/Temporary/LastTest.log

echo "Tests completed. See build/Testing/Temporary/LastTest.log for details."
