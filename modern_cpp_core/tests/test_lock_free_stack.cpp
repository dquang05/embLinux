#include "../data_structures/lock_free_stack.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <cassert>
#include <latch>
#include "test_utils.hpp"

using core::data_structures::LockFreeStack;

/**
 * @brief Tests the LockFreeStack in a single-threaded environment.
 * 
 * @note Ensures basic push and pop functionality works as expected.
 */
void test_single_thread() {
	LockFreeStack<int, 10> stack;
	bool p1 = stack.push(10);
	bool p2 = stack.push(20);
	
	CORE_ASSERT(p1 && p2);

	auto val1 = stack.pop();
	auto val2 = stack.pop();
	auto val3 = stack.pop();

	CORE_ASSERT(val1 && *val1 == 20);
	CORE_ASSERT(val2 && *val2 == 10);
	CORE_ASSERT(!val3);
	CORE_PASS("test_single_thread");
}

/**
 * @brief Tests the LockFreeStack in a multi-threaded environment.
 * 
 * @note Spawns multiple threads to concurrently push elements, then verifies 
 *       if all elements were successfully pushed and can be popped.
 */
void test_multi_thread() {
	constexpr int kNumThreads = 10;
	constexpr int kNumElements = 1000;
	LockFreeStack<int, kNumThreads * kNumElements> stack;

	std::latch start_latch{kNumThreads};

	auto worker_push = [&stack, &start_latch]() {
		start_latch.arrive_and_wait();
		for (int i = 0; i < kNumElements; ++i) {
			bool success = stack.push(i);
			CORE_ASSERT(success);
		}
	};

	std::vector<std::jthread> threads;
	for (int i = 0; i < kNumThreads; ++i) {
		threads.emplace_back(worker_push);
	}

	// jthread auto-joins on destruction
	threads.clear();

	int count = 0;
	while (auto val = stack.pop()) {
		++count;
	}

	CORE_ASSERT(count == kNumThreads * kNumElements);
	CORE_PASS("test_multi_thread");
}

/**
 * @brief Simulates an extreme contention scenario (e.g., High-Frequency Memory Pool/Free List).
 * 
 * @note Instead of dynamically allocating memory (new/delete), a lock-free stack is used 
 *       to store reusable objects. Multiple threads continuously pop an object, use it, 
 *       and push it back. This causes extreme contention which would bottleneck a Mutex.
 */
void test_extreme_contention_freelist() {
	constexpr int kInitialPoolSize = 1000;
	constexpr int kNumThreads = 10;
	constexpr int kOperationsPerThread = 10000;
	
	LockFreeStack<int, kInitialPoolSize + kNumThreads> free_list;

	// Initialize the pool with 1000 reusable objects
	for (int i = 0; i < kInitialPoolSize; ++i) {
		bool success = free_list.push(0);
		CORE_ASSERT(success);
	}

	std::atomic<int> successful_operations{0};
	std::latch start_latch{kNumThreads};

	auto worker = [&free_list, &successful_operations, &start_latch]() {
		start_latch.arrive_and_wait();
		for (int i = 0; i < kOperationsPerThread; ++i) {
			// Simulate: Thread tries to acquire an object from the Free List
			if (auto obj = free_list.pop()) {
				// Simulate fast processing
				*obj = i; 
				
				// Return the object immediately to the Free List
				bool success = free_list.push(*obj);
				CORE_ASSERT(success);
				successful_operations.fetch_add(1, std::memory_order_relaxed);
			}
		}
	};

	std::vector<std::jthread> threads;
	for (int i = 0; i < kNumThreads; ++i) {
		threads.emplace_back(worker);
	}
	
	threads.clear(); // Waits for all jthreads to finish

	CORE_ASSERT(successful_operations.load() == kNumThreads * kOperationsPerThread);
	CORE_PASS("test_extreme_contention_freelist");
}

/**
 * @brief Stress tests the stack specifically to provoke and verify prevention of the ABA problem.
 * 
 * @note Without Hazard Pointers, popping and pushing the same elements rapidly 
 *       across multiple threads will likely trigger an ABA bug or segmentation fault.
 */
void test_aba_stress() {
	constexpr int kNumThreads = 4;
	constexpr int kOperations = 100000; // Increased operations for stress testing
	
	LockFreeStack<int, 200> stack;
	
	for (int i = 0; i < 100; ++i) {
		stack.push(i);
	}

	std::latch start_latch{kNumThreads};

	auto worker = [&stack, &start_latch]() {
		start_latch.arrive_and_wait();
		for (int i = 0; i < kOperations; ++i) {
			auto val = stack.pop();
			if (val) {
				stack.push(*val);
			}
		}
	};

	std::vector<std::jthread> threads;
	for (int i = 0; i < kNumThreads; ++i) {
		threads.emplace_back(worker);
	}
	
	threads.clear();
	CORE_PASS("test_aba_stress");
}

int main() {
	test_single_thread();
	test_multi_thread();
	test_extreme_contention_freelist();
	test_aba_stress();
	return 0;
}

// RISK REVIEW:
// 1. Concurrency: High contention may cause extreme CPU spinning during `compare_exchange_weak`.
// 2. Caller responsibilities: Caller must ensure that testing constants (kNumThreads, kOperationsPerThread) 
//    do not exceed system limits or cause out-of-memory errors.
// 3. Unspecified edge cases: What happens if `pop()` fails due to an empty pool in the freelist test? 
//    Currently, the test safely ignores it, but real-world usage must handle pool exhaustion.
