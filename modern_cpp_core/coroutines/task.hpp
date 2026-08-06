#ifndef MODERN_CPP_CORE_COROUTINES_TASK_HPP
#define MODERN_CPP_CORE_COROUTINES_TASK_HPP

#include <coroutine>
#include <exception>
#include <utility>

namespace core::coroutines {

/**
 * @brief A lazily evaluated coroutine task.
 * 
 * Execution does not start until the task is `co_await`ed.
 * 
 * @tparam T The return type of the task.
 */
template <typename T>
class Task {
public:
	struct promise_type;
	using handle_type = std::coroutine_handle<promise_type>;

	struct promise_type {
		T m_value;
		std::exception_ptr m_exception;
		std::coroutine_handle<> m_continuation;

		Task get_return_object() noexcept {
			return Task{handle_type::from_promise(*this)};
		}
		
		std::suspend_always initial_suspend() noexcept { return {}; }
		
		struct FinalAwaiter {
			bool await_ready() const noexcept { return false; }
			template <typename Promise>
			std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> coro) noexcept {
				if (coro.promise().m_continuation) {
					return coro.promise().m_continuation;
				}
				return std::noop_coroutine();
			}
			void await_resume() noexcept {}
		};
		
		FinalAwaiter final_suspend() noexcept { return {}; }
		
		template <typename U>
		void return_value(U&& value) noexcept {
			m_value = std::forward<U>(value);
		}
		
		void unhandled_exception() noexcept {
			m_exception = std::current_exception();
		}
	};

	Task(const Task&) = delete;
	Task& operator=(const Task&) = delete;

	Task(Task&& other) noexcept : m_coroutine(std::exchange(other.m_coroutine, nullptr)) {}
	
	Task& operator=(Task&& other) noexcept {
		if (this != &other) {
			if (m_coroutine) {
				m_coroutine.destroy();
			}
			m_coroutine = std::exchange(other.m_coroutine, nullptr);
		}
		return *this;
	}

	~Task() {
		if (m_coroutine) {
			m_coroutine.destroy();
		}
	}

	bool is_ready() const noexcept {
		return !m_coroutine || m_coroutine.done();
	}

	// Awaiter interface for `co_await Task`
	bool await_ready() const noexcept {
		return m_coroutine.done();
	}
	
	std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation) noexcept {
		m_coroutine.promise().m_continuation = continuation;
		return m_coroutine;
	}
	
	T await_resume() {
		if (m_coroutine.promise().m_exception) {
			std::rethrow_exception(m_coroutine.promise().m_exception);
		}
		return std::move(m_coroutine.promise().m_value);
	}

private:
	explicit Task(handle_type coro) noexcept : m_coroutine(coro) {}

	handle_type m_coroutine;
};

/**
 * @brief Task specialization for void return type.
 */
template <>
class Task<void> {
public:
	struct promise_type;
	using handle_type = std::coroutine_handle<promise_type>;

	struct promise_type {
		std::exception_ptr m_exception;
		std::coroutine_handle<> m_continuation;

		Task<void> get_return_object() noexcept {
			return Task<void>{handle_type::from_promise(*this)};
		}
		
		std::suspend_always initial_suspend() noexcept { return {}; }
		
		struct FinalAwaiter {
			bool await_ready() const noexcept { return false; }
			template <typename Promise>
			std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> coro) noexcept {
				if (coro.promise().m_continuation) {
					return coro.promise().m_continuation;
				}
				return std::noop_coroutine();
			}
			void await_resume() noexcept {}
		};
		
		FinalAwaiter final_suspend() noexcept { return {}; }
		
		void return_void() noexcept {}
		
		void unhandled_exception() noexcept {
			m_exception = std::current_exception();
		}
	};

	Task(const Task&) = delete;
	Task& operator=(const Task&) = delete;

	Task(Task&& other) noexcept : m_coroutine(std::exchange(other.m_coroutine, nullptr)) {}
	
	Task& operator=(Task&& other) noexcept {
		if (this != &other) {
			if (m_coroutine) {
				m_coroutine.destroy();
			}
			m_coroutine = std::exchange(other.m_coroutine, nullptr);
		}
		return *this;
	}

	~Task() {
		if (m_coroutine) {
			m_coroutine.destroy();
		}
	}

	bool is_ready() const noexcept {
		return !m_coroutine || m_coroutine.done();
	}

	bool await_ready() const noexcept {
		return m_coroutine.done();
	}
	
	std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation) noexcept {
		m_coroutine.promise().m_continuation = continuation;
		return m_coroutine;
	}
	
	void await_resume() {
		if (m_coroutine.promise().m_exception) {
			std::rethrow_exception(m_coroutine.promise().m_exception);
		}
	}

private:
	explicit Task(handle_type coro) noexcept : m_coroutine(coro) {}

	handle_type m_coroutine;
};

} // namespace core::coroutines

// RISK REVIEW:
// - Awaiting multiple times: std::coroutine_handle does not support being awaited multiple times safely if it consumes state.
//   Callers must ensure they co_await a Task only once.
// - Dangling references: Since Tasks are lazy, if they capture references to stack variables that go out of scope before the task is awaited, it leads to Undefined Behavior. Callers must capture by value or ensure lifetime.

#endif // MODERN_CPP_CORE_COROUTINES_TASK_HPP
