#include <iostream>
#include <vector>
#include <numeric>
#include <future>
#include <stdexcept>

// A robust, exception-safe parallel accumulate using std::async
template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init) {
    unsigned long const length = std::distance(first, last);
    unsigned long const max_chunk_size = 25;

    if (length <= max_chunk_size) {
        // Simulate a deliberate exception to test safety
        if (length > 0 && *first == -1) {
            throw std::runtime_error("Simulated error in worker thread!");
        }
        return std::accumulate(first, last, init);
    } else {
        Iterator mid_point = first;
        std::advance(mid_point, length / 2);

        // std::async handles the thread lifecycle safely.
        // If an exception occurs, it is captured in the future.
        std::future<T> first_half_result = 
            std::async(std::launch::async, parallel_accumulate<Iterator, T>, first, mid_point, init);

        T second_half_result = parallel_accumulate(mid_point, last, T());

        // .get() will rethrow any exception caught in the async task
        return first_half_result.get() + second_half_result;
    }
}

int main() {
    std::vector<int> data(100, 1);
    data[15] = -1; // This trigger will cause an exception

    try {
        std::cout << "Starting exception-safe parallel algorithm...\n";
        int result = parallel_accumulate(data.begin(), data.end(), 0);
        std::cout << "Result: " << result << "\n";
    } catch (const std::exception& e) {
        std::cout << "[CAUGHT EXCEPTION SAFELY]: " << e.what() << "\n";
        std::cout << "The program did not crash via std::terminate!\n";
    }

    return 0;
}