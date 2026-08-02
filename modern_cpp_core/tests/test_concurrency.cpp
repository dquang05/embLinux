#include <iostream>
#include <cassert>
#include <chrono>
#include <numeric>
#include <vector>
#include "../concurrency/thread_pool.hpp"
#include "../concurrency/latch_barrier.hpp"
#include "../concurrency/semaphore.hpp"

using namespace core::concurrency;

int main()
{
	std::cout << "Testing core::concurrency::JThreadPool\n";
	
	JThreadPool pool(4);
	
	// Test basic submission
	auto fut1 = pool.submit([]() { return 42; });
	assert(fut1.get() == 42);
	
	// Test multiple tasks
	std::vector<std::future<int>> futures;
	for (int i = 0; i < 100; ++i) {
		futures.push_back(pool.submit([i]() { return i * 2; }));
	}
	
	int sum = 0;
	for (auto& f : futures) {
		sum += f.get();
	}
	assert(sum == (99 * 100)); // 99 * 100 = 9900

	std::cout << "Thread pool basic functionality tests passed.\n";

	// Test Latch
	Latch latch(2);
	pool.submit([&]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		latch.count_down();
	});
	pool.submit([&]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		latch.count_down();
	});
	
	latch.wait();
	std::cout << "Latch test passed.\n";
	
	// Test Semaphore
	CountingSemaphore<2> sem(2);
	sem.acquire();
	sem.acquire();
	
	auto fut_sem = pool.submit([&]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		sem.release();
		return true;
	});
	
	sem.acquire(); // Will block until thread releases
	assert(fut_sem.get());
	std::cout << "Semaphore test passed.\n";

	std::cout << "All core::concurrency tests passed!\n";
	return 0;
}
