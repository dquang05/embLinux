#ifndef MODERN_CPP_CORE_COROUTINES_CHANNEL_HPP
#define MODERN_CPP_CORE_COROUTINES_CHANNEL_HPP

#include <coroutine>
#include <optional>
#include <queue>
#include "../concurrency/spinlock.hpp"

namespace core::coroutines {

/**
 * @brief A bounded CSP Channel for coroutine communication without blocking threads.
 */
template <typename T, std::size_t Capacity>
class Channel {
public:
	Channel() = default;

	auto send(T value) {
		struct Awaiter {
			Channel& ch;
			T val;

			bool await_ready() {
				std::coroutine_handle<> to_resume;
				{
					std::scoped_lock lock(ch.m_mtx);
					if (ch.m_queue.size() < Capacity) {
						ch.m_queue.push(std::move(val));
						if (!ch.m_recv_waiters.empty()) {
							to_resume = ch.m_recv_waiters.front();
							ch.m_recv_waiters.pop();
						}
					} else {
						return false; // suspend
					}
				}
				if (to_resume) to_resume.resume();
				return true;
			}

			void await_suspend(std::coroutine_handle<> coro) {
				std::scoped_lock lock(ch.m_mtx);
				ch.m_send_waiters.push({coro, std::move(val)});
			}

			void await_resume() noexcept {}
		};
		return Awaiter{*this, std::move(value)};
	}

	auto receive() {
		struct Awaiter {
			Channel& ch;
			std::optional<T> val;

			bool await_ready() {
				std::coroutine_handle<> to_resume;
				{
					std::scoped_lock lock(ch.m_mtx);
					if (!ch.m_queue.empty()) {
						val = std::move(ch.m_queue.front());
						ch.m_queue.pop();
						
						if (!ch.m_send_waiters.empty()) {
							auto [coro, send_val] = std::move(ch.m_send_waiters.front());
							ch.m_send_waiters.pop();
							ch.m_queue.push(std::move(send_val));
							to_resume = coro;
						}
					} else {
						return false; // suspend
					}
				}
				if (to_resume) to_resume.resume();
				return true;
			}

			void await_suspend(std::coroutine_handle<> coro) {
				std::scoped_lock lock(ch.m_mtx);
				ch.m_recv_waiters.push(coro);
			}

			T await_resume() {
				if (val.has_value()) {
					return std::move(*val);
				}
				
				std::coroutine_handle<> to_resume;
				T res;
				{
					std::scoped_lock lock(ch.m_mtx);
					res = std::move(ch.m_queue.front());
					ch.m_queue.pop();
					
					if (!ch.m_send_waiters.empty()) {
						auto [coro, send_val] = std::move(ch.m_send_waiters.front());
						ch.m_send_waiters.pop();
						ch.m_queue.push(std::move(send_val));
						to_resume = coro;
					}
				}
				if (to_resume) to_resume.resume();
				return res;
			}
		};
		return Awaiter{*this, std::nullopt};
	}

private:
	concurrency::Spinlock m_mtx;
	std::queue<T> m_queue;
	
	struct SenderTask {
		std::coroutine_handle<> coro;
		T val;
	};
	
	std::queue<SenderTask> m_send_waiters;
	std::queue<std::coroutine_handle<>> m_recv_waiters;
};

} // namespace core::coroutines

// RISK REVIEW:
// - Deadlock Potential: Suspending while holding a lock is avoided by performing resumes outside the lock scope.
// - Scalability: Uses a spinlock. Suitable for very short critical sections, but under extreme contention with many threads, it could cause CPU spikes.

#endif // MODERN_CPP_CORE_COROUTINES_CHANNEL_HPP
