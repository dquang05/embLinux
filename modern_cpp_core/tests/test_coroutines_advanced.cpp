#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include "../coroutines/coroutine_pool.hpp"
#include "../coroutines/event_loop.hpp"
#include "../coroutines/async_timer.hpp"
#include "../coroutines/channel.hpp"
#include "../coroutines/task.hpp"

using namespace core::coroutines;
using namespace std::chrono_literals;

struct DetachedTask {
    struct promise_type {
        static void* operator new(std::size_t size) { return CoroutinePoolAllocator::allocate(size); }
        static void operator delete(void* ptr, std::size_t size) { CoroutinePoolAllocator::deallocate(ptr, size); }
        DetachedTask get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
};

void test_pool_exhaustion() {
    std::cout << "[TEST] Coroutine Pool Exhaustion\n";
    std::vector<Task<void>> tasks;
    bool caught = false;
    try {
        for (int i = 0; i < 70; ++i) { // Max is 64
            auto lambda = []() -> Task<void> { co_return; };
            tasks.push_back(lambda());
        }
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    assert(caught);
    std::cout << "  -> PASS\n";
}

std::atomic<bool> timer_done{false};
std::atomic<int> timer_elapsed{0};

DetachedTask run_async_timer() {
    auto start = std::chrono::steady_clock::now();
    co_await AsyncTimer(50ms);
    auto end = std::chrono::steady_clock::now();
    timer_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    timer_done = true;
}

void test_async_timer() {
    std::cout << "[TEST] Async Timer via EventLoop\n";
    
    std::jthread loop_thread([]() {
        EventLoop::get().run();
    });

    run_async_timer(); // kicks off immediately

    while (!timer_done.load()) {
        std::this_thread::sleep_for(5ms);
    }
    EventLoop::get().stop();

    assert(timer_elapsed >= 45); // Allow some OS scheduling variance
    std::cout << "  -> PASS (Elapsed: " << timer_elapsed.load() << "ms)\n";
}

Channel<int, 5> chan;
std::atomic<int> consumer_sum{0};
std::atomic<bool> channel_done{false};

DetachedTask producer_task() {
    for (int i = 1; i <= 100; ++i) {
        co_await chan.send(i);
    }
}

DetachedTask consumer_task() {
    int sum = 0;
    for (int i = 1; i <= 100; ++i) {
        sum += co_await chan.receive();
    }
    consumer_sum = sum;
    channel_done = true;
}

void test_channel() {
    std::cout << "[TEST] Bounded CSP Channel (Capacity 5)\n";
    
    producer_task();
    consumer_task();

    while (!channel_done.load()) {
        std::this_thread::yield();
    }

    // Sum of 1 to 100 = 5050
    assert(consumer_sum.load() == 5050);
    std::cout << "  -> PASS\n";
}

int main() {
    std::cout << "=== Running Advanced Coroutine Tests ===\n";
    
    // Boot-time initialization of memory pool
    // 64 coroutines maximum, 256 bytes per frame
    CoroutinePoolAllocator::init(64, 256);

    test_pool_exhaustion();
    test_async_timer();
    test_channel();

    CoroutinePoolAllocator::destroy();
    
    std::cout << "=== All tests PASSED ===\n";
    return 0;
}
