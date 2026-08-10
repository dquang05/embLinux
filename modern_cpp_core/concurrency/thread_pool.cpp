#include "thread_pool.hpp"
#include <system_error>
#include <cerrno>

namespace core::concurrency {

static thread_local std::optional<std::size_t> tl_worker_index;

std::optional<std::size_t> JThreadPool::get_worker_index() {
    return tl_worker_index;
}

std::expected<std::unique_ptr<JThreadPool>, std::error_code> JThreadPool::create(std::size_t thread_count) {
    if (thread_count == 0) {
        thread_count = 1;
    }

    auto pool = std::unique_ptr<JThreadPool>(new JThreadPool(thread_count));
    
    for (std::size_t i = 0; i < thread_count; ++i) {
        pool->m_local_queues.push_back(std::make_unique<WorkerQueue>());
    }

    // We catch std::system_error which std::jthread constructor throws if OS thread creation fails.
    try {
        for (std::size_t i = 0; i < thread_count; ++i) {
            pool->m_workers.emplace_back([ptr = pool.get(), i](std::stop_token stoken) {
                tl_worker_index = i;
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
    m_workers.clear(); // Explicitly join all threads here before members are destroyed
}

void JThreadPool::worker_loop(std::stop_token stoken)
{
    std::size_t my_idx = tl_worker_index.value_or(0);
    
    while (!stoken.stop_requested()) {
        FixedTask<64> task;

        // 1. Try local queue
        {
            std::scoped_lock lock(m_local_queues[my_idx]->m_mutex);
            if (!m_local_queues[my_idx]->m_queue.empty()) {
                task = std::move(m_local_queues[my_idx]->m_queue.front());
                m_local_queues[my_idx]->m_queue.pop_front();
            }
        }

        // 2. Try global queue
        if (!task) {
            std::unique_lock lock(m_queue_mutex);
            if (!m_task_queue.empty()) {
                task = std::move(m_task_queue.front());
                m_task_queue.pop();
            }
        }

        // 3. Try stealing from others
        if (!task) {
            for (std::size_t i = 0; i < m_local_queues.size(); ++i) {
                std::size_t target_idx = (my_idx + i + 1) % m_local_queues.size();
                std::unique_lock lock(m_local_queues[target_idx]->m_mutex, std::try_to_lock);
                if (lock.owns_lock() && !m_local_queues[target_idx]->m_queue.empty()) {
                    task = std::move(m_local_queues[target_idx]->m_queue.back());
                    m_local_queues[target_idx]->m_queue.pop_back();
                    break;
                }
            }
        }

        // 4. If task found, execute and continue
        if (task) {
            task();
            continue;
        }

        // 5. If no task, wait
        std::unique_lock lock(m_queue_mutex);
        m_cv.wait(lock, stoken, [this, my_idx]() {
            if (m_is_shutdown) return true;
            if (!m_task_queue.empty()) return true;
            std::scoped_lock local_lock(m_local_queues[my_idx]->m_mutex);
            return !m_local_queues[my_idx]->m_queue.empty();
        });

        if (m_is_shutdown) {
            bool all_empty = m_task_queue.empty();
            for (auto& q : m_local_queues) {
                std::scoped_lock l(q->m_mutex);
                if (!q->m_queue.empty()) {
                    all_empty = false;
                    break;
                }
            }
            if (all_empty) {
                return;
            }
        }
    }
}

} // namespace core::concurrency

// RISK REVIEW:
// - Edge cases: OS fails to create threads -> Returns std::unexpected, caller must handle it.
// - Concurrency Risks: No deadlock inside pool logic. Worker wait logic is robust to spurious wakeups.
