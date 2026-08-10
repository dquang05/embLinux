#include "../data_structures/lock_free_queue.hpp"
#include <iostream>
#include <thread>
#include <cassert>
#include <atomic>
#include <latch>
#include <chrono>
#include "test_utils.hpp"

using core::data_structures::LockFreeQueue;

/**
 * @brief Tests basic single-threaded push and pop.
 */
void test_single_thread() {
	LockFreeQueue<int, 2> queue;
	CORE_ASSERT(queue.push(10));
	CORE_ASSERT(queue.push(20));
	CORE_ASSERT(!queue.push(30)); // Queue is full (Capacity is 2)

	auto val1 = queue.pop();
	auto val2 = queue.pop();
	auto val3 = queue.pop();

	CORE_ASSERT(val1 && *val1 == 10);
	CORE_ASSERT(val2 && *val2 == 20);
	CORE_ASSERT(!val3); // Queue is empty
	std::cout << "test_single_thread passed.\n";
}

/**
 * @brief Tests SPSC operations under high throughput.
 */
void test_spsc() {
	LockFreeQueue<int, 1024> queue;
	constexpr int kNumElements = 1000000;
	std::atomic<long long> sum_expected{0};
	std::atomic<long long> sum_popped{0};
	std::latch start_latch{2};

	auto producer = [&queue, &sum_expected, &start_latch]() {
		start_latch.arrive_and_wait();
		for (int i = 1; i <= kNumElements; ++i) {
			while (!queue.push(i)) {
				// Spin wait if full
			}
			sum_expected.fetch_add(i, std::memory_order_relaxed);
		}
	};

	auto consumer = [&queue, &sum_popped, &start_latch]() {
		start_latch.arrive_and_wait();
		for (int i = 0; i < kNumElements; ++i) {
			std::optional<int> val;
			while (!(val = queue.pop())) {
				// Spin wait if empty
			}
			sum_popped.fetch_add(*val, std::memory_order_relaxed);
		}
	};

	{
		std::jthread p(producer);
		std::jthread c(consumer);
	} // auto-joins

	CORE_ASSERT(sum_popped.load() == sum_expected.load());
	std::cout << "test_spsc passed.\n";
}

void test_boundary_conditions() {
	LockFreeQueue<int, 16> queue;
	constexpr int kNumElements = 100000;
	
	// Test Producer much faster than Consumer (Queue Full constantly)
	std::atomic<int> pushed{0};
	std::atomic<int> popped{0};
	
	std::jthread p1([&queue, &pushed]() {
		for (int i = 0; i < kNumElements; ++i) {
			while (!queue.push(i)) { /* spin */ }
			pushed++;
		}
	});
	
	std::jthread c1([&queue, &popped]() {
		for (int i = 0; i < kNumElements; ++i) {
			std::this_thread::sleep_for(std::chrono::microseconds(1)); // Slow consumer
			while (!queue.pop()) { /* spin */ }
			popped++;
		}
	});
	
	p1.join();
	c1.join();
	CORE_ASSERT(pushed.load() == kNumElements);
	CORE_ASSERT(popped.load() == kNumElements);
	
	// Test Consumer much faster than Producer (Queue Empty constantly)
	pushed = 0;
	popped = 0;
	
	std::jthread p2([&queue, &pushed]() {
		for (int i = 0; i < kNumElements; ++i) {
			std::this_thread::sleep_for(std::chrono::microseconds(1)); // Slow producer
			while (!queue.push(i)) { /* spin */ }
			pushed++;
		}
	});
	
	std::jthread c2([&queue, &popped]() {
		for (int i = 0; i < kNumElements; ++i) {
			while (!queue.pop()) { /* spin */ }
			popped++;
		}
	});
	
	p2.join();
	c2.join();
	CORE_ASSERT(pushed.load() == kNumElements);
	CORE_ASSERT(popped.load() == kNumElements);

	std::cout << "test_boundary_conditions passed.\n";
}

int main() {
	test_single_thread();
	test_spsc();
	test_boundary_conditions();
	std::cout << "All LockFreeQueue tests passed.\n";
	return 0;
}

// RISK REVIEW:
// 1. Concurrency: Testing verifies strictly Single-Producer Single-Consumer (SPSC) mode. 
//    Multi-Producer or Multi-Consumer usage is not supported and will break the queue.
// 2. Performance: Spin waits are used in the test. In production, spinning might consume 100% CPU.
