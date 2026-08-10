#include "../memory/aligned_cache_line.hpp"
#include <iostream>
#include <cassert>
#include <atomic>
#include <thread>
#include <vector>
#include "test_utils.hpp"

using core::memory::AlignedCacheLine;
using core::memory::kCacheLineSize;

/**
 * @brief Tests basic alignment properties.
 */
void test_alignment() {
	// The size of AlignedCacheLine<int> must be at least the cache line size.
	CORE_ASSERT(sizeof(AlignedCacheLine<int>) >= kCacheLineSize);
	
	// The alignment requirement must be exactly kCacheLineSize.
	CORE_ASSERT(alignof(AlignedCacheLine<int>) == kCacheLineSize);

	AlignedCacheLine<int> aligned_val(42);
	CORE_ASSERT(aligned_val == 42);
	
	aligned_val = 100;
	CORE_ASSERT(aligned_val == 100);

	std::cout << "test_alignment passed.\n";
}

/**
 * @brief Simulates concurrent access to check if false sharing is mitigated.
 *        While it's hard to assert performance automatically, we ensure 
 *        the aligned data is fundamentally thread-safe when using atomics.
 */
void test_concurrent_access() {
	struct Data {
		AlignedCacheLine<std::atomic<int>> counter1;
		AlignedCacheLine<std::atomic<int>> counter2;
	};

	Data data;
	data.counter1.m_value.store(0);
	data.counter2.m_value.store(0);

	constexpr int kNumIterations = 1000000;

	auto worker1 = [&data]() {
		for (int i = 0; i < kNumIterations; ++i) {
			data.counter1.m_value.fetch_add(1, std::memory_order_relaxed);
		}
	};

	auto worker2 = [&data]() {
		for (int i = 0; i < kNumIterations; ++i) {
			data.counter2.m_value.fetch_add(1, std::memory_order_relaxed);
		}
	};

	{
		std::jthread t1(worker1);
		std::jthread t2(worker2);
	}

	CORE_ASSERT(data.counter1.m_value.load() == kNumIterations);
	CORE_ASSERT(data.counter2.m_value.load() == kNumIterations);

	// Ensure they are on different cache lines
	uintptr_t addr1 = reinterpret_cast<uintptr_t>(&data.counter1);
	uintptr_t addr2 = reinterpret_cast<uintptr_t>(&data.counter2);
	
	// Diff must be at least the cache line size
	CORE_ASSERT(addr2 - addr1 >= kCacheLineSize);

	std::cout << "test_concurrent_access passed.\n";
}

int main() {
	test_alignment();
	test_concurrent_access();
	std::cout << "All AlignedCacheLine tests passed.\n";
	return 0;
}

// RISK REVIEW:
// 1. Concurrency: Data structure isolates variables to prevent False Sharing.
// 2. Performance: Tests only check correctness, not CPU cache hit rates, which requires profiling.
