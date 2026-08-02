#ifndef MODERN_CPP_CORE_CONCURRENCY_SEMAPHORE_HPP
#define MODERN_CPP_CORE_CONCURRENCY_SEMAPHORE_HPP

#include <semaphore>
#include <cstddef>

namespace core::concurrency {

/**
 * @brief A counting semaphore.
 * 
 * A synchronization primitive that controls access to a finite number of resources.
 * Uses C++20 std::counting_semaphore.
 * 
 * @tparam LeastMaxValue The minimum maximum value the semaphore can hold.
 */
template <std::ptrdiff_t LeastMaxValue>
using CountingSemaphore = std::counting_semaphore<LeastMaxValue>;

/**
 * @brief A binary semaphore.
 * 
 * A specialization of CountingSemaphore with a maximum value of 1.
 * Useful as a fast, lightweight mutex or for single-resource signaling.
 * Uses C++20 std::binary_semaphore.
 */
using BinarySemaphore = std::binary_semaphore;

} // namespace core::concurrency

#endif // MODERN_CPP_CORE_CONCURRENCY_SEMAPHORE_HPP
