#include <iostream>
#include <cassert>
#include <vector>
#include "../coroutines/generator.hpp"
#include "../coroutines/task.hpp"

using namespace core::coroutines;

// 1. Test Generator
Generator<int> fibonacci(int max_n) {
	int a = 0;
	int b = 1;
	for (int i = 0; i < max_n; ++i) {
		co_yield a;
		int next = a + b;
		a = b;
		b = next;
	}
}

void test_generator() {
	std::cout << "Testing core::coroutines::Generator\n";
	std::vector<int> expected = {0, 1, 1, 2, 3, 5, 8, 13};
	
	int idx = 0;
	for (int val : fibonacci(8)) {
		assert(val == expected[idx++]);
	}
	assert(idx == 8);
	std::cout << "Generator test passed.\n";
}

// Test Generator without copying
struct NoCopy {
	int val;
	NoCopy(int v) : val(v) {}
	NoCopy(const NoCopy&) = delete;
	NoCopy(NoCopy&&) = default;
	NoCopy& operator=(const NoCopy&) = delete;
	NoCopy& operator=(NoCopy&&) = default;
};

Generator<NoCopy> generate_no_copy() {
	co_yield NoCopy{42};
}

void test_generator_no_copy() {
	std::cout << "Testing core::coroutines::Generator without copy\n";
	for (const auto& item : generate_no_copy()) {
		assert(item.val == 42);
	}
	std::cout << "Generator no-copy test passed.\n";
}

// 2. Test Task (Lazy evaluation)
bool task_started = false;

Task<int> compute_value() {
	task_started = true;
	co_return 42;
}

Task<int> async_operation() {
	int val = co_await compute_value();
	co_return val * 2;
}

// Simple task runner that stores result
template<typename T>
struct SyncWaitTask {
	struct promise_type {
		T m_value;
		SyncWaitTask get_return_object() { return SyncWaitTask{std::coroutine_handle<promise_type>::from_promise(*this)}; }
		std::suspend_never initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }
		void return_value(T v) { m_value = v; }
		void unhandled_exception() { std::terminate(); }
	};
	std::coroutine_handle<promise_type> m_coro;
	T get() { return m_coro.promise().m_value; }
	~SyncWaitTask() { if (m_coro) m_coro.destroy(); }
};

SyncWaitTask<int> run_async_operation() {
	co_return co_await async_operation();
}

void test_task_full() {
	std::cout << "Testing core::coroutines::Task full run\n";
	task_started = false;
	auto runner = run_async_operation();
	
	assert(task_started);
	assert(runner.get() == 84); // 42 * 2
	
	std::cout << "Task test passed.\n";
}

int main() {
	test_generator();
	test_generator_no_copy();
	
	task_started = false;
	auto lazy_task = compute_value();
	assert(!task_started); // Lazy eval works
	
	test_task_full();
	
	std::cout << "All core::coroutines tests passed!\n";
	return 0;
}
