#pragma once

#include <atomic>
#include <thread>
#include <functional>
#include <stdexcept>

namespace core::data_structures {

constexpr size_t kMaxHazardPointers = 100;

struct HazardPointer {
	std::atomic<std::thread::id> id;
	std::atomic<void *> pointer;
};

inline HazardPointer g_hazard_pointers[kMaxHazardPointers];

class HazardPointerOwner {
private:
	HazardPointer *m_hp;

public:
	HazardPointerOwner(const HazardPointerOwner &) = delete;
	HazardPointerOwner &operator=(const HazardPointerOwner &) = delete;

	HazardPointerOwner() : m_hp(nullptr) {
		for (size_t i = 0; i < kMaxHazardPointers; ++i) {
			std::thread::id old_id;
			if (g_hazard_pointers[i].id.compare_exchange_strong(old_id, std::this_thread::get_id())) {
				m_hp = &g_hazard_pointers[i];
				break;
			}
		}
		if (!m_hp) {
			throw std::runtime_error("No available hazard pointers");
		}
	}

	~HazardPointerOwner() {
		m_hp->pointer.store(nullptr, std::memory_order_release);
		m_hp->id.store(std::thread::id(), std::memory_order_release);
	}

	std::atomic<void *> &get_pointer() {
		return m_hp->pointer;
	}
};

inline std::atomic<void *> &get_hazard_pointer_for_current_thread() {
	thread_local HazardPointerOwner hp_owner;
	return hp_owner.get_pointer();
}

inline bool outstanding_hazard_pointers_for(void *p) {
	for (size_t i = 0; i < kMaxHazardPointers; ++i) {
		if (g_hazard_pointers[i].pointer.load(std::memory_order_acquire) == p) {
			return true;
		}
	}
	return false;
}

template <typename T>
void do_delete(void *p) {
	delete static_cast<T *>(p);
}

struct DataToReclaim {
	void *data;
	std::function<void(void *)> deleter;
	DataToReclaim *next;

	template <typename T>
	DataToReclaim(T *p) : data(p), deleter(&do_delete<T>), next(nullptr) {
	}

	~DataToReclaim() {
		deleter(data);
	}
};

inline std::atomic<DataToReclaim *> g_nodes_to_reclaim;

inline void add_to_reclaim_list(DataToReclaim *node) {
	node->next = g_nodes_to_reclaim.load(std::memory_order_relaxed);
	while (!g_nodes_to_reclaim.compare_exchange_weak(node->next, node, std::memory_order_release, std::memory_order_relaxed)) {
		// retry
	}
}

template <typename T>
void reclaim_later(T *data) {
	add_to_reclaim_list(new DataToReclaim(data));
}

inline void delete_nodes_with_no_hazards() {
	DataToReclaim *current = g_nodes_to_reclaim.exchange(nullptr, std::memory_order_acquire);
	while (current) {
		DataToReclaim *next = current->next;
		if (!outstanding_hazard_pointers_for(current->data)) {
			delete current;
		} else {
			add_to_reclaim_list(current);
		}
		current = next;
	}
}

} // namespace core::data_structures
