#include <atomic>
#include <thread>
#include <vector>

// A synchronization primitive enforcing lockstep execution
class barrier {
    unsigned const count;
    std::atomic<unsigned> spaces;
    std::atomic<unsigned> generation;

public:
    explicit barrier(unsigned count_) : count(count_), spaces(count_), generation(0) {}

    void wait() {
        unsigned const my_generation = generation.load();
        
        if (!--spaces) {
            // Last thread to arrive resets the barrier and increments the generation
            spaces = count;
            ++generation;
        } else {
            // Other threads spin-wait until the generation changes
            while (generation.load() == my_generation) {
                std::this_thread::yield();
            }
        }
    }
};

// SIMD-style pairwise partial sum using a barrier
template<typename Iterator>
void parallel_partial_sum(Iterator first, Iterator last) {
    typedef typename Iterator::value_type value_type;

    struct process_element {
        void operator()(Iterator first, Iterator last, std::vector<value_type>& buffer, unsigned i, barrier& b) {
            value_type& ith_element = *(first + i);
            bool update_source = false;

            // Incrementally add values from increasing strides (1, 2, 4, 8...)
            for (unsigned step = 0, stride = 1; stride <= i; ++step, stride *= 2) {
                value_type const& source = (step % 2) ? buffer[i] : ith_element;
                value_type& dest = (step % 2) ? ith_element : buffer[i];
                value_type const& addend = (step % 2) ? buffer[i - stride] : *(first + i - stride);

                dest = source + addend;
                update_source = !(step % 2);
                
                // Ensure all threads complete this step before moving to the next stride
                b.wait();
            }

            if (update_source) {
                ith_element = buffer[i];
            }
        }
    };

    unsigned long const length = std::distance(first, last);
    if (length <= 1) return;

    std::vector<value_type> buffer(length);
    barrier b(length); // Create a barrier for N threads
    std::vector<std::thread> threads(length - 1);

    // Spawn N-1 threads
    for (unsigned long i = 0; i < (length - 1); ++i) {
        threads[i] = std::thread(process_element(), first, last, std::ref(buffer), i, std::ref(b));
    }
    
    // The main thread acts as the final worker
    process_element()(first, last, buffer, length - 1, b);

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
}