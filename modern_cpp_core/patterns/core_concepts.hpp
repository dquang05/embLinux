#pragma once

#include <concepts>
#include <type_traits>
#include <iterator>

namespace core::patterns {

/**
 * @brief Concept for any numeric type (integral or floating point).
 */
template <typename T>
concept numeric = std::integral<T> || std::floating_point<T>;

/**
 * @brief Concept for a lockable type (e.g., std::mutex, std::shared_mutex, Spinlock).
 */
template <typename T>
concept lockable = requires(T t) {
    { t.lock() } -> std::same_as<void>;
    { t.try_lock() } -> std::same_as<bool>;
    { t.unlock() } -> std::same_as<void>;
};

/**
 * @brief Concept for a type that can be safely accessed concurrently via a read lock.
 * Requires the type to be lockable and support shared locking.
 */
template <typename T>
concept shared_lockable = lockable<T> && requires(T t) {
    { t.lock_shared() } -> std::same_as<void>;
    { t.try_lock_shared() } -> std::same_as<bool>;
    { t.unlock_shared() } -> std::same_as<void>;
};

} // namespace core::patterns
