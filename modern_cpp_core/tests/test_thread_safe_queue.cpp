#include "../data_structures/thread_safe_queue.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <cassert>
#include <atomic>

using core::data_structures::ThreadSafeQueue;

/**
 * @brief Tests basic push and non-blocking pop (try_pop) in a single thread.
 */
void test_try_pop() {
	ThreadSafeQueue<int> queue;
	queue.push(10);
	queue.push(20);

	auto val1 = queue.try_pop();
	auto val2 = queue.try_pop();
	auto val3 = queue.try_pop();

	assert(val1 && *val1 == 10);
	assert(val2 && *val2 == 20);
	assert(!val3);
	std::cout << "test_try_pop passed.\n";
}

/**
 * @brief Tests blocking pop (wait_and_pop) with producer and consumer threads.
 */
void test_wait_and_pop() {
	ThreadSafeQueue<int> queue;
	constexpr int kNumElements = 1000;
	std::atomic<int> sum_popped{0};
	std::atomic<int> sum_expected{0};

	auto producer = [&queue, &sum_expected]() {
		for (int i = 1; i <= kNumElements; ++i) {
			queue.push(i);
			sum_expected.fetch_add(i, std::memory_order_relaxed);
		}
	};

	auto consumer = [&queue, &sum_popped]() {
		for (int i = 0; i < kNumElements; ++i) {
			int val = queue.wait_and_pop();
			sum_popped.fetch_add(val, std::memory_order_relaxed);
		}
	};

	{
		std::jthread t1(producer);
		std::jthread t2(consumer);
	} // jthreads auto-join here

	assert(sum_popped.load() == sum_expected.load());
	std::cout << "test_wait_and_pop passed.\n";
}

int main() {
	test_try_pop();
	test_wait_and_pop();
	std::cout << "All ThreadSafeQueue tests passed.\n";
	return 0;
}

// RISK REVIEW:
// 1. Concurrency: Testing wait_and_pop() relies on threads waking up correctly via condition_variable.
// 2. Deadlocks: If producer pushes fewer items than consumer expects, the consumer will wait forever.
// 3. Thread Management: Uses std::jthread for safe, automatic joining on scope exit.
