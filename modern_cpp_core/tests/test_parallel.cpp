#include "../patterns/parallel_algorithms.hpp"
#include "../patterns/telemetry.hpp"
#include "../concurrency/thread_pool.hpp"
#include "test_utils.hpp"
#include <iostream>
#include <vector>
#include <numeric>

using namespace core::patterns;
using namespace core::concurrency;

void test_parallel_for_each() {
    auto pool_res = JThreadPool::create(4);
    CORE_ASSERT(pool_res.has_value());
    auto pool = std::move(pool_res.value());

    std::vector<int> data(10000, 1);
    
    parallel::parallel_for_each(*pool, data.begin(), data.end(), [](int& x) {
        x *= 2;
    }, 1000); // chunk size 1000
    
    // Verify
    for (int x : data) {
        CORE_ASSERT(x == 2);
    }
    std::cout << "test_parallel_for_each passed.\n";
}

void test_parallel_transform() {
    auto pool_res = JThreadPool::create(4);
    CORE_ASSERT(pool_res.has_value());
    auto pool = std::move(pool_res.value());

    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 1);
    std::vector<int> out(10000);
    
    parallel::parallel_transform(*pool, data.begin(), data.end(), out.begin(), [](int x) {
        return x * x;
    }, 1000); // chunk size 1000
    
    // Verify
    CORE_ASSERT(out[0] == 1);
    CORE_ASSERT(out[4] == 25);
    CORE_ASSERT(out[9999] == 10000 * 10000);
    
    std::cout << "test_parallel_transform passed.\n";
}

void test_relaxed_counter() {
    RelaxedCounter<uint64_t> telemetry_dropped_frames(0);
    
    auto pool_res = JThreadPool::create(8);
    CORE_ASSERT(pool_res.has_value());
    auto pool = std::move(pool_res.value());
    
    std::vector<int> tasks(10000, 0);
    
    parallel::parallel_for_each(*pool, tasks.begin(), tasks.end(), [&telemetry_dropped_frames](int&) {
        telemetry_dropped_frames.increment();
    }, 100);
    
    CORE_ASSERT(telemetry_dropped_frames.get() == 10000);
    std::cout << "test_relaxed_counter passed.\n";
}

int main() {
    test_parallel_for_each();
    test_parallel_transform();
    test_relaxed_counter();
    std::cout << "All parallel and telemetry tests passed.\n";
    return 0;
}
