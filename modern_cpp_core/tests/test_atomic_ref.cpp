#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <cassert>
#include "test_utils.hpp"

// Simulate a packed data structure that we might receive over network or DMA.
// We use alignas to guarantee it meets std::atomic_ref's alignment requirements
// while remaining a trivial standard-layout struct.
struct alignas(std::atomic_ref<int>::required_alignment) SensorData {
	int temperature;
	int humidity;
	int pressure;
};

void test_atomic_ref_concurrent_update() {
	// A massive array of non-atomic structs. No padding overhead from std::atomic.
	std::vector<SensorData> data(100, {0, 0, 0});

	constexpr int kNumThreads = 10;
	constexpr int kIncrementsPerThread = 10000;

	auto worker = [&data](int index) {
		// Static assert to ensure alignment at compile time
		static_assert(alignof(SensorData) == std::atomic_ref<int>::required_alignment, 
					  "SensorData must be aligned to std::atomic_ref requirements");

		// Runtime assert to ensure the specific memory location is aligned correctly
		CORE_ASSERT(reinterpret_cast<uintptr_t>(&data[index].temperature) % std::atomic_ref<int>::required_alignment == 0);

		for (int i = 0; i < kIncrementsPerThread; ++i) {
			// Temporarily treat the non-atomic temperature field as atomic
			std::atomic_ref<int> temp_ref(data[index].temperature);
			temp_ref.fetch_add(1, std::memory_order_relaxed);
		}
	};

	std::vector<std::jthread> threads;
	// Have 10 threads all concurrently incrementing the temperature of data[50]
	for (int i = 0; i < kNumThreads; ++i) {
		threads.emplace_back(worker, 50);
	}

	// Wait for all threads (jthread auto-joins)
	threads.clear();

	// Verify that all increments were caught without data races
	CORE_ASSERT(data[50].temperature == kNumThreads * kIncrementsPerThread);
	
	std::cout << "test_atomic_ref_concurrent_update passed.\n";
}

int main() {
	test_atomic_ref_concurrent_update();
	std::cout << "All std::atomic_ref tests passed.\n";
	return 0;
}
