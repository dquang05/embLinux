#pragma once

#include <iostream>
#include <cstdlib>

// CORE_ASSERT does not compile out in Release mode (NDEBUG).
// It ensures that test assertions are always evaluated.
#define CORE_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "[FAIL] Assertion failed: (" << #condition << "), " \
                      << "function " << __builtin_FUNCTION() \
                      << ", file " << __FILE__ \
                      << ", line " << __LINE__ << ".\n"; \
            std::exit(1); \
        } \
    } while (false)

#define CORE_PASS(test_name) \
    std::cout << "[PASS] " << test_name << "\n"

#define CORE_FAIL(test_name, reason) \
    do { \
        std::cerr << "[FAIL] " << test_name << ": " << reason << "\n"; \
        std::exit(1); \
    } while (false)
