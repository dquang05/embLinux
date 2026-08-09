#ifndef MODERN_CPP_CORE_CONCURRENCY_SPINLOCK_HPP
#define MODERN_CPP_CORE_CONCURRENCY_SPINLOCK_HPP

#include <atomic>

// Handle compiler-specific backoff instructions
#if defined(__x86_64__) || defined(_M_X64)
  #include <emmintrin.h>
  #define CPU_BACKOFF() _mm_pause()
#elif defined(__aarch64__)
  #define CPU_BACKOFF() __asm__ volatile("yield" ::: "memory")
#else
  // Fallback for other architectures
  #define CPU_BACKOFF() 
#endif

namespace core::concurrency {

/**
 * @brief Ultra-fast Spinlock for extremely short critical sections.
 * 
 * Uses std::atomic_flag and a CPU backoff instruction to prevent livelocks 
 * and reduce power consumption during contention.
 * Complies with the BasicLockable concept.
 */
class Spinlock {
public:
    Spinlock() noexcept = default;
    
    // Non-copyable, non-movable
    Spinlock(const Spinlock&) = delete;
    Spinlock& operator=(const Spinlock&) = delete;
    Spinlock(Spinlock&&) = delete;
    Spinlock& operator=(Spinlock&&) = delete;

    void lock() noexcept {
        while (m_flag.test_and_set(std::memory_order_acquire)) {
            while (m_flag.test(std::memory_order_relaxed)) {
                CPU_BACKOFF();
            }
        }
    }

    void unlock() noexcept {
        m_flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
};

} // namespace core::concurrency

// RISK REVIEW:
// - Edge cases: CPU_BACKOFF may be empty on exotic architectures.
// - Concurrency Risks: Vulnerable to Priority Inversion if a high-priority thread spins waiting for a preempted low-priority thread.
// - Caller Responsibilities: ONLY use this for very short critical sections (e.g., updating a few pointers). DO NOT block, allocate memory, or perform I/O while holding a spinlock.

#endif // MODERN_CPP_CORE_CONCURRENCY_SPINLOCK_HPP
