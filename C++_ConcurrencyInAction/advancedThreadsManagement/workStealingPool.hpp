#pragma once

#include <deque>
#include <mutex>
#include <vector>
#include <thread>
#include <future>
#include <atomic>
#include <memory>
#include <type_traits>

// ============================================================================
// CONCEPT 1: The Lock-Based Work-Stealing Queue
// ============================================================================
class work_stealing_queue
{
private:
    typedef function_wrapper data_type;
    std::deque<data_type> the_queue;     
    mutable std::mutex the_mutex;

public:
    work_stealing_queue() {}
    
    // Disallow copying to protect container invariants
    work_stealing_queue(const work_stealing_queue& other) = delete;
    work_stealing_queue& operator=(const work_stealing_queue& other) = delete;

    // Owner thread pushes to the front (LIFO structure)
    void push(data_type data)     
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        the_queue.push_front(std::move(data));
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        return the_queue.empty();
    }

    // Owner thread pops from the front (LIFO optimization for cache warmth)
    bool try_pop(data_type& res)     
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        if(the_queue.empty())
        {
            return false;
        }
        res = std::move(the_queue.front());
        the_queue.pop_front();
        return true;
    }

    // Foreign threads steal from the back to minimize lock contention with the owner
    bool try_steal(data_type& res)     
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        if(the_queue.empty())
        {
            return false;
        }
        res = std::move(the_queue.back());
        the_queue.pop_back();
        return true;
    }
};

// ============================================================================
// CONCEPT 2: Thread Pool Coordinating Work Stealing
// ============================================================================
class thread_pool
{
private:
    typedef function_wrapper task_type;
    
    std::atomic_bool done;
    thread_safe_queue pool_work_queue; // Global queue fallback
    std::vector<std::unique_ptr<work_stealing_queue>> queues;    
    std::vector<std::thread> threads;
    join_threads joiner;

    // Thread-local pointers to establish identities
    static thread_local work_stealing_queue* local_work_queue;    
    static thread_local unsigned my_index;

    void worker_thread(unsigned my_index_)
    {
        my_index = my_index_;
        local_work_queue = queues[my_index].get();    
        while(!done)
        {
            run_pending_task();
        }
    }

    bool pop_task_from_local_queue(task_type& task)
    {
        return local_work_queue && local_work_queue->try_pop(task);
    }

    bool pop_task_from_pool_queue(task_type& task)
    {
        return pool_work_queue.try_pop(task);
    }

    // Iterates through other threads' queues using an offset to avoid herd contention
    bool pop_task_from_other_thread_queue(task_type& task)    
    {
        for(unsigned i = 0; i < queues.size(); ++i)
        {
            unsigned const index = (my_index + i + 1) % queues.size();
            if(queues[index]->try_steal(task))
            {
                return true;
            }
        }
        return false;
    }

public:
    thread_pool() : done(false), joiner(threads)
    {
        unsigned const thread_count = std::thread::hardware_concurrency();
        try
        {
            for(unsigned i = 0; i < thread_count; ++i)
            {
                queues.push_back(std::unique_ptr<work_stealing_queue>(new work_stealing_queue));
                threads.push_back(std::thread(&thread_pool::worker_thread, this, i));
            }
        }
        catch(...)
        {
            done = true;
            throw;
        }
    }

    ~thread_pool()
    {
        done = true;
    }

    template<typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f)
    {
        typedef typename std::result_of<FunctionType()>::type result_type;
        
        std::packaged_task<result_type()> task(f);
        std::future<result_type> res(task.get_future());
        
        if(local_work_queue)
        {
            local_work_queue->push(std::move(task));
        }
        else
        {
            pool_work_queue.push(std::move(task));
        }
        return res;
    }

    // Three-stage fallback pipeline: Local -> Global -> Steal
    void run_pending_task()
    {
        task_type task; 
        if(pop_task_from_local_queue(task) ||   
           pop_task_from_pool_queue(task) ||       
           pop_task_from_other_thread_queue(task))       
        {
            task();
        }
        else
        {
            std::this_thread::yield();
        }
    }
};

// Define static thread_local members (Must be initialized in a cpp file or declared inline in C++17)
inline thread_local work_stealing_queue* thread_pool::local_work_queue = nullptr;
inline thread_local unsigned thread_pool::my_index = 0;

