#pragma once
#include <list>
#include <future>
#include <algorithm>
#include <utility>
#include <chrono>

// Assuming thread_pool and function_wrapper are defined elsewhere

template<typename T>
struct sorter
{
    thread_pool pool;

    std::list<T> do_sort(std::list<T>& chunk_data)
    {
        if (chunk_data.empty())
        {
            return chunk_data;
        }

        std::list<T> result;
        // Move the pivot element to the result list
        result.splice(result.begin(), chunk_data, chunk_data.begin());
        T const& partition_val = *result.begin();

        // Partition the remaining elements based on the pivot
        typename std::list<T>::iterator divide_point =
            std::partition(chunk_data.begin(), chunk_data.end(),
                           [&](T const& val) { return val < partition_val; });
        
        std::list<T> new_lower_chunk;
        // Move the lower partition into a separate list
        new_lower_chunk.splice(new_lower_chunk.end(),
                               chunk_data, chunk_data.begin(),
                               divide_point);

        // Submit the lower chunk asynchronously using a lambda instead of std::bind
        std::future<std::list<T>> new_lower =
            pool.submit([this, chunk = std::move(new_lower_chunk)]() mutable {
                return this->do_sort(chunk);
            });

        // Sort the higher chunk locally on the current thread
        std::list<T> new_higher(do_sort(chunk_data));
        result.splice(result.end(), new_higher);

        // Active waiting loop to prevent pool deadlock
        while (new_lower.waitFor(std::chrono::seconds(0)) == std::future_status::timeout)
        {
            pool.runPendingTask();
        }

        // Combine the sorted lower chunk back into the result
        result.splice(result.begin(), new_lower.get());
        return result;
    }
};

template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    sorter<T> s;
    return s.do_sort(input);
}