#ifndef MODERN_CPP_CORE_CONCURRENCY_THREAD_POOL_HPP
#define MODERN_CPP_CORE_CONCURRENCY_THREAD_POOL_HPP

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <expected>
#include <system_error>
#include "fixed_task.hpp"

namespace core::concurrency {

/**
 * @brief A modern C++20 Thread Pool utilizing std::jthread and std::stop_token.
 * 
 * Uses standard C++20 synchronization primitives. Features a factory method 
 * returning std::expected to safely handle OS thread allocation failures.
 */
class JThreadPool {
public:
	/**
	 * @brief Factory method to create a thread pool.
	 * @param thread_count The number of threads to spawn.
	 * @return std::expected containing the pool on success, or an error code on failure.
	 */
	[[nodiscard]] static std::expected<std::unique_ptr<JThreadPool>, std::error_code> 
    create(std::size_t thread_count = std::thread::hardware_concurrency());

	~JThreadPool();

	// Delete copy and move constructors
	JThreadPool(const JThreadPool&) = delete;
	JThreadPool& operator=(const JThreadPool&) = delete;
	JThreadPool(JThreadPool&&) = delete;
	JThreadPool& operator=(JThreadPool&&) = delete;

	/**
	 * @brief Submits a fire-and-forget task without allocating any futures or promises.
	 * @note This uses FixedTask to guarantee zero hidden heap allocations.
	 * 
	 * @tparam F Callable type
	 * @param f The callable function to execute.
	 */
	template<typename F>
	void execute(F&& f) {
		{
			std::scoped_lock lock(m_queue_mutex);
			if (m_is_shutdown) {
				return;
			}
			m_task_queue.emplace(std::forward<F>(f));
		}
		m_cv.notify_one();
	}

private:
	// Private constructor used by create()
	explicit JThreadPool(std::size_t thread_count);

	/**
	 * @brief The worker loop executed by each thread.
	 * @param stoken The stop token provided by std::jthread.
	 */
	void worker_loop(std::stop_token stoken);

	std::vector<std::jthread> m_workers;
	std::queue<FixedTask<64>> m_task_queue;
	
	std::mutex m_queue_mutex;
	std::condition_variable_any m_cv;
	bool m_is_shutdown{false};
};

} // namespace core::concurrency

// RISK REVIEW:
// - Edge cases: Calling execute() on a stopped pool ignores the task. Thread creation limits (EAGAIN) handled via expected.
// - Concurrency Risks: The task queue uses a std::mutex. Caller is responsible for data races inside tasks.
// - Caller Responsibilities: Caller must ensure task capture size fits within FixedTask<64>.

#endif // MODERN_CPP_CORE_CONCURRENCY_THREAD_POOL_HPP
