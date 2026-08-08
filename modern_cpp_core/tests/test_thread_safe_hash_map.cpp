#include "../data_structures/thread_safe_hash_map.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <cassert>
#include <string>

using core::data_structures::ThreadSafeHashMap;

/**
 * @brief Tests basic insert, get, and remove operations.
 */
void test_basic_operations() {
	ThreadSafeHashMap<std::string, int> map;
	map.insert("apple", 10);
	map.insert("banana", 20);
	
	assert(map.get("apple") == 10);
	assert(map.get("banana") == 20);
	
	map.insert("apple", 15); // Update existing
	assert(map.get("apple") == 15);
	
	map.remove("banana");
	assert(!map.get("banana"));
	std::cout << "test_basic_operations passed.\n";
}

/**
 * @brief Tests concurrent inserts and reads across multiple threads.
 */
void test_concurrent_access() {
	ThreadSafeHashMap<int, int> map;
	constexpr int kNumThreads = 10;
	constexpr int kOperations = 1000;

	auto worker = [&map](int thread_id) {
		for (int i = 0; i < kOperations; ++i) {
			int key = thread_id * kOperations + i;
			map.insert(key, key * 2);
			
			auto val = map.get(key);
			assert(val && *val == key * 2);
		}
	};

	std::vector<std::jthread> threads;
	for (int i = 0; i < kNumThreads; ++i) {
		threads.emplace_back(worker, i);
	}
	
	threads.clear(); // Joins all jthreads

	for (int i = 0; i < kNumThreads; ++i) {
		for (int j = 0; j < kOperations; ++j) {
			int key = i * kOperations + j;
			assert(map.get(key) == key * 2);
		}
	}
	std::cout << "test_concurrent_access passed.\n";
}

int main() {
	test_basic_operations();
	test_concurrent_access();
	std::cout << "All ThreadSafeHashMap tests passed.\n";
	return 0;
}

// RISK REVIEW:
// 1. Concurrency: Validates that multiple threads can insert without data corruption, leveraging fine-grained locks.
// 2. Caller Responsibilities: Keys must provide a proper std::hash specialization.
