#include "../data_structures/lock_free_stack.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <cassert>

using core::data_structures::LockFreeStack;

/**
 * @brief Tests the LockFreeStack in a single-threaded environment.
 * 
 * @note Ensures basic push and pop functionality works as expected.
 */
void test_single_thread() {
	LockFreeStack<int> stack;
	stack.push(10);
	stack.push(20);

	auto val1 = stack.pop();
	auto val2 = stack.pop();
	auto val3 = stack.pop();

	assert(val1 && *val1 == 20);
	assert(val2 && *val2 == 10);
	assert(!val3);
	std::cout << "test_single_thread passed.\n";
}

/**
 * @brief Tests the LockFreeStack in a multi-threaded environment.
 * 
 * @note Spawns multiple threads to concurrently push elements, then verifies 
 *       if all elements were successfully pushed and can be popped.
 */
void test_multi_thread() {
	LockFreeStack<int> stack;
	constexpr int kNumThreads = 10;
	constexpr int kNumElements = 1000;

	auto worker_push = [&stack]() {
		for (int i = 0; i < kNumElements; ++i) {
			stack.push(i);
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

	assert(count == kNumThreads * kNumElements);
	std::cout << "test_multi_thread passed.\n";
}

/**
 * @brief Simulates an extreme contention scenario (e.g., High-Frequency Memory Pool/Free List).
 * 
 * @note Instead of dynamically allocating memory (new/delete), a lock-free stack is used 
 *       to store reusable objects. Multiple threads continuously pop an object, use it, 
 *       and push it back. This causes extreme contention which would bottleneck a Mutex.
 */
void test_extreme_contention_freelist() {
	LockFreeStack<int> free_list;
	constexpr int kInitialPoolSize = 1000;
	constexpr int kNumThreads = 10;
	constexpr int kOperationsPerThread = 10000;

	// Initialize the pool with 1000 reusable objects
	for (int i = 0; i < kInitialPoolSize; ++i) {
		free_list.push(0);
	}

	std::atomic<int> successful_operations{0};

	auto worker = [&free_list, &successful_operations]() {
		for (int i = 0; i < kOperationsPerThread; ++i) {
			// Simulate: Thread tries to acquire an object from the Free List
			if (auto obj = free_list.pop()) {
				// Simulate fast processing
				*obj = i; 
				
				// Return the object immediately to the Free List
				free_list.push(*obj);
				successful_operations.fetch_add(1, std::memory_order_relaxed);
			}
		}
	};

	std::vector<std::jthread> threads;
	for (int i = 0; i < kNumThreads; ++i) {
		threads.emplace_back(worker);
	}
	
	threads.clear(); // Waits for all jthreads to finish

	assert(successful_operations.load() == kNumThreads * kOperationsPerThread);
	std::cout << "test_extreme_contention_freelist passed.\n";
}

/**
 * @brief Stress tests the stack specifically to provoke and verify prevention of the ABA problem.
 * 
 * @note Without Hazard Pointers, popping and pushing the same elements rapidly 
 *       across multiple threads will likely trigger an ABA bug or segmentation fault.
 */
void test_aba_stress() {
	LockFreeStack<int> stack;
	constexpr int kNumThreads = 4;
	constexpr int kOperations = 10000;
	
	for (int i = 0; i < 100; ++i) {
		stack.push(i);
	}

	auto worker = [&stack]() {
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
	std::cout << "test_aba_stress passed.\n";
}

int main() {
	test_single_thread();
	test_multi_thread();
	test_extreme_contention_freelist();
	test_aba_stress();
	std::cout << "All LockFreeStack tests passed.\n";
	return 0;
}

// RISK REVIEW:
// 1. Concurrency: High contention may cause extreme CPU spinning during `compare_exchange_weak`.
// 2. Caller responsibilities: Caller must ensure that testing constants (kNumThreads, kOperationsPerThread) 
//    do not exceed system limits or cause out-of-memory errors.
// 3. Unspecified edge cases: What happens if `pop()` fails due to an empty pool in the freelist test? 
//    Currently, the test safely ignores it, but real-world usage must handle pool exhaustion.
