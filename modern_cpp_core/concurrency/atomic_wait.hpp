#ifndef MODERN_CPP_CORE_CONCURRENCY_ATOMIC_WAIT_HPP
#define MODERN_CPP_CORE_CONCURRENCY_ATOMIC_WAIT_HPP

#include <atomic>
#include <concepts>

namespace core::concurrency {

/**
 * @brief Waits on an atomic variable until a condition is met, handling spurious wakeups.
 * 
 * @tparam T The underlying type of the atomic variable.
 * @tparam Predicate A callable that returns true when the condition is met.
 * @param atomic_var The atomic variable to wait on.
 * @param old_val The expected old value before waiting (usually load() result).
 * @param pred The predicate to evaluate after waking up.
 */
template <typename T, std::predicate<> Predicate>
inline void atomic_wait_until(std::atomic<T>& atomic_var, T old_val, Predicate pred) {
    while (!pred()) {
        atomic_var.wait(old_val, std::memory_order_relaxed);
        old_val = atomic_var.load(std::memory_order_relaxed);
    }
}

/**
 * @brief Notifies one thread waiting on the atomic variable.
 */
template <typename T>
inline void atomic_notify_one(std::atomic<T>& atomic_var) noexcept {
    atomic_var.notify_one();
}

/**
 * @brief Notifies all threads waiting on the atomic variable.
 */
template <typename T>
inline void atomic_notify_all(std::atomic<T>& atomic_var) noexcept {
    atomic_var.notify_all();
}

} // namespace core::concurrency

// RISK REVIEW:
// - Edge cases: Predicate throwing an exception will bubble up.
// - Concurrency Risks: Memory order is relaxed for wait/load. The caller must ensure appropriate barriers or ACQ_REL memory orders if synchronizing complex data.
// - Caller Responsibilities: Ensure `old_val` is properly loaded before passing.

#endif // MODERN_CPP_CORE_CONCURRENCY_ATOMIC_WAIT_HPP
