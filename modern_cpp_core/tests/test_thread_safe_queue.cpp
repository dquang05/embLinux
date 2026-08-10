#include "../data_structures/thread_safe_queue.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <vector>
#include <cassert>
#include <atomic>
#include <latch>
#include <chrono>
#include "test_utils.hpp"

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

	CORE_ASSERT(val1 && *val1 == 10);
	CORE_ASSERT(val2 && *val2 == 20);
	CORE_ASSERT(!val3);
	CORE_PASS("test_try_pop");
}

/**
 * @brief Tests blocking pop (wait_and_pop) with producer and consumer threads.
 */
void test_wait_and_pop() {
	ThreadSafeQueue<int> queue;
	constexpr int kNumProducers = 4;
	constexpr int kNumConsumers = 4;
	constexpr int kNumElementsPerProducer = 10000;
	std::atomic<long long> sum_popped{0};
	std::atomic<long long> sum_expected{0};
	std::latch start_latch{kNumProducers + kNumConsumers};

	auto producer = [&queue, &sum_expected, &start_latch]() {
		start_latch.arrive_and_wait();
		for (int i = 1; i <= kNumElementsPerProducer; ++i) {
			queue.push(i);
			sum_expected.fetch_add(i, std::memory_order_relaxed);
		}
	};

	auto consumer = [&queue, &sum_popped, &start_latch]() {
		start_latch.arrive_and_wait();
		for (int i = 0; i < kNumElementsPerProducer; ++i) {
			int val = queue.wait_and_pop();
			sum_popped.fetch_add(val, std::memory_order_relaxed);
		}
	};

	{
		std::vector<std::jthread> producers;
		std::vector<std::jthread> consumers;
		for (int i = 0; i < kNumProducers; ++i) {
			producers.emplace_back(producer);
		}
		for (int i = 0; i < kNumConsumers; ++i) {
			consumers.emplace_back(consumer);
		}
	} // jthreads auto-join here

	CORE_ASSERT(sum_popped.load() == sum_expected.load());
	CORE_PASS("test_wait_and_pop");
}

/**
 * @brief Tests edge case where consumer waits but no producer pushes.
 * Uses std::chrono to ensure we don't deadlock the test suite forever if we test this.
 * We just document this behavior. wait_and_pop does not have a timeout.
 */
void test_deadlock_edge_case() {
	ThreadSafeQueue<int> queue;
	std::atomic<bool> consumer_finished{false};

	// Start a consumer thread that will block on wait_and_pop
	std::jthread consumer([&queue, &consumer_finished]() {
		int val = queue.wait_and_pop();
		CORE_ASSERT(val == 999);
		consumer_finished.store(true, std::memory_order_relaxed);
	});

	// Wait a bit to ensure consumer is blocked
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	CORE_ASSERT(!consumer_finished.load(std::memory_order_relaxed));

	// Unblock it by pushing an element (since we don't have stop_token integration in wait_and_pop yet)
	queue.push(999);

	// The jthread will auto join on scope exit, which ensures the unblock was successful.
	// If it doesn't unblock, the test will hang here (which would fail CI).
	
	// We can manually wait and assert for clarity
	consumer.join();
	CORE_ASSERT(consumer_finished.load(std::memory_order_relaxed));

	CORE_PASS("test_deadlock_edge_case");
}

int main() {
	test_try_pop();
	test_wait_and_pop();
	test_deadlock_edge_case();
	return 0;
}

// RISK REVIEW:
// 1. Concurrency: Testing wait_and_pop() relies on threads waking up correctly via condition_variable.
// 2. Deadlocks: If producer pushes fewer items than consumer expects, the consumer will wait forever.
// 3. Thread Management: Uses std::jthread for safe, automatic joining on scope exit.
