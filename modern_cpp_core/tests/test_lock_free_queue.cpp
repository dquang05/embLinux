#include "../data_structures/lock_free_queue.hpp"
#include <iostream>
#include <thread>
#include <cassert>
#include <atomic>

using core::data_structures::LockFreeQueue;

/**
 * @brief Tests basic single-threaded push and pop.
 */
void test_single_thread() {
	LockFreeQueue<int, 2> queue;
	assert(queue.push(10));
	assert(queue.push(20));
	assert(!queue.push(30)); // Queue is full (Capacity is 2)

	auto val1 = queue.pop();
	auto val2 = queue.pop();
	auto val3 = queue.pop();

	assert(val1 && *val1 == 10);
	assert(val2 && *val2 == 20);
	assert(!val3); // Queue is empty
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

	auto producer = [&queue, &sum_expected]() {
		for (int i = 1; i <= kNumElements; ++i) {
			while (!queue.push(i)) {
				// Spin wait if full
			}
			sum_expected.fetch_add(i, std::memory_order_relaxed);
		}
	};

	auto consumer = [&queue, &sum_popped]() {
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

	assert(sum_popped.load() == sum_expected.load());
	std::cout << "test_spsc passed.\n";
}

int main() {
	test_single_thread();
	test_spsc();
	std::cout << "All LockFreeQueue tests passed.\n";
	return 0;
}

// RISK REVIEW:
// 1. Concurrency: Testing verifies strictly Single-Producer Single-Consumer (SPSC) mode. 
//    Multi-Producer or Multi-Consumer usage is not supported and will break the queue.
// 2. Performance: Spin waits are used in the test. In production, spinning might consume 100% CPU.
