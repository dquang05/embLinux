#ifndef MODERN_CPP_CORE_CONCURRENCY_THREAD_POOL_HPP
#define MODERN_CPP_CORE_CONCURRENCY_THREAD_POOL_HPP

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <memory>
#include <type_traits>

namespace core::concurrency {

/**
 * @brief A modern C++20 Thread Pool utilizing std::jthread and std::stop_token.
 * 
 * Provides a robust, simple task queue mechanism for executing tasks asynchronously.
 * Uses standard C++20 synchronization primitives and returns std::future for task results.
 */
class JThreadPool {
public:
	/**
	 * @brief Constructs a thread pool with a specified number of threads.
	 * @param thread_count The number of threads to spawn. Defaults to hardware concurrency.
	 */
	explicit JThreadPool(std::size_t thread_count = std::thread::hardware_concurrency());

	/**
	 * @brief Destructor automatically stops all threads safely and joins them via std::jthread.
	 */
	~JThreadPool();

	// Delete copy and move constructors to prevent accidental duplication
	JThreadPool(const JThreadPool&) = delete;
	JThreadPool& operator=(const JThreadPool&) = delete;
	JThreadPool(JThreadPool&&) = delete;
	JThreadPool& operator=(JThreadPool&&) = delete;

	/**
	 * @brief Submits a task to the thread pool for execution.
	 * 
	 * @tparam F Type of the callable function.
	 * @tparam Args Types of the arguments.
	 * @param f The callable function to execute.
	 * @param args The arguments to pass to the function.
	 * @return A std::future representing the eventual result of the task.
	 */
	template<typename F, typename... Args>
	auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>
	{
		using return_type = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
		
		// Wrap the task in a shared_ptr so it can be copied into a std::function<void()>
		auto task = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		
		std::future<return_type> res = task->get_future();
		
		{
			std::scoped_lock lock(m_queue_mutex);
			if (m_is_shutdown) {
				// We don't use exceptions per rules, but submitting to a stopped pool is an error.
				// For now, if shut down, we just don't execute it. The future will block indefinitely or 
				// we could throw a std::runtime_error (but we want to avoid exceptions).
				// Since we can't return std::expected from this signature, we just do nothing and return a broken promise.
				// Wait, returning a broken promise will throw std::future_error on get().
				// This is a known limitation of the current standard future approach when avoiding exceptions.
			} else {
				m_task_queue.emplace([task]() { (*task)(); });
			}
		}
		
		m_cv.notify_one();
		return res;
	}

private:
	/**
	 * @brief The worker loop executed by each thread.
	 * @param stoken The stop token provided by std::jthread to signal cancellation.
	 */
	void worker_loop(std::stop_token stoken);

	std::vector<std::jthread> m_workers;
	std::queue<std::function<void()>> m_task_queue;
	
	std::mutex m_queue_mutex;
	std::condition_variable_any m_cv;
	bool m_is_shutdown;
};

} // namespace core::concurrency

#endif // MODERN_CPP_CORE_CONCURRENCY_THREAD_POOL_HPP
