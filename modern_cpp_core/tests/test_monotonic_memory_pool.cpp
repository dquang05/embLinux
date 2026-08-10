#include "../memory/monotonic_memory_pool.hpp"
#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <memory_resource>
#include "test_utils.hpp"

using core::memory::MonotonicMemoryPool;

void test_basic_allocation() {
	// 1KB buffer pool
	MonotonicMemoryPool<1024> pool;
	
	void* p1 = pool.allocate(100);
	CORE_ASSERT(p1 != nullptr);
	
	void* p2 = pool.allocate(200);
	CORE_ASSERT(p2 != nullptr);
	CORE_ASSERT(p1 != p2);

	// Monotonic means p2 is after p1
	CORE_ASSERT(static_cast<char*>(p2) > static_cast<char*>(p1));

	std::cout << "test_basic_allocation passed.\n";
}

void test_pmr_vector() {
	MonotonicMemoryPool<1024> pool;
	
	// Create a pmr::vector that uses our monotonic pool
	std::pmr::vector<int> vec(pool.get_allocator());
	
	for (int i = 0; i < 50; ++i) {
		vec.push_back(i);
	}
	
	CORE_ASSERT(vec.size() == 50);
	CORE_ASSERT(vec[49] == 49);
	
	// The vector memory should come from our local pool, not the heap.
	
	std::cout << "test_pmr_vector passed.\n";
}

void test_exhaustion() {
	// Very small pool (64 bytes)
	MonotonicMemoryPool<64> pool;
	
	void* p = pool.allocate(32);
	CORE_ASSERT(p != nullptr);
	
	// Should return nullptr because upstream is null_memory_resource
	// and we only have 32 bytes left, but asking for 64.
	void* p_fail = pool.allocate(64);
	CORE_ASSERT(p_fail == nullptr);
	
	std::cout << "test_exhaustion passed.\n";
}

void test_reset() {
	MonotonicMemoryPool<256> pool;
	
	// Allocate most of the pool
	pool.allocate(200);
	
	void* p_fail = pool.allocate(100); // Exceeds 256
	CORE_ASSERT(p_fail == nullptr);
	
	// Reset the pool, which should free everything
	pool.reset();
	
	// Now we can allocate 200 bytes again safely
	void* p = pool.allocate(200);
	CORE_ASSERT(p != nullptr);
	
	std::cout << "test_reset passed.\n";
}

int main() {
	test_basic_allocation();
	test_pmr_vector();
	test_exhaustion();
	test_reset();
	std::cout << "All MonotonicMemoryPool tests passed.\n";
	return 0;
}
