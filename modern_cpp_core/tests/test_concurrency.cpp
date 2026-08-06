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

	// --- Edge Cases and Advanced Tests ---
	std::cout << "Running advanced tests...\n";

	// 1. Concurrent Submissions (Race Condition Test)
	{
		JThreadPool stress_pool(8);
		std::vector<std::future<int>> stress_futures;
		std::vector<std::jthread> submitters;
		std::mutex fut_mutex;
		
		for (int i = 0; i < 4; ++i) {
			submitters.emplace_back([&stress_pool, &stress_futures, &fut_mutex]() {
				for (int j = 0; j < 500; ++j) {
					auto fut = stress_pool.submit([j]() {
						return j * 2;
					});
					std::scoped_lock lock(fut_mutex);
					stress_futures.push_back(std::move(fut));
				}
			});
		}
		
		submitters.clear(); // wait for all submitters to finish
		
		long long total_sum = 0;
		for (auto& f : stress_futures) {
			total_sum += f.get();
		}
		assert(total_sum == 998000);
		std::cout << "Concurrent submissions test passed.\n";
	}

	// 2. Exception Safety
	{
		JThreadPool exception_pool(2);
		auto fut_throw = exception_pool.submit([]() -> int {
			throw std::runtime_error("Test Exception");
		});
		
		bool caught = false;
		try {
			fut_throw.get();
		} catch (const std::runtime_error&) {
			caught = true;
		}
		assert(caught);
		std::cout << "Exception safety test passed.\n";
	}
	
	// 3. Destruction with pending tasks
	{
		std::future<void> fut_pending;
		{
			JThreadPool drop_pool(1);
			
			// Submit a task that blocks for 50ms
			drop_pool.submit([]() {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			});
			
			// Submit a second task, since pool size is 1, it will sit in queue
			fut_pending = drop_pool.submit([]() {
			});
		} // drop_pool is destroyed here, requesting stop
		
		// The pending task's promise should be broken
		bool broken = false;
		try {
			fut_pending.get();
		} catch (const std::future_error& e) {
			if (e.code() == std::future_errc::broken_promise) {
				broken = true;
			}
		}
		assert(broken);
		std::cout << "Destruction with pending tasks test passed.\n";
	}

	std::cout << "All core::concurrency tests passed!\n";
	return 0;
}
