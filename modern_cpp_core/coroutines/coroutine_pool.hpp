#ifndef MODERN_CPP_CORE_COROUTINES_POOL_HPP
#define MODERN_CPP_CORE_COROUTINES_POOL_HPP

#include <cstddef>
#include <mutex>
#include "../concurrency/spinlock.hpp"

namespace core::coroutines {

/**
 * @brief Global memory pool for Coroutine frames to prevent OS mallocs during execution.
 * 
 * Must be initialized once at startup (Boot-time Initialization).
 */
class CoroutinePoolAllocator {
public:
	/**
	 * @brief Initializes the global memory pool. CALL THIS EXACTLY ONCE IN main()!
	 * 
	 * @param max_coroutines The maximum number of concurrent coroutines supported.
	 * @param block_size The maximum size in bytes of a single coroutine frame.
	 */
	static void init(std::size_t max_coroutines, std::size_t block_size = 256);

	/**
	 * @brief Cleans up the pool (usually only on application exit).
	 */
	static void destroy() noexcept;

	/**
	 * @brief Allocates a block from the pool. Throws std::bad_alloc if pool is exhausted or uninitialized.
	 */
	static void *allocate(std::size_t size);

	/**
	 * @brief Returns a block to the pool.
	 */
	static void deallocate(void *ptr, std::size_t size) noexcept;

private:
	struct Block {
		Block *next;
	};

	static std::byte *s_memory_buffer;
	static Block *s_free_list;
	static std::size_t s_block_size;
	static concurrency::Spinlock s_spinlock;
	static bool s_initialized;
};

} // namespace core::coroutines

// RISK REVIEW:
// - Edge cases: If a coroutine frame exceeds `block_size`, allocate() throws std::bad_alloc.
// - Concurrency Risks: Protected by an ultra-fast Spinlock, safe for concurrent allocation from multiple threads.

#endif // MODERN_CPP_CORE_COROUTINES_POOL_HPP
