#ifndef MODERN_CPP_CORE_COROUTINES_GENERATOR_HPP
#define MODERN_CPP_CORE_COROUTINES_GENERATOR_HPP

#include <coroutine>
#include <exception>
#include <iterator>
#include <utility>
#include <cstddef>
#include <optional>

namespace core::coroutines {

/**
 * @brief A synchronous coroutine generator for lazily evaluating sequences.
 * 
 * Generates values of type T on demand using `co_yield`.
 * 
 * @tparam T The type of value generated.
 */
template <typename T>
class Generator {
public:
	struct promise_type;
	using handle_type = std::coroutine_handle<promise_type>;

	struct promise_type {
		std::optional<T> m_current_value;
		std::exception_ptr m_exception;

		Generator get_return_object() {
			return Generator(handle_type::from_promise(*this));
		}
		
		std::suspend_always initial_suspend() noexcept { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }
		
		template <typename U>
		std::suspend_always yield_value(U&& value) {
			m_current_value.emplace(std::forward<U>(value));
			return {};
		}
		
		void return_void() noexcept {}
		
		void unhandled_exception() {
			m_exception = std::current_exception();
		}
	};

	class iterator {
	public:
		using iterator_category = std::input_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = const T*;
		using reference = const T&;

		iterator() : m_coroutine(nullptr) {}
		explicit iterator(handle_type coro) : m_coroutine(coro) {}

		iterator& operator++() {
			if (m_coroutine) {
				m_coroutine.resume();
				if (m_coroutine.done()) {
					if (m_coroutine.promise().m_exception) {
						std::rethrow_exception(m_coroutine.promise().m_exception);
					}
					m_coroutine = nullptr;
				}
			}
			return *this;
		}

		void operator++(int) {
			++(*this);
		}

		bool operator==(const iterator& other) const {
			return m_coroutine == other.m_coroutine;
		}

		reference operator*() const {
			return *m_coroutine.promise().m_current_value;
		}

		pointer operator->() const {
			return &(*m_coroutine.promise().m_current_value);
		}

	private:
		handle_type m_coroutine;
	};

	Generator(const Generator&) = delete;
	Generator& operator=(const Generator&) = delete;

	Generator(Generator&& other) noexcept : m_coroutine(std::exchange(other.m_coroutine, nullptr)) {}

	Generator& operator=(Generator&& other) noexcept {
		if (this != &other) {
			if (m_coroutine) {
				m_coroutine.destroy();
			}
			m_coroutine = std::exchange(other.m_coroutine, nullptr);
		}
		return *this;
	}

	~Generator() {
		if (m_coroutine) {
			m_coroutine.destroy();
		}
	}

	iterator begin() {
		if (m_coroutine) {
			m_coroutine.resume();
			if (m_coroutine.done()) {
				if (m_coroutine.promise().m_exception) {
					std::rethrow_exception(m_coroutine.promise().m_exception);
				}
				return end();
			}
		}
		return iterator{m_coroutine};
	}

	iterator end() {
		return iterator{nullptr};
	}

private:
	explicit Generator(handle_type coro) : m_coroutine(coro) {}

	handle_type m_coroutine;
};

} // namespace core::coroutines

// RISK REVIEW:
// - Single-pass iteration: The generator mutates coroutine state upon traversal. Calling `begin()` twice will not restart the generation but continue or fail if done.
// - Dangling references: If `T` is a reference or a view type (e.g., `std::string_view`), callers must ensure the underlying lifetime outlives the lazy traversal of the Generator.

#endif // MODERN_CPP_CORE_COROUTINES_GENERATOR_HPP
