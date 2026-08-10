#pragma once

#include <iostream>
#include <cstdlib>

// CORE_ASSERT does not compile out in Release mode (NDEBUG).
// It ensures that test assertions are always evaluated.
#define CORE_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: (" << #condition << "), " \
                      << "function " << __builtin_FUNCTION() \
                      << ", file " << __FILE__ \
                      << ", line " << __LINE__ << ".\n"; \
            std::exit(1); \
        } \
    } while (false)
