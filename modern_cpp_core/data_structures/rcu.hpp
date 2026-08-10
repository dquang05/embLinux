#ifndef MODERN_CPP_CORE_DATA_STRUCTURES_RCU_HPP
#define MODERN_CPP_CORE_DATA_STRUCTURES_RCU_HPP

#include <atomic>
#include <thread>
#include <array>
#include <stdexcept>
#include <system_error>

namespace core::data_structures {

/**
 * @brief Maximum number of concurrent threads that can participate in RCU.
 * Kept small for embedded systems to minimize synchronization iteration overhead.
 */
constexpr size_t kMaxRcuThreads = 64;

/**
 * @brief Represents an RCU domain managing reader epochs and synchronization.
 * Loosely models the C++26 std::rcu_domain proposal.
 */
class rcu_domain {
private:
	struct alignas(std::hardware_destructive_interference_size) ThreadRecord {
		std::atomic<uint64_t> epoch{0}; // 0 means inactive/not in read-side critical section
		std::atomic<bool> in_use{false};
	};

	std::array<ThreadRecord, kMaxRcuThreads> m_threads;
	std::atomic<uint64_t> m_global_epoch{1};

public:
	rcu_domain() = default;

	~rcu_domain() = default;
	rcu_domain(const rcu_domain&) = delete;
	rcu_domain& operator=(const rcu_domain&) = delete;

	/**
	 * @brief Registers a thread with the domain.
	 * @return The slot index assigned to the thread.
	 * @throws std::runtime_error if maximum thread limit is reached.
	 */
	size_t register_thread() {
		for (size_t i = 0; i < kMaxRcuThreads; ++i) {
			bool expected = false;
			if (m_threads[i].in_use.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
				m_threads[i].epoch.store(0, std::memory_order_release);
				return i;
			}
		}
		throw std::runtime_error("RCU: Maximum thread limit exceeded (kMaxRcuThreads)");
	}

	/**
	 * @brief Unregisters a thread from the domain.
	 */
	void unregister_thread(size_t slot) {
		m_threads[slot].epoch.store(0, std::memory_order_release);
		m_threads[slot].in_use.store(false, std::memory_order_release);
	}

	/**
	 * @brief Enters a read-side critical section.
	 */
	void lock(size_t slot) {
		uint64_t current_epoch = m_global_epoch.load(std::memory_order_acquire);
		
		// exchange with seq_cst acts as a full memory barrier, preventing Store-Load reordering.
		m_threads[slot].epoch.exchange(current_epoch, std::memory_order_seq_cst);
	}

	/**
	 * @brief Exits a read-side critical section.
	 */
	void unlock(size_t slot) {
		m_threads[slot].epoch.store(0, std::memory_order_release);
	}

	/**
	 * @brief Blocks until all pre-existing read-side critical sections have completed.
	 */
	void synchronize() {
		// Advance the global epoch with seq_cst to act as a full memory barrier.
		// This ensures any previous pointer swaps are globally visible before we check reader epochs.
		uint64_t new_epoch = m_global_epoch.fetch_add(1, std::memory_order_seq_cst) + 1;

		// Wait for all threads to either be inactive (0) or reach the new epoch
		for (size_t i = 0; i < kMaxRcuThreads; ++i) {
			if (!m_threads[i].in_use.load(std::memory_order_acquire)) {
				continue;
			}
			
			// Spin-wait for this specific thread
			while (true) {
				uint64_t thread_epoch = m_threads[i].epoch.load(std::memory_order_acquire);
				
				// A thread is safe if:
				// 1. It is not currently in a read-side critical section (epoch == 0)
				// 2. It entered a read-side critical section AFTER we swapped the pointer
				//    and incremented the global epoch (thread_epoch >= new_epoch).
				if (thread_epoch == 0 || thread_epoch >= new_epoch) {
					break;
				}
				
				// Yield to prevent CPU hogging
				std::this_thread::yield();
			}
		}
	}
};

/**
 * @brief Retrieves the global default RCU domain.
 */
inline rcu_domain& get_default_domain() {
	static rcu_domain domain;
	return domain;
}

/**
 * @brief Thread-local context manager for RCU thread registration.
 */
class RcuThreadContext {
public:
	size_t slot;
	rcu_domain& domain;

	explicit RcuThreadContext(rcu_domain& dom) : domain(dom) {
		slot = domain.register_thread();
	}
	~RcuThreadContext() {
		domain.unregister_thread(slot);
	}
};

/**
 * @brief Retrieves the RCU context for the calling thread.
 */
inline RcuThreadContext& get_rcu_thread_context(rcu_domain& domain = get_default_domain()) {
	// Thread-local variables are initialized once per thread.
	thread_local RcuThreadContext context(domain);
	return context;
}

/**
 * @brief RAII wrapper for a read-side critical section.
 * Loosely models the C++26 std::scoped_rcu_reader proposal.
 */
class scoped_rcu_reader {
private:
	rcu_domain& m_domain;
	size_t m_slot;

public:
	explicit scoped_rcu_reader(rcu_domain& domain = get_default_domain()) 
		: m_domain(domain), m_slot(get_rcu_thread_context(domain).slot) {
		m_domain.lock(m_slot);
	}

	~scoped_rcu_reader() {
		m_domain.unlock(m_slot);
	}

	scoped_rcu_reader(const scoped_rcu_reader&) = delete;
	scoped_rcu_reader& operator=(const scoped_rcu_reader&) = delete;
};

/**
 * @brief Global synchronize function for the default domain.
 */
inline void rcu_synchronize(rcu_domain& domain = get_default_domain()) {
	domain.synchronize();
}

/**
 * @brief A high-level RCU Pointer wrapper for basic Read-Copy-Update operations.
 * 
 * @tparam T The type of data being protected.
 */
template <typename T>
class RcuPtr {
private:
	std::atomic<T*> m_ptr;
	rcu_domain& m_domain;

public:
	explicit RcuPtr(T* initial_ptr = nullptr, rcu_domain& domain = get_default_domain()) 
		: m_ptr(initial_ptr), m_domain(domain) {}

	~RcuPtr() {
		T* old = m_ptr.load(std::memory_order_relaxed);
		delete old;
	}

	RcuPtr(const RcuPtr&) = delete;
	RcuPtr& operator=(const RcuPtr&) = delete;

	/**
	 * @brief Reads the pointer. 
	 * @note MUST be called within the scope of a `scoped_rcu_reader`.
	 */
	const T* read() const {
		return m_ptr.load(std::memory_order_acquire);
	}

	/**
	 * @brief Updates the data by safely swapping and deferring destruction.
	 * 
	 * Copies the behavior of: Read, Copy, Update. The caller provides the newly
	 * copied and updated pointer. This function swaps it in, waits for all active
	 * readers to finish, and then deletes the old pointer.
	 * 
	 * @param new_ptr The new allocated pointer.
	 */
	void update(T* new_ptr) {
		T* old_ptr = m_ptr.exchange(new_ptr, std::memory_order_release);
		if (old_ptr) {
			m_domain.synchronize();
			delete old_ptr;
		}
	}
};

} // namespace core::data_structures

// RISK REVIEW:
// 1. Concurrency: Employs Epoch-based RCU with thread-local registry slots. `std::atomic_thread_fence(seq_cst)` is 
//    critically required and used to avoid Store-Load reordering between epoch registration and pointer reads.
// 2. Caller Responsibilities: Callers MUST instantiate `scoped_rcu_reader` before calling `RcuPtr::read()`. Failing 
//    to do so will result in Use-After-Free bugs.
// 3. Performance: Write path `update()` blocks heavily on spinning yields. This is intended for read-heavy, write-rare 
//    workloads. Writers should not be placed in real-time loops.

#endif // MODERN_CPP_CORE_DATA_STRUCTURES_RCU_HPP
