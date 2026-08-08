#pragma once

#include <array>
#include <atomic>
#include <optional>
#include <new>

namespace core::data_structures {

#ifdef __cpp_lib_hardware_interference_size
	constexpr size_t kCacheLineSize = std::hardware_destructive_interference_size;
#else
	constexpr size_t kCacheLineSize = 64;
#endif

/**
 * @brief A high-performance lock-free Single-Producer Single-Consumer (SPSC) Ring Buffer.
 * 
 * @tparam T The type of elements stored in the queue.
 * @tparam Capacity The maximum number of elements the queue can hold.
 */
template <typename T, size_t Capacity>
class LockFreeQueue {
private:
	static_assert(Capacity > 0, "Capacity must be greater than 0");
	static constexpr size_t kRealCapacity = Capacity + 1;

	std::array<T, kRealCapacity> m_buffer;

	// Align head and tail to different cache lines to prevent False Sharing.
	alignas(kCacheLineSize) std::atomic<size_t> m_head{0};
	alignas(kCacheLineSize) std::atomic<size_t> m_tail{0};

public:
	LockFreeQueue() = default;
	~LockFreeQueue() = default;

	LockFreeQueue(const LockFreeQueue &) = delete;
	LockFreeQueue &operator=(const LockFreeQueue &) = delete;

	/**
	 * @brief Pushes an element into the queue (copy).
	 * 
	 * @param value The value to push.
	 * @return true If the push was successful.
	 * @return false If the queue was full.
	 */
	bool push(const T &value) {
		size_t current_tail = m_tail.load(std::memory_order_relaxed);
		size_t next_tail = (current_tail + 1) % kRealCapacity;

		// Acquire barrier to ensure we read the latest head written by the consumer
		if (next_tail == m_head.load(std::memory_order_acquire)) {
			return false;
		}

		m_buffer[current_tail] = value;
		
		// Release barrier to ensure data is written before tail is updated
		m_tail.store(next_tail, std::memory_order_release);
		return true;
	}

	/**
	 * @brief Pushes an element into the queue (move).
	 * 
	 * @param value The value to move.
	 * @return true If the push was successful.
	 * @return false If the queue was full.
	 */
	bool push(T &&value) {
		size_t current_tail = m_tail.load(std::memory_order_relaxed);
		size_t next_tail = (current_tail + 1) % kRealCapacity;

		if (next_tail == m_head.load(std::memory_order_acquire)) {
			return false;
		}

		m_buffer[current_tail] = std::move(value);
		m_tail.store(next_tail, std::memory_order_release);
		return true;
	}

	/**
	 * @brief Pops an element from the queue.
	 * 
	 * @return std::optional<T> The popped element, or std::nullopt if the queue is empty.
	 */
	std::optional<T> pop() {
		size_t current_head = m_head.load(std::memory_order_relaxed);

		// Acquire barrier to ensure we read the latest tail written by the producer
		if (current_head == m_tail.load(std::memory_order_acquire)) {
			return std::nullopt;
		}

		T value = std::move(m_buffer[current_head]);
		
		// Release barrier to ensure data is read before head is updated
		m_head.store((current_head + 1) % kRealCapacity, std::memory_order_release);
		return value;
	}
};

// RISK REVIEW:
// 1. Concurrency: This queue is lock-free but STRICTLY Single-Producer Single-Consumer (SPSC). 
//    Having multiple producers or multiple consumers will cause race conditions and data corruption.
//    Atomic memory orders (acquire/release) are heavily relied upon.
// 2. Caller responsibilities: Caller MUST ensure only one thread calls push() and one calls pop().
// 3. Memory: Fixed capacity at compile time. No heap allocation occurs during runtime.

} // namespace core::data_structures
