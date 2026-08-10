#include "../data_structures/lock_free_queue.hpp"
#include <iostream>
#include <thread>
#include <cassert>
#include <atomic>
#include <latch>
#include <chrono>
#include <cstdint>
#include <new>
#include <cstdlib>
#include "test_utils.hpp"

using core::data_structures::LockFreeQueue;

// --- Heap Allocation Tracker ---
std::atomic<bool> g_disable_allocations{false};

void* operator new(std::size_t size) {
	if (g_disable_allocations.load(std::memory_order_relaxed)) {
		std::cerr << "[FAIL] Unexpected heap allocation of size " << size << "!\n";
		std::exit(1);
	}
	void* p = std::malloc(size);
	if (!p) throw std::bad_alloc();
	return p;
}

void operator delete(void* p) noexcept {
	std::free(p);
}

void operator delete(void* p, std::size_t) noexcept {
	std::free(p);
}
// -------------------------------

void test_single_thread() {
	LockFreeQueue<uint32_t, 2> queue;
	CORE_ASSERT(queue.push(10));
	CORE_ASSERT(queue.push(20));
	CORE_ASSERT(!queue.push(30)); // Queue is full

	auto val1 = queue.pop();
	auto val2 = queue.pop();
	auto val3 = queue.pop();

	CORE_ASSERT(val1 && *val1 == 10);
	CORE_ASSERT(val2 && *val2 == 20);
	CORE_ASSERT(!val3); // Queue is empty
	CORE_PASS("test_single_thread");
}

void test_spsc_no_allocation_and_determinism() {
	LockFreeQueue<uint32_t, 1024> queue;
	constexpr uint32_t kNumElements = 1000000;
	std::atomic<uint64_t> sum_expected{0};
	std::atomic<uint64_t> sum_popped{0};
	std::latch start_latch{2};

	auto producer = [&queue, &sum_expected, &start_latch]() {
		start_latch.arrive_and_wait();
		g_disable_allocations.store(true, std::memory_order_relaxed);
		
		auto start = std::chrono::steady_clock::now();
		for (uint32_t i = 1; i <= kNumElements; ++i) {
			while (!queue.push(i)) {}
			sum_expected.fetch_add(i, std::memory_order_relaxed);
		}
		auto end = std::chrono::steady_clock::now();
		g_disable_allocations.store(false, std::memory_order_relaxed);
		
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		CORE_ASSERT(duration < 5000); // WCET check
	};

	auto consumer = [&queue, &sum_popped, &start_latch]() {
		start_latch.arrive_and_wait();
		for (uint32_t i = 0; i < kNumElements; ++i) {
			std::optional<uint32_t> val;
			while (!(val = queue.pop())) {}
			sum_popped.fetch_add(*val, std::memory_order_relaxed);
		}
	};

	{
		std::jthread p(producer);
		std::jthread c(consumer);
	} 

	CORE_ASSERT(sum_popped.load() == sum_expected.load());
	CORE_PASS("test_spsc_no_allocation_and_determinism");
}

void test_integer_overflow() {
	LockFreeQueue<uint8_t, 16> queue;
	
	// Push MAX values to ensure boundary works
	CORE_ASSERT(queue.push(UINT8_MAX));
	CORE_ASSERT(queue.push(0));
	
	auto val1 = queue.pop();
	auto val2 = queue.pop();
	
	CORE_ASSERT(val1 && *val1 == UINT8_MAX);
	CORE_ASSERT(val2 && *val2 == 0);
	CORE_PASS("test_integer_overflow");
}

void test_boundary_conditions() {
	LockFreeQueue<uint32_t, 16> queue;
	constexpr uint32_t kNumElements = 100000;
	
	std::atomic<uint32_t> pushed{0};
	std::atomic<uint32_t> popped{0};
	
	// Fast producer, slow consumer -> Full Queue
	std::jthread p1([&queue, &pushed]() {
		for (uint32_t i = 0; i < kNumElements; ++i) {
			while (!queue.push(i)) {}
			pushed++;
		}
	});
	
	std::jthread c1([&queue, &popped]() {
		for (uint32_t i = 0; i < kNumElements; ++i) {
			std::this_thread::yield(); 
			while (!queue.pop()) {}
			popped++;
		}
	});
	
	p1.join();
	c1.join();
	CORE_ASSERT(pushed.load() == kNumElements);
	CORE_ASSERT(popped.load() == kNumElements);
	
	// Slow producer, fast consumer -> Empty Queue
	pushed = 0;
	popped = 0;
	
	std::jthread p2([&queue, &pushed]() {
		for (uint32_t i = 0; i < kNumElements; ++i) {
			std::this_thread::yield(); 
			while (!queue.push(i)) {}
			pushed++;
		}
	});
	
	std::jthread c2([&queue, &popped]() {
		for (uint32_t i = 0; i < kNumElements; ++i) {
			while (!queue.pop()) {}
			popped++;
		}
	});
	
	p2.join();
	c2.join();
	CORE_ASSERT(pushed.load() == kNumElements);
	CORE_ASSERT(popped.load() == kNumElements);

	CORE_PASS("test_boundary_conditions");
}

int main() {
	test_single_thread();
	test_spsc_no_allocation_and_determinism();
	test_integer_overflow();
	test_boundary_conditions();
	return 0;
}
