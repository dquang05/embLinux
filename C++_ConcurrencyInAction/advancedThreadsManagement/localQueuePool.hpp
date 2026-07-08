#pragma once

#include <future>
#include <thread>
#include <utility>

template <typename Function>
auto submit(Function&& function)
    -> std::future<std::invoke_result_t<Function>>
{
    using result_type = std::invoke_result_t<Function>;

    std::packaged_task<result_type()> task(std::forward<Function>(function));
    auto future = task.get_future();

    if (local_work_queue)
    {
        local_work_queue->push(std::move(task));
    }
    else
    {
        pool_work_queue.push(std::move(task));
    }

    return future;
}

void run_pending_task()
{
    function_wrapper task;

    if (local_work_queue && !local_work_queue->empty())
    {
        task = std::move(local_work_queue->front());
        local_work_queue->pop();
        task();
    }
    else if (pool_work_queue.try_pop(task))
    {
        task();
    }
    else
    {
        std::this_thread::yield();
    }
}