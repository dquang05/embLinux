#pragma once

#include "../concurrency/thread_pool.hpp"
#include <iterator>
#include <latch>
#include <algorithm>
#include <vector>

namespace core::patterns::parallel {

/**
 * @brief Executes a unary function on a range in parallel using JThreadPool.
 * 
 * Partitions the range into chunks and distributes them to the thread pool.
 * The calling thread blocks until all chunks are processed.
 * 
 * @warning Do not call this recursively from within a thread pool worker to avoid deadlock.
 */
template <typename RandomIt, typename UnaryFunction>
void parallel_for_each(core::concurrency::JThreadPool& pool, 
                       RandomIt first, RandomIt last, 
                       UnaryFunction f, 
                       std::size_t chunk_size = 1000) {
    auto distance = std::distance(first, last);
    if (distance <= 0) return;
    
    std::size_t num_chunks = (distance + chunk_size - 1) / chunk_size;
    std::latch done_latch(num_chunks);
    
    for (std::size_t i = 0; i < num_chunks; ++i) {
        auto chunk_start = first + i * chunk_size;
        auto chunk_end = (i == num_chunks - 1) ? last : chunk_start + chunk_size;
        
        // Execute chunk in the thread pool
        pool.execute([chunk_start, chunk_end, f, &done_latch]() {
            std::for_each(chunk_start, chunk_end, f);
            done_latch.count_down();
        });
    }
    
    // Wait for all chunks to finish
    done_latch.wait();
}

/**
 * @brief Executes a transformation on a range in parallel using JThreadPool.
 */
template <typename RandomIt, typename OutputIt, typename UnaryOperation>
void parallel_transform(core::concurrency::JThreadPool& pool,
                        RandomIt first, RandomIt last, OutputIt d_first,
                        UnaryOperation op,
                        std::size_t chunk_size = 1000) {
    auto distance = std::distance(first, last);
    if (distance <= 0) return;
    
    std::size_t num_chunks = (distance + chunk_size - 1) / chunk_size;
    std::latch done_latch(num_chunks);
    
    for (std::size_t i = 0; i < num_chunks; ++i) {
        auto chunk_start = first + i * chunk_size;
        auto chunk_end = (i == num_chunks - 1) ? last : chunk_start + chunk_size;
        auto out_start = d_first + i * chunk_size;
        
        pool.execute([chunk_start, chunk_end, out_start, op, &done_latch]() {
            std::transform(chunk_start, chunk_end, out_start, op);
            done_latch.count_down();
        });
    }
    
    done_latch.wait();
}

} // namespace core::patterns::parallel
