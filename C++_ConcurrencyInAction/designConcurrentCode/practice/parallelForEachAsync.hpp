#include <algorithm>
#include <future>
#include <iterator>

// A recursive, exception-safe parallel for_each using std::async
template<typename Iterator, typename Func>
void parallel_for_each(Iterator first, Iterator last, Func f) {
    unsigned long const length = std::distance(first, last);
    if (!length) return;

    unsigned long const min_per_thread = 25;

    // Base case: If the chunk is small enough, process it sequentially
    if (length < (2 * min_per_thread)) {
        std::for_each(first, last, f);
    } else {
        // Recursive case: Split the data in half
        Iterator const mid_point = first + length / 2;
        
        // Asynchronously process the first half
        std::future<void> first_half = 
            std::async(std::launch::async, parallel_for_each<Iterator, Func>, first, mid_point, f);
        
        // Synchronously process the second half in the current thread
        parallel_for_each(mid_point, last, f);
        
        // Wait for the async task to finish and propagate any exceptions
        first_half.get();
    }
}