#pragma once

#include <atomic>
#include <concepts>

namespace core::patterns {

/**
 * @brief A high-performance thread-safe counter using std::memory_order_relaxed.
 * 
 * Relaxed counters do not provide synchronization or ordering guarantees with 
 * respect to other memory accesses. This eliminates cache synchronization overhead
 * across CPU cores, making it perfect for independent metrics like frame drops,
 * bytes processed, or error counts in real-time loops.
 * 
 * @tparam T An integral numeric type.
 */
template <std::integral T>
class RelaxedCounter {
private:
    std::atomic<T> m_value{0};

public:
    constexpr RelaxedCounter() noexcept = default;
    constexpr explicit RelaxedCounter(T initial_value) noexcept : m_value(initial_value) {}

    RelaxedCounter(const RelaxedCounter&) = delete;
    RelaxedCounter& operator=(const RelaxedCounter&) = delete;

    /**
     * @brief Increment the counter by 1.
     */
    void increment() noexcept {
        m_value.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * @brief Decrement the counter by 1.
     */
    void decrement() noexcept {
        m_value.fetch_sub(1, std::memory_order_relaxed);
    }

    /**
     * @brief Add a specific value to the counter.
     */
    void add(T val) noexcept {
        m_value.fetch_add(val, std::memory_order_relaxed);
    }

    /**
     * @brief Get the current value. 
     * @note Due to relaxed ordering, this might not reflect the most recent writes from other threads instantly.
     */
    T get() const noexcept {
        return m_value.load(std::memory_order_relaxed);
    }

    /**
     * @brief Reset the counter to 0.
     */
    void reset() noexcept {
        m_value.store(0, std::memory_order_relaxed);
    }
};

} // namespace core::patterns
