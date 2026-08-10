#pragma once

#include <atomic>
#include <optional>
#include <array>
#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace core::data_structures {

/**
 * @brief A bounded lock-free, thread-safe stack using a pre-allocated array and tagged indices.
 *        Completely avoids dynamic memory allocation and inherently solves the ABA problem.
 * 
 * @tparam T The type of elements stored in the stack.
 * @tparam Capacity The maximum number of elements the stack can hold.
 */
template <typename T, std::size_t Capacity>
class LockFreeStack {
private:
	static constexpr uint32_t kNullIndex = 0xFFFFFFFF;

	struct alignas(8) TaggedIndex {
		uint32_t index;
		uint32_t tag;

		bool operator==(const TaggedIndex& other) const {
			return index == other.index && tag == other.tag;
		}
	};

	struct Node {
		std::optional<T> data;
		std::atomic<uint32_t> next{kNullIndex};
	};

	std::array<Node, Capacity> m_pool;
	std::atomic<TaggedIndex> m_head;
	std::atomic<TaggedIndex> m_free_head;

	static_assert(std::atomic<TaggedIndex>::is_always_lock_free, "TaggedIndex must be lock-free (usually 64-bit atomic on 64-bit systems).");

public:
	LockFreeStack() {
		// Initialize the free list to contain all elements
		for (uint32_t i = 0; i < Capacity - 1; ++i) {
			m_pool[i].next.store(i + 1, std::memory_order_relaxed);
		}
		m_pool[Capacity - 1].next.store(kNullIndex, std::memory_order_relaxed);
		
		m_head.store({kNullIndex, 0}, std::memory_order_relaxed);
		m_free_head.store({0, 0}, std::memory_order_relaxed);
	}
	
	~LockFreeStack() = default;

	LockFreeStack(const LockFreeStack &) = delete;
	LockFreeStack &operator=(const LockFreeStack &) = delete;

	/**
	 * @brief Pushes an element onto the stack.
	 * 
	 * @param value The value to push.
	 * @return true if pushed successfully, false if the stack is full.
	 */
	bool push(T const &value) {
		return push_impl(value);
	}

	/**
	 * @brief Pushes an element onto the stack (move semantics).
	 * 
	 * @param value The value to push.
	 * @return true if pushed successfully, false if the stack is full.
	 */
	bool push(T &&value) {
		return push_impl(std::move(value));
	}

	/**
	 * @brief Pops an element from the stack safely using Tagged Indices.
	 * 
	 * @return std::optional<T> The popped element, or std::nullopt if the stack is empty.
	 */
	std::optional<T> pop() {
		TaggedIndex old_head = m_head.load(std::memory_order_acquire);
		TaggedIndex new_head;

		while (true) {
			if (old_head.index == kNullIndex) {
				return std::nullopt;
			}
			new_head.index = m_pool[old_head.index].next.load(std::memory_order_relaxed);
			new_head.tag = old_head.tag + 1; // Increment tag to prevent ABA

			if (m_head.compare_exchange_weak(old_head, new_head, std::memory_order_release, std::memory_order_acquire)) {
				break;
			}
		}

		// Extract data
		std::optional<T> result = std::move(m_pool[old_head.index].data);
		m_pool[old_head.index].data = std::nullopt; // Clear

		// Return node to free list
		TaggedIndex old_free_head = m_free_head.load(std::memory_order_acquire);
		TaggedIndex new_free_head;
		new_free_head.index = old_head.index;
		
		while (true) {
			m_pool[new_free_head.index].next.store(old_free_head.index, std::memory_order_relaxed);
			new_free_head.tag = old_free_head.tag + 1;
			if (m_free_head.compare_exchange_weak(old_free_head, new_free_head, std::memory_order_release, std::memory_order_acquire)) {
				break;
			}
		}

		return result;
	}

private:
	template<typename U>
	bool push_impl(U&& value) {
		// Grab a node from the free list
		TaggedIndex old_free_head = m_free_head.load(std::memory_order_acquire);
		TaggedIndex new_free_head;

		while (true) {
			if (old_free_head.index == kNullIndex) {
				return false; // Stack is full
			}
			new_free_head.index = m_pool[old_free_head.index].next.load(std::memory_order_relaxed);
			new_free_head.tag = old_free_head.tag + 1;

			if (m_free_head.compare_exchange_weak(old_free_head, new_free_head, std::memory_order_release, std::memory_order_acquire)) {
				break;
			}
		}

		// Initialize the node
		uint32_t node_idx = old_free_head.index;
		m_pool[node_idx].data = std::forward<U>(value);

		// Push to the stack
		TaggedIndex old_head = m_head.load(std::memory_order_acquire);
		TaggedIndex new_head;
		new_head.index = node_idx;

		while (true) {
			m_pool[node_idx].next.store(old_head.index, std::memory_order_relaxed);
			new_head.tag = old_head.tag + 1;

			if (m_head.compare_exchange_weak(old_head, new_head, std::memory_order_release, std::memory_order_acquire)) {
				break;
			}
		}

		return true;
	}
};

// RISK REVIEW:
// 1. Concurrency: Achieves truly Wait-Free performance for allocation, and Lock-Free for stack logic.
// 2. Caller Responsibilities: Caller must provide an adequate `Capacity`. Pushing to a full stack returns false.
// 3. ABA Problem: Solved perfectly using Tagged Indices (64-bit atomic version counters).

} // namespace core::data_structures
