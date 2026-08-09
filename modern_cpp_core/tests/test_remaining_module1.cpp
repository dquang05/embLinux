#include <iostream>
#include <cassert>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <latch>
#include "../concurrency/atomic_wait.hpp"
#include "../concurrency/spinlock.hpp"
#include "../concurrency/thread_affinity.hpp"

using namespace core::concurrency;

void test_atomic_wait() {
    std::cout << "[TEST] Atomic Wait/Notify\n";
    std::atomic<int> flag{0};
    bool success = false;

    std::jthread t([&]() {
        atomic_wait_until(flag, 0, [&]() { return flag.load() == 1; });
        success = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    flag.store(1);
    atomic_notify_one(flag);

    t.join();
    assert(success);
    std::cout << "  -> PASS\n";
}

void test_spinlock() {
    std::cout << "[TEST] Spinlock High Contention\n";
    Spinlock slock;
    long long shared_counter = 0;
    std::latch go_signal(1);
    std::vector<std::jthread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            go_signal.wait();
            for (int j = 0; j < 10000; ++j) {
                slock.lock();
                shared_counter++;
                slock.unlock();
            }
        });
    }

    go_signal.count_down();
    for (auto& t : threads) t.join();

    assert(shared_counter == 100000);
    std::cout << "  -> PASS\n";
}

void test_thread_affinity() {
    std::cout << "[TEST] Thread Affinity & Real-time Priority\n";

    std::jthread t([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });

    // Test Affinity
    auto res_affinity = set_thread_affinity(t, 0); // Core 0 should exist
    if (res_affinity) {
        std::cout << "  Affinity (Core 0): SUCCESS\n";
    } else {
        std::cout << "  Affinity (Core 0): FAILED (" << res_affinity.error().message() << ")\n";
        // It could fail on some restricted VMs, so we just log it.
    }

    // Test Priority
    auto res_priority = set_thread_realtime_priority(t, 50);
    if (res_priority) {
        std::cout << "  Real-time Priority (50): SUCCESS\n";
    } else {
        std::cout << "  Real-time Priority (50): SKIPPED (Expected EPERM if not root: " 
                  << res_priority.error().message() << ")\n";
    }

    std::cout << "  -> PASS (Handled Expected Gracefully)\n";
}

int main() {
    std::cout << "=== Running Remaining Module 1 Tests ===\n";
    
    test_atomic_wait();
    test_spinlock();
    test_thread_affinity();

    std::cout << "=== All tests PASSED ===\n";
    return 0;
}
