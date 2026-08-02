#include "thread_pool.hpp"

namespace core::concurrency {

JThreadPool::JThreadPool(std::size_t thread_count)
	: m_is_shutdown(false)
{
	if (thread_count == 0) {
		thread_count = 1;
	}

	m_workers.reserve(thread_count);
	for (std::size_t i = 0; i < thread_count; ++i) {
		m_workers.emplace_back([this](std::stop_token stoken) {
			this->worker_loop(stoken);
		});
	}
}

JThreadPool::~JThreadPool()
{
	{
		std::scoped_lock lock(m_queue_mutex);
		m_is_shutdown = true;
	}
	
	// Notify all workers to wake up and see the stop token / shutdown flag
	m_cv.notify_all();

	// std::jthread automatically requests stop and joins on destruction,
	// so we don't strictly need to do it manually. However, calling
	// request_stop() explicitly allows condition_variable_any to wake up
	// immediately if it is waiting on the stop_token.
	for (auto& worker : m_workers) {
		worker.request_stop();
	}
	
	// The vector destructor will block until all threads are joined.
}

void JThreadPool::worker_loop(std::stop_token stoken)
{
	while (!stoken.stop_requested()) {
		std::function<void()> task;

		{
			// Wait until there's a task or a stop is requested
			std::unique_lock lock(m_queue_mutex);
			
			// std::condition_variable_any allows waiting with a stop_token directly.
			// It will automatically unblock if request_stop() is called.
			m_cv.wait(lock, stoken, [this]() {
				return !m_task_queue.empty() || m_is_shutdown;
			});

			if (m_is_shutdown && m_task_queue.empty()) {
				return;
			}

			if (m_task_queue.empty()) {
				continue;
			}

			task = std::move(m_task_queue.front());
			m_task_queue.pop();
		}

		if (task) {
			task();
		}
	}
}

} // namespace core::concurrency

// RISK REVIEW:
// - Deadlocks: If a task submitted to the thread pool waits on a future of another task
//   submitted to the same pool, it can cause a deadlock if the pool size is small and all
//   threads are blocked (thread starvation). Work stealing can mitigate this, but currently
//   is not implemented. Caller must ensure they don't block threads indefinitely waiting 
//   for pool-dependent tasks.
// - Shutdown Delay: During destruction, pending tasks in the queue are NOT executed. 
//   The threads will process the currently active task and then exit. If the caller wants
//   to wait for all tasks to finish, they must implement a barrier or latch mechanism before
//   destroying the pool.
