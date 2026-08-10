#pragma once

#include <memory_resource>
#include <array>
#include <cstddef>

namespace core::memory {

/**
 * @brief A statically sized monotonic memory pool designed for embedded systems.
 *        Memory is pre-allocated on the stack or BSS (if global), and allocations
 *        are drawn from this buffer monotonically. Deallocations are no-ops.
 *        The entire memory is reclaimed when the pool is destroyed or reset.
 * 
 * @tparam BufferSize The size of the memory buffer in bytes.
 */
template <std::size_t BufferSize>
class MonotonicMemoryPool {
public:
	// We use null_memory_resource() as upstream to strictly forbid heap allocations
	// if the pool runs out of memory. This prevents non-deterministic latency spikes.
	MonotonicMemoryPool() 
		: m_resource(m_buffer.data(), m_buffer.size(), std::pmr::null_memory_resource()) {}

	// Delete copy and move semantics since the resource holds pointers to the internal array
	MonotonicMemoryPool(const MonotonicMemoryPool&) = delete;
	MonotonicMemoryPool& operator=(const MonotonicMemoryPool&) = delete;
	MonotonicMemoryPool(MonotonicMemoryPool&&) = delete;
	MonotonicMemoryPool& operator=(MonotonicMemoryPool&&) = delete;

	/**
	 * @brief Get a polymorphic allocator bound to this memory pool.
	 *        This can be passed to pmr containers (e.g. std::pmr::vector).
	 */
	std::pmr::polymorphic_allocator<std::byte> get_allocator() {
		return std::pmr::polymorphic_allocator<std::byte>(&m_resource);
	}

	/**
	 * @brief Allocate memory of given size and alignment.
	 */
	void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept {
		try {
			return m_resource.allocate(bytes, alignment);
		} catch (const std::bad_alloc&) {
			return nullptr;
		}
	}

	/**
	 * @brief Reset the memory pool, freeing all allocated memory at once.
	 *        WARNING: Calling this invalidates all pointers previously allocated!
	 *        Only call this at the end of a request/cycle when all objects in the pool
	 *        are guaranteed to be no longer used.
	 */
	void reset() {
		m_resource.release();
	}

	/**
	 * @brief Direct access to the underlying memory resource if needed by advanced PMR APIs.
	 */
	std::pmr::memory_resource* resource() {
		return &m_resource;
	}

private:
	alignas(std::max_align_t) std::array<std::byte, BufferSize> m_buffer;
	std::pmr::monotonic_buffer_resource m_resource;
};

// RISK REVIEW:
// 1. Memory Exhaustion: Will return nullptr if BufferSize is exceeded, since upstream is null.
// 2. Destructors: pmr allocators do NOT call destructors automatically. If complex objects 
//    are allocated here, their destructors must be called manually before reset().

} // namespace core::memory
