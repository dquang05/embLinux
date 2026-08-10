#ifndef MODERN_CPP_CORE_COROUTINES_EVENT_LOOP_HPP
#define MODERN_CPP_CORE_COROUTINES_EVENT_LOOP_HPP

#include <coroutine>
#include <memory>

namespace core::coroutines {

/**
 * @brief Singleton event loop for asynchronous operations.
 * 
 * Drives the execution of coroutines based on I/O events or timers.
 */
class EventLoop {
public:
	/**
	 * @brief Gets the singleton instance of the EventLoop.
	 * 
	 * @return EventLoop& The event loop instance.
	 */
	static EventLoop& get();

	/**
	 * @brief Runs the event loop forever, blocking the current OS thread.
	 */
	void run();

	/**
	 * @brief Processes pending events and returns immediately.
	 */
	void poll();

	/**
	 * @brief Stops the event loop.
	 */
	void stop();

	/**
	 * @brief Internal API to register a timerfd with a coroutine handle.
	 */
	void register_timer(int timer_fd, std::coroutine_handle<> coro);
	
	/**
	 * @brief Internal API to unregister a timerfd.
	 */
	void unregister_timer(int timer_fd);

private:
	EventLoop();
	~EventLoop();
	
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace core::coroutines

#endif // MODERN_CPP_CORE_COROUTINES_EVENT_LOOP_HPP
