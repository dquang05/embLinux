#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>

// Define a large array size to make the performance difference obvious.
const size_t ARRAY_SIZE = 100000000;
const int NUM_THREADS = 4;

// Bad approach: Interleaved access causing False Sharing
void bad_concurrent_processing(std::vector<int>& data) {
    auto worker = [&](int thread_id) {
        // Each thread accesses elements with a stride equal to NUM_THREADS.
        // This forces multiple threads on different cores to write to the 
        // same cache line continuously, triggering severe cache ping-pong.
        for (size_t i = thread_id; i < data.size(); i += NUM_THREADS) {
            data[i]++;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker, i);
    }
    for (auto& t : threads) {
        t.join();
    }
}

// Good approach: Contiguous access for better Data Proximity
void good_concurrent_processing(std::vector<int>& data) {
    size_t chunk_size = data.size() / NUM_THREADS;

    auto worker = [&](int thread_id) {
        size_t start = thread_id * chunk_size;
        size_t end = (thread_id == NUM_THREADS - 1) ? data.size() : (start + chunk_size);
        
        // Each thread works on a contiguous range of memory.
        // Threads own their respective cache lines exclusively, avoiding 
        // cross-core invalidation and maximizing hardware prefetcher efficiency.
        for (size_t i = start; i < end; ++i) {
            data[i]++;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker, i);
    }
    for (auto& t : threads) {
        t.join();
    }
}

// Helper function to measure and print execution time
template <typename Func>
void measure_time(const std::string& name, Func process_func) {
    // Initialize array with zeros
    std::vector<int> data(ARRAY_SIZE, 0);

    // Start timer
    auto start = std::chrono::high_resolution_clock::now();
    
    // Execute the concurrent function
    process_func(data);
    
    // Stop timer
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "[*] " << name << " took: " << duration.count() << " ms\n";
}

int main() {
    std::cout << "Starting benchmark with " << ARRAY_SIZE << " elements and " 
              << NUM_THREADS << " threads...\n";
    std::cout << "--------------------------------------------------------\n";

    // Run the bad approach (False Sharing)
    measure_time("Bad Approach (False Sharing)     ", bad_concurrent_processing);

    // Run the good approach (Data Proximity)
    measure_time("Good Approach (Contiguous Blocks)", good_concurrent_processing);

    std::cout << "--------------------------------------------------------\n";
    return 0;
}