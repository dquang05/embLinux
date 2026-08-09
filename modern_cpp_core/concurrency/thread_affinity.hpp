#ifndef MODERN_CPP_CORE_CONCURRENCY_THREAD_AFFINITY_HPP
#define MODERN_CPP_CORE_CONCURRENCY_THREAD_AFFINITY_HPP

#include <thread>
#include <expected>
#include <system_error>

namespace core::concurrency {

/**
 * @brief Pins a std::jthread to a specific CPU core to prevent cache ping-pong.
 * 
 * @param t The thread to pin.
 * @param core_id The ID of the CPU core (0 to std::thread::hardware_concurrency()-1).
 * @return std::expected<void, std::error_code> Error code if pinning fails.
 */
[[nodiscard]] std::expected<void, std::error_code> set_thread_affinity(std::jthread& t, int core_id) noexcept;

/**
 * @brief Promotes a std::jthread to Real-Time scheduling priority (SCHED_FIFO).
 * 
 * @param t The thread to promote.
 * @param priority The priority level (usually 1 to 99).
 * @return std::expected<void, std::error_code> Error code if promotion fails (e.g., EPERM).
 */
[[nodiscard]] std::expected<void, std::error_code> set_thread_realtime_priority(std::jthread& t, int priority) noexcept;

} // namespace core::concurrency

// RISK REVIEW:
// - Edge cases: core_id out of bounds will return EINVAL. Non-root user asking for SCHED_FIFO returns EPERM.
// - Concurrency Risks: None directly.
// - Caller Responsibilities: Caller MUST NOT ignore the returned [[nodiscard]] expected.

#endif // MODERN_CPP_CORE_CONCURRENCY_THREAD_AFFINITY_HPP
