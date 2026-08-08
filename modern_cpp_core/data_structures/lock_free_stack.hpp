#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include "hazard_pointer.hpp"

namespace core::data_structures {

/**
 * @brief A lock-free, thread-safe stack using Treiber's algorithm and Hazard Pointers.
 * 
 * @tparam T The type of elements stored in the stack.
 */
template <typename T>
class LockFreeStack {
private:
	struct Node {
		T data;
		Node *next;

		Node(T const &data_) : data(data_), next(nullptr) {
		}

		Node(T &&data_) : data(std::move(data_)), next(nullptr) {
		}
	};

	std::atomic<Node *> m_head{nullptr};

public:
	LockFreeStack() = default;
	
	~LockFreeStack() {
		while (pop()) {
			// drain
		}
	}

	LockFreeStack(const LockFreeStack &) = delete;
	LockFreeStack &operator=(const LockFreeStack &) = delete;

	/**
	 * @brief Pushes an element onto the stack.
	 * 
	 * @param value The value to push.
	 */
	void push(T const &value) {
		Node *new_node = new Node(value);
		new_node->next = m_head.load(std::memory_order_relaxed);
		while (!m_head.compare_exchange_weak(new_node->next, new_node, std::memory_order_release, std::memory_order_relaxed)) {
			// retry
		}
	}

	/**
	 * @brief Pushes an element onto the stack (move semantics).
	 * 
	 * @param value The value to push.
	 */
	void push(T &&value) {
		Node *new_node = new Node(std::move(value));
		new_node->next = m_head.load(std::memory_order_relaxed);
		while (!m_head.compare_exchange_weak(new_node->next, new_node, std::memory_order_release, std::memory_order_relaxed)) {
			// retry
		}
	}

	/**
	 * @brief Pops an element from the stack safely using Hazard Pointers.
	 * 
	 * @return std::optional<T> The popped element, or std::nullopt if the stack is empty.
	 */
	std::optional<T> pop() {
		std::atomic<void *> &hp = get_hazard_pointer_for_current_thread();
		Node *old_head = m_head.load(std::memory_order_relaxed);
		
		Node *temp;
		do {
			temp = old_head;
			hp.store(old_head, std::memory_order_release);
			old_head = m_head.load(std::memory_order_acquire);
		} while (old_head != temp);

		while (old_head && !m_head.compare_exchange_weak(old_head, old_head->next, std::memory_order_release, std::memory_order_relaxed)) {
			do {
				temp = old_head;
				hp.store(old_head, std::memory_order_release);
				old_head = m_head.load(std::memory_order_acquire);
			} while (old_head != temp);
		}

		hp.store(nullptr, std::memory_order_release);

		if (old_head) {
			T res = std::move(old_head->data);
			
			if (outstanding_hazard_pointers_for(old_head)) {
				reclaim_later(old_head);
			} else {
				delete old_head;
			}
			
			delete_nodes_with_no_hazards();
			return res;
		}

		return std::nullopt;
	}
};

// RISK REVIEW:
// 1. Concurrency: Achieves truly lock-free behavior without hidden spinlocks by utilizing Hazard Pointers.
//    Solves the ABA problem efficiently without relying on std::shared_ptr overhead.
// 2. Caller Responsibilities: Be aware that memory reclamation may be delayed. Long-running threads without
//    periodic cleanup might accumulate memory in the hazard pointers list if there is extremely high contention.
// 3. ABA Problem: Mitigated, as a node is never reused/deleted while a hazard pointer is pointing to it.

} // namespace core::data_structures
