#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <atomic>
#include "test_utils.hpp"

// In C++20, we can use `constinit` for objects with static or thread storage duration.
// It forces the compiler to initialize the variable at compile-time (constant initialization).
// This eliminates the "Static Initialization Order Fiasco" and guarantees that when any
// thread first accesses the variable, it is ALREADY fully initialized. No runtime locks needed!
//
// However, `constinit` requires the constructor to be `constexpr`. For Singletons that
// require runtime initialization (e.g., opening a file, reading config), we should
// rely on standard C++ Block-scope static variables (often called Magic Statics, since C++11)
// instead of `std::call_once`.

struct HardwareConfig {
	int max_voltage;
	int max_current;
	
	// Must be a constexpr constructor to be constinit compatible
	constexpr HardwareConfig(int v, int c) : max_voltage(v), max_current(c) {}
};

// Global config initialized at compile time.
// `constinit` ensures we don't accidentally do dynamic initialization here.
constinit HardwareConfig g_global_config(220, 10);

// For compile-time static state, we use constinit global variables.
class CompileTimeConfigState {
public:
	constexpr CompileTimeConfigState() : m_log_count(0) {}
	
	void log() {
		// Non-atomic for simplicity of this test, but just demonstrating access
		m_log_count++;
	}
	
	int get_count() const { return m_log_count; }

private:
	int m_log_count;
};

// Compile-time initialized instance
constinit CompileTimeConfigState g_logger;

// For runtime Singletons, we use Block-scope static variables (Magic Statics).
class RuntimeLogger {
public:
	static RuntimeLogger& get_instance() {
		// C++11 Magic Static: Thread-safe lazy initialization without std::call_once
		static RuntimeLogger instance;
		return instance;
	}

	void log() { m_log_count.fetch_add(1, std::memory_order_relaxed); }
	int get_count() const { return m_log_count.load(std::memory_order_relaxed); }

private:
	RuntimeLogger() : m_log_count(0) {
		// Imagine runtime logic here like opening a file descriptor
	}
	std::atomic<int> m_log_count;
};

void test_constinit_access() {
	// Start multiple threads reading the config simultaneously.
	// Since it's constinit, there is ZERO chance of a race condition
	// on the initialization of g_global_config or g_logger.
	auto worker = []() {
		CORE_ASSERT(g_global_config.max_voltage == 220);
		CORE_ASSERT(g_global_config.max_current == 10);
		
		// Access runtime singleton
		RuntimeLogger::get_instance().log();
	};

	std::vector<std::jthread> threads;
	for (int i = 0; i < 10; ++i) {
		threads.emplace_back(worker);
	}

	std::cout << "test_constinit_access passed.\n";
}

int main() {
	test_constinit_access();
	std::cout << "All constinit tests passed.\n";
	return 0;
}
