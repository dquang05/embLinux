#include "../data_structures/hazard_pointer.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <cassert>
#include <latch>
#include <atomic>
#include "test_utils.hpp"

using core::data_structures::get_hazard_pointer_for_current_thread;
using core::data_structures::kMaxHazardPointers;
using core::data_structures::HazardPointerError;

void test_basic_acquire_release() {
    auto hp = get_hazard_pointer_for_current_thread();
    CORE_ASSERT(hp.has_value());
    CORE_PASS("test_basic_acquire_release");
}

void test_exhaustion() {
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};
    
    // We spawn more threads than the maximum hazard pointers available
    constexpr size_t kExtraThreads = 5;
    constexpr size_t num_threads = kMaxHazardPointers + kExtraThreads;
    
    std::latch start_latch{num_threads};
    std::latch exit_latch{num_threads};

    auto holder_thread = [&]() {
        start_latch.arrive_and_wait();
        auto hp = get_hazard_pointer_for_current_thread();
        if (hp.has_value()) {
            success_count.fetch_add(1, std::memory_order_relaxed);
        } else {
            failure_count.fetch_add(1, std::memory_order_relaxed);
            CORE_ASSERT(hp.error() == HazardPointerError::NoAvailableHazardPointers);
        }
        exit_latch.arrive_and_wait();
    };

    std::vector<std::jthread> threads;
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(holder_thread);
    }
    
    threads.clear(); // Auto join all threads

    // Since test_basic_acquire_release() already ran on the main thread, 
    // the main thread holds 1 hazard pointer via thread_local.
    // Thus, only kMaxHazardPointers - 1 are available for the new threads.
    CORE_ASSERT(success_count.load() == kMaxHazardPointers - 1);
    CORE_ASSERT(failure_count.load() == kExtraThreads + 1);

    CORE_PASS("test_exhaustion");
}

int main() {
    test_basic_acquire_release();
    test_exhaustion();
    return 0;
}
