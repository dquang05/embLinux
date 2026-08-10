#pragma once

#include <queue>
#include <mutex>
#include <semaphore>
#include <optional>
#include <concepts>

namespace core::data_structures {

/**
 * @brief A thread-safe queue leveraging C++20 concepts and counting_semaphore.
 * 
 * @tparam T The type of elements stored in the queue. Must satisfy std::movable.
 */
template <std::movable T>
class ThreadSafeQueue {
private:
	std::queue<T> m_queue;
	mutable std::mutex m_mutex;
	std::counting_semaphore<> m_sem{0};

public:
	ThreadSafeQueue() = default;
	~ThreadSafeQueue() = default;

	ThreadSafeQueue(const ThreadSafeQueue &) = delete;
	ThreadSafeQueue &operator=(const ThreadSafeQueue &) = delete;

	/**
	 * @brief Pushes an element into the queue.
	 * 
	 * @param value The value to be moved into the queue.
	 */
	void push(T value) {
		{
			std::scoped_lock lock(m_mutex);
			m_queue.push(std::move(value));
		}
		m_sem.release();
	}

	/**
	 * @brief Tries to pop an element from the queue without blocking.
	 * 
	 * @return std::optional<T> The popped element, or std::nullopt if the queue is empty.
	 */
	std::optional<T> try_pop() {
		std::scoped_lock lock(m_mutex);
		if (m_queue.empty()) {
			return std::nullopt;
		}
		T value = std::move(m_queue.front());
		m_queue.pop();
		return value;
	}

	/**
	 * @brief Waits until an element is available and pops it from the queue.
	 * 
	 * @return T The popped element.
	 */
	T wait_and_pop() {
		m_sem.acquire();
		std::scoped_lock lock(m_mutex);
		T value = std::move(m_queue.front());
		m_queue.pop();
		return value;
	}

	/**
	 * @brief Checks if the queue is empty.
	 * 
	 * @return true If the queue is empty.
	 * @return false If the queue contains elements.
	 */
	bool empty() const {
		std::scoped_lock lock(m_mutex);
		return m_queue.empty();
	}
};

// RISK REVIEW:
// 1. Concurrency: Uses std::mutex with std::scoped_lock/unique_lock for thread safety. Wait operations 
//    use std::condition_variable to avoid CPU spinning (blocking wait instead).
// 2. Caller responsibilities: Callers should prefer try_pop() if they do not want the thread to block. 
//    wait_and_pop() will block indefinitely until an item is pushed.
// 3. Performance: The queue uses coarse-grained locking (a single mutex for both push and pop). 
//    This might become a bottleneck under massive contention compared to a true LockFreeQueue.

} // namespace core::data_structures
