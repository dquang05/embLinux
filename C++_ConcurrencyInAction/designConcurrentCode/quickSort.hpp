#include <list>
#include <vector>
#include <thread>
#include <future>
#include <atomic>
#include <stack>
#include <mutex>
#include <memory>
#include <algorithm>
#include <chrono>

// 1. DEFINITION OF THREAD-SAFE STACK
template<typename T>
class thread_safe_stack 
{
private:
    std::stack<T> data;
    mutable std::mutex m; // mutable allows const member functions to lock the mutex for thread safety

public:
    thread_safe_stack() {}

    thread_safe_stack(const thread_safe_stack& other) // Copy constructor for thread_safe_stack
    {
        std::lock_guard<std::mutex> lock(other.m);
        data = other.data;
    }

    thread_safe_stack& operator=(const thread_safe_stack&) = delete; // Block copy assignment to prevent accidental copying

    // Safely push a new item onto the stack
    void push(T new_value) 
    {
        std::lock_guard<std::mutex> lock(m);
        data.push(std::move(new_value));
    }

    // Safely pop an item. Returns an empty shared_ptr if the stack is empty
    std::shared_ptr<T> pop() 
    {
        std::lock_guard<std::mutex> lock(m);
        if (data.empty()) return std::shared_ptr<T>(); // Check empty before popping
        
        std::shared_ptr<T> const res(std::make_shared<T>(
            std::move(data.top())
            )
        );
        data.pop();
        return res;
    }

    // Check if stack is empty in a thread-safe manner
    bool empty() const 
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};

// 2. DEFINITION OF PARALLEL SORTER
template<typename T>
struct sorter 
{
    // Represents a sub-list (chunk) that needs to be sorted
    struct chunk_to_sort
    {
        std::list<T> data;
        std::promise<std::list<T>> promise; // Holds the future sorted result
    };

    thread_safe_stack<chunk_to_sort> chunks; // Global storage for pending chunks
    std::vector<std::thread> threads;        // Pool of worker threads
    unsigned const max_thread_count;         // Maximum threads allowed by hardware
    std::atomic<bool> end_of_data;           // Signal to stop all worker threads

    sorter(): // Member initializer list
        max_thread_count(std::thread::hardware_concurrency() - 1), // Save one core for main thread
        end_of_data(false)
    {}

    // Destructor coordinates the safe shutdown of all spawned threads
    ~sorter() 
    {
        end_of_data = true; 
        for(unsigned i = 0; i < threads.size(); ++i)
        {
            if(threads[i].joinable())
            {
                threads[i].join(); 
            }
        }
    }

    // Worker threads call this to try and fetch work from the shared stack
    void try_sort_chunk()
    {
        std::shared_ptr<chunk_to_sort> chunk = chunks.pop();  // chunk: queue of chunks to sort
        if(chunk)
        {
            sort_chunk(chunk); 
        }
    }

    // Core recursive Quicksort function
    std::list<T> do_sort(std::list<T>& chunk_data) 
    {
        if(chunk_data.empty())
        {
            return chunk_data;
        }

        std::list<T> result;
        // Step 1: Pick the first element as the Pivot
        result.splice(result.begin(), chunk_data, chunk_data.begin());
        T const& partition_val = *result.begin();

        // Step 2: Rearrange elements (smaller than pivot go left, larger go right)
        typename std::list<T>::iterator divide_point = 
            std::partition(chunk_data.begin(), chunk_data.end(),
                           [&](T const& val){ return val < partition_val; });

        // Step 3: Extract the lower half into a separate chunk object
        chunk_to_sort new_lower_chunk;
        new_lower_chunk.data.splice(new_lower_chunk.data.end(),
                                     chunk_data, chunk_data.begin(),
                                     divide_point);

        // Get the future channel to receive the sorted lower half later
        std::future<std::list<T>> new_lower = new_lower_chunk.promise.get_future();
        
        // Push the lower chunk onto the stack so other threads can help
        chunks.push(std::move(new_lower_chunk)); 

        // Step 4: If we have free CPU cores, spawn a new worker thread
        if(threads.size() < max_thread_count) 
        {
            threads.push_back(std::thread(&sorter<T>::sort_thread, this));
        }

        // Step 5: Sort the higher half synchronously on the current thread
        std::list<T> new_higher(do_sort(chunk_data));
        result.splice(result.end(), new_higher);

        // Step 6: While waiting for the lower half to finish, help process other chunks
        while(new_lower.wait_for(std::chrono::seconds(0)) != std::future_status::ready) 
        {
            try_sort_chunk(); 
        }

        // Step 7: Combine everything back together
        result.splice(result.begin(), new_lower.get());
        return result;
    }

    // Helper to fulfill the promise after sorting a popped chunk
    void sort_chunk(std::shared_ptr<chunk_to_sort> const& chunk)
    {
        chunk->promise.set_value(do_sort(chunk->data)); 
    }

    // Main loop for worker threads
    void sort_thread()
    {
        while(!end_of_data) 
        {
            try_sort_chunk(); 
            std::this_thread::yield(); // Cooperatively give up CPU time slice
        }
    }
};

// 3. MAIN PUBLIC INTERFACE
template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input) 
{
    if(input.empty())
    {
        return input;
    }
    sorter<T> s;
    return s.do_sort(input); 
}