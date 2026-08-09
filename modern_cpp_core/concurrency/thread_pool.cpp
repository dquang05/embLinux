#include "thread_pool.hpp"
#include <system_error>
#include <cerrno>

namespace core::concurrency {

std::expected<std::unique_ptr<JThreadPool>, std::error_code> JThreadPool::create(std::size_t thread_count) {
    if (thread_count == 0) {
        thread_count = 1;
    }

    auto pool = std::unique_ptr<JThreadPool>(new JThreadPool(thread_count));
    
    // We catch std::system_error which std::jthread constructor throws if OS thread creation fails.
    try {
        for (std::size_t i = 0; i < thread_count; ++i) {
            pool->m_workers.emplace_back([ptr = pool.get()](std::stop_token stoken) {
                ptr->worker_loop(stoken);
            });
        }
    } catch (const std::system_error& e) {
        // If thread creation fails (e.g. EAGAIN), we stop and join the threads that were successfully spawned.
        return std::unexpected(e.code());
    }

    return pool;
}

JThreadPool::JThreadPool(std::size_t thread_count)
    : m_is_shutdown(false)
{
    m_workers.reserve(thread_count);
}

JThreadPool::~JThreadPool()
{
    {
        std::scoped_lock lock(m_queue_mutex);
        m_is_shutdown = true;
    }
    
    // Notify all workers to wake up and see the shutdown flag
    m_cv.notify_all();

    // request_stop() allows condition_variable_any to unblock wait(stoken)
    for (auto& worker : m_workers) {
        worker.request_stop();
    }
}

void JThreadPool::worker_loop(std::stop_token stoken)
{
    while (!stoken.stop_requested()) {
        FixedTask<64> task;

        {
            std::unique_lock lock(m_queue_mutex);
            
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
// - Edge cases: OS fails to create threads -> Returns std::unexpected, caller must handle it.
// - Concurrency Risks: No deadlock inside pool logic. Worker wait logic is robust to spurious wakeups.
