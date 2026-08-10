#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <cassert>
#include <cmath>
#include <limits>
#include "test_utils.hpp"

// In C++11/14/17, std::atomic<float> existed but did NOT support fetch_add or fetch_sub!
// Developers had to write slow Compare-And-Swap (CAS) loops to safely update
// a floating-point number from multiple threads.
//
// In C++20, std::atomic<float> and std::atomic<double> natively support fetch_add/sub.
// This is critical for Embedded Control Systems (like PID controllers or Sensor Fusion),
// allowing lock-free accumulation of floating-point sensor data.

void test_atomic_float_accumulation() {
	std::atomic<float> sensor_accumulator{0.0f};

	// 10 concurrent threads (e.g., simulating fast ISRs or parallel compute nodes)
	constexpr int kNumThreads = 10;
	constexpr int kIterations = 10000;
	constexpr float kIncrement = 1.5f;

	auto worker = [&sensor_accumulator]() {
		for (int i = 0; i < kIterations; ++i) {
			// Lock-free float addition! (C++20 feature)
			sensor_accumulator.fetch_add(kIncrement, std::memory_order_relaxed);
		}
	};

	std::vector<std::jthread> threads;
	for (int i = 0; i < kNumThreads; ++i) {
		threads.emplace_back(worker);
	}

	// Auto join via jthread
	threads.clear();

	float expected = kNumThreads * kIterations * kIncrement;
	float actual = sensor_accumulator.load();

	// Float comparison with dynamic epsilon to account for iteration accumulation
	float epsilon = std::numeric_limits<float>::epsilon() * expected * 2.0f;
	CORE_ASSERT(std::abs(actual - expected) <= epsilon);

	std::cout << "test_atomic_float_accumulation passed (Expected: " 
			  << expected << ", Actual: " << actual << ").\n";
}

int main() {
	test_atomic_float_accumulation();
	std::cout << "All atomic float tests passed.\n";
	return 0;
}
