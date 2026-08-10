#pragma once

#include <atomic>
#include <thread>
#include <expected>

namespace core::data_structures {

constexpr size_t kMaxHazardPointers = 100;

struct HazardPointer {
	std::atomic<std::thread::id> id;
	std::atomic<void *> pointer;
};

inline HazardPointer g_hazard_pointers[kMaxHazardPointers];

enum class HazardPointerError {
	NoAvailableHazardPointers
};

class HazardPointerOwner {
private:
	HazardPointer *m_hp;

public:
	HazardPointerOwner(const HazardPointerOwner &) = delete;
	HazardPointerOwner &operator=(const HazardPointerOwner &) = delete;

	HazardPointerOwner() : m_hp(nullptr) {}

	bool try_acquire() {
		if (m_hp) return true;
		for (size_t i = 0; i < kMaxHazardPointers; ++i) {
			std::thread::id old_id;
			if (g_hazard_pointers[i].id.compare_exchange_strong(old_id, std::this_thread::get_id())) {
				m_hp = &g_hazard_pointers[i];
				return true;
			}
		}
		return false;
	}

	~HazardPointerOwner() {
		if (m_hp) {
			m_hp->pointer.store(nullptr, std::memory_order_release);
			m_hp->id.store(std::thread::id(), std::memory_order_release);
		}
	}

	std::atomic<void *> &get_pointer() {
		return m_hp->pointer;
	}

	bool is_valid() const {
		return m_hp != nullptr;
	}
};

inline std::expected<std::atomic<void *>*, HazardPointerError> get_hazard_pointer_for_current_thread() {
	thread_local HazardPointerOwner hp_owner;
	if (!hp_owner.is_valid()) {
		if (!hp_owner.try_acquire()) {
			return std::unexpected(HazardPointerError::NoAvailableHazardPointers);
		}
	}
	return &hp_owner.get_pointer();
}

inline bool outstanding_hazard_pointers_for(void *p) {
	for (size_t i = 0; i < kMaxHazardPointers; ++i) {
		if (g_hazard_pointers[i].pointer.load(std::memory_order_seq_cst) == p) {
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
	void (*deleter)(void *);
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
