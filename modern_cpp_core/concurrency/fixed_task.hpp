#ifndef MODERN_CPP_CORE_CONCURRENCY_FIXED_TASK_HPP
#define MODERN_CPP_CORE_CONCURRENCY_FIXED_TASK_HPP

#include <cstddef>
#include <type_traits>
#include <utility>
#include <new>

namespace core::concurrency {

/**
 * @brief A fixed-capacity, non-allocating callable wrapper to replace std::function
 * @note Guarantees 0 heap allocations, critical for Wait-Free data paths in Embedded Linux.
 */
template <size_t Capacity = 64>
class FixedTask {
private:
    struct Concept {
        virtual ~Concept() = default;
        virtual void invoke() = 0;
        virtual void move_to(void* dest) noexcept = 0;
    };

    template <typename F>
    struct Model final : Concept {
        F func;
        explicit Model(F&& f) : func(std::forward<F>(f)) {}
        void invoke() override { func(); }
        void move_to(void* dest) noexcept override {
            new (dest) Model(std::move(func));
        }
    };

    alignas(std::max_align_t) std::byte m_storage[Capacity];
    Concept* m_concept{nullptr};

public:
    FixedTask() noexcept = default;

    template <typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, FixedTask>>>
    FixedTask(F&& f) {
        using M = Model<std::decay_t<F>>;
        static_assert(sizeof(M) <= Capacity, "Lambda capture size exceeds FixedTask capacity! Hidden heap allocation prevented.");
        static_assert(alignof(M) <= alignof(std::max_align_t), "Alignment requirements too strict.");
        
        m_concept = new (m_storage) M(std::forward<F>(f));
    }

    FixedTask(FixedTask&& other) noexcept {
        if (other.m_concept) {
            other.m_concept->move_to(m_storage);
            m_concept = reinterpret_cast<Concept*>(m_storage);
            other.m_concept->~Concept();
            other.m_concept = nullptr;
        }
    }

    FixedTask& operator=(FixedTask&& other) noexcept {
        if (this != &other) {
            if (m_concept) {
                m_concept->~Concept();
                m_concept = nullptr;
            }
            if (other.m_concept) {
                other.m_concept->move_to(m_storage);
                m_concept = reinterpret_cast<Concept*>(m_storage);
                other.m_concept->~Concept();
                other.m_concept = nullptr;
            }
        }
        return *this;
    }

    FixedTask(const FixedTask&) = delete;
    FixedTask& operator=(const FixedTask&) = delete;

    ~FixedTask() {
        if (m_concept) {
            m_concept->~Concept();
        }
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return m_concept != nullptr;
    }

    void operator()() {
        if (m_concept) {
            m_concept->invoke();
        }
    }
};

} // namespace core::concurrency

// RISK REVIEW:
// - Edge cases: Calling an empty FixedTask is a no-op, it will not crash.
// - Concurrency Risks: The Callable state moved into this task is evaluated without synchronization here. Caller is responsible for data-races inside the lambda.
// - Caller Responsibilities: Caller must ensure the lambda capture size does not exceed `Capacity` (defaults to 64 bytes). Compile-time error otherwise.

#endif // MODERN_CPP_CORE_CONCURRENCY_FIXED_TASK_HPP
