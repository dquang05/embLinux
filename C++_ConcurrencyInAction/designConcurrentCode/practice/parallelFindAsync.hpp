#include <atomic>
#include <future>
#include <iterator>

// Internal implementation carrying the shared 'done' flag
template<typename Iterator, typename MatchType>
Iterator parallel_find_impl(Iterator first, Iterator last, MatchType match, std::atomic<bool>& done) {
    try {
        unsigned long const length = std::distance(first, last);
        unsigned long const min_per_thread = 25;

        // Base case: Sequential search with Early Termination check
        if (length < (2 * min_per_thread)) {
            for (; (first != last) && !done.load(); ++first) {
                if (*first == match) {
                    done = true; // Signal other threads to stop
                    return first;
                }
            }
            return last;
        } else {
            // Recursive split
            Iterator const mid_point = first + (length / 2);
            
            std::future<Iterator> async_result = 
                std::async(std::launch::async, parallel_find_impl<Iterator, MatchType>, mid_point, last, match, std::ref(done));
            
            Iterator const direct_result = parallel_find_impl(first, mid_point, match, done);
            
            // If the direct result didn't find it, check the async result
            return (direct_result == mid_point) ? async_result.get() : direct_result;
        }
    } catch (...) {
        done = true; // Ensure all threads stop on exception
        throw;
    }
}

// Public API wrapper
template<typename Iterator, typename MatchType>
Iterator parallel_find(Iterator first, Iterator last, MatchType match) {
    std::atomic<bool> done(false);
    return parallel_find_impl(first, last, match, done);
}