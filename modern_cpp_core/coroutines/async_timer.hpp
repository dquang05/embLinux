#ifndef MODERN_CPP_CORE_COROUTINES_ASYNC_TIMER_HPP
#define MODERN_CPP_CORE_COROUTINES_ASYNC_TIMER_HPP

#include <coroutine>
#include <chrono>
#include <system_error>
#include "event_loop.hpp"

#if defined(__linux__) || defined(__gnu_linux__)
#include <sys/timerfd.h>
#include <unistd.h>
#endif

namespace core::coroutines {

/**
 * @brief An awaitable timer that uses Linux timerfd to wait asynchronously without blocking a thread.
 * 
 * Works in conjunction with the EventLoop to resume the coroutine when the timer expires.
 */
class AsyncTimer {
public:
	/**
	 * @brief Constructs an AsyncTimer with a specified timeout.
	 * 
	 * @param timeout The duration to wait in milliseconds.
	 */
	explicit AsyncTimer(std::chrono::milliseconds timeout) {
#if defined(__linux__) || defined(__gnu_linux__)
		m_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
		if (m_fd < 0) {
			throw std::system_error(errno, std::generic_category(), "Failed to create timerfd");
		}

		struct itimerspec its{};
		its.it_value.tv_sec = timeout.count() / 1000;
		its.it_value.tv_nsec = (timeout.count() % 1000) * 1000000;
		
		if (timerfd_settime(m_fd, 0, &its, nullptr) < 0) {
			close(m_fd);
			throw std::system_error(errno, std::generic_category(), "Failed to set timerfd");
		}
#else
		throw std::runtime_error("AsyncTimer requires Linux timerfd");
#endif
	}

	~AsyncTimer() {
		if (m_fd >= 0) {
			close(m_fd);
		}
	}

	// Non-copyable
	AsyncTimer(const AsyncTimer&) = delete;
	AsyncTimer& operator=(const AsyncTimer&) = delete;

	// Awaitable interface
	bool await_ready() const noexcept { return false; }
	
	void await_suspend(std::coroutine_handle<> coro) {
		EventLoop::get().register_timer(m_fd, coro);
	}
	
	void await_resume() noexcept {}

private:
	int m_fd{-1};
};

} // namespace core::coroutines

// RISK REVIEW:
// - Edge cases: Exhaustion of file descriptors throws system_error.

#endif // MODERN_CPP_CORE_COROUTINES_ASYNC_TIMER_HPP
