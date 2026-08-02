#ifndef MODERN_CPP_CORE_CONCURRENCY_LATCH_BARRIER_HPP
#define MODERN_CPP_CORE_CONCURRENCY_LATCH_BARRIER_HPP

#include <latch>
#include <barrier>

namespace core::concurrency {

/**
 * @brief A count-down latch.
 * 
 * A latch is a synchronization primitive that allows one or more threads to wait until
 * a given number of operations have been performed by other threads.
 * Uses C++20 std::latch.
 */
using Latch = std::latch;

/**
 * @brief A reusable synchronization barrier.
 * 
 * A barrier allows a group of threads to synchronize at a specific point in their execution.
 * Unlike a latch, a barrier is reusable after all threads reach the synchronization point.
 * 
 * @tparam CompletionFunction The function to execute when a phase completes.
 */
template <typename CompletionFunction = void (*)()>
using Barrier = std::barrier<CompletionFunction>;

} // namespace core::concurrency

#endif // MODERN_CPP_CORE_CONCURRENCY_LATCH_BARRIER_HPP
