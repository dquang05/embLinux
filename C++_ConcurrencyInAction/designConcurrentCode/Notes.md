# 1. Introduction

## Moving Beyond Basic Tools

Designing concurrent code is not merely about using basic synchronization primitives such as mutexes, condition variables, or individual thread-safe data structures. It requires a broader perspective to build large-scale architectures and perform useful work across the entire application.

## Design Considerations

In addition to traditional software design principles such as encapsulation, cohesion, and coupling, concurrent programming must address several additional challenges:

- Which data should be shared, and which data should remain independent?
- How should access to shared data be synchronized to prevent data races?
- Which threads must wait for other threads to complete intermediate steps?
- How many threads should be used to achieve optimal performance on the available hardware?

## Objective

Choosing the appropriate thread organization and structure has a decisive impact on both system performance and code clarity and maintainability.

# 2. Work-Sharing Techniques Between Threads

This section focuses on different approaches to dividing work among threads in order to fully utilize multicore processors.

Design is deployed in [quickSort.hpp](quickSort.hpp)

## Dividing Data Before Processing Begins

- Suitable for simple data-processing algorithms such as `std::for_each`.
- The data set is divided into fixed-size partitions before execution begins.
- Each thread works independently on its assigned portion of the data.
- Results are combined in a final reduction step.

## Dividing Data Recursively

- Applicable to algorithms where the structure of the data becomes apparent only during processing, such as Quicksort.
- Instead of creating a new thread for every recursive branch, which can lead to excessive context-switching overhead, the number of threads is limited.
- A shared work pool is used to distribute tasks dynamically, typically implemented with a Thread Pool and a Thread-Safe Stack.

## Dividing Work by Task Type

- Threads are assigned specialized responsibilities, following the principle of Separation of Concerns.
- This approach helps maintain responsive user interfaces and timely network processing.

### Pipeline Architecture

- Work can also be organized as a pipeline, where each thread is responsible for a specific stage in a sequence of processing steps.
- The output of one stage becomes the input of the next stage.
- This allows data to flow continuously through the system, producing results more smoothly and consistently.

# 3. Hardware Considerations for Multithreading

Even with perfect logical separation, concurrent code performance is heavily bound by CPU architecture and memory hierarchy. Understanding these hardware mechanics is crucial for designing optimal data structures.

## Number of Processors & Oversubscription

- **Hardware Limits:** A system can only execute as many parallel threads as it has physical/logical cores.
- **Oversubscription:** Having more *runnable* threads than available cores forces the OS to perform constant context switching. This overhead wastes CPU cycles and degrades performance.
- **Mitigation:** Use `std::thread::hardware_concurrency()` to scale thread pools dynamically, but be mindful of other CPU-intensive applications running on the host.

## Data Contention and Cache Ping-Pong

- **The Problem:** While concurrent *reads* are cheap, concurrent *writes* are expensive. When a thread modifies shared data, the updated cache line must propagate to other CPU cores, causing latency.
- **Cache Ping-Pong:** If multiple cores continuously modify the same memory location (e.g., a shared `std::atomic` counter in a loop), the cache line violently bounces between cores. This stalls the processors.
- **Mutex Overhead:** Locking/unlocking a mutex modifies its internal state. Thus, highly contended mutexes also trigger severe cache ping-pong.

## False Sharing

CPUs load memory in chunks called **Cache Lines** (typically 64 bytes), not individual bytes. 

**False Sharing** occurs when two *independent* variables happen to reside in the same cache line, and two different threads modify them. Although the threads don't share data, the hardware invalidates the entire cache line on every write, causing catastrophic cache ping-pong.

### Code Example: The Impact of Memory Layout

A practical benchmark demonstrating the performance impact of **False Sharing** and how proper memory organization improves cache efficiency.

**Source code:** [`falseSharing.cpp`](falseSharing.cpp)

| Approach | Memory Access Pattern | Impact |
| :--- | :--- | :--- |
| **❌ Bad (Interleaved)** | Threads access adjacent elements (e.g., T0: 0,4,8; T1: 1,5,9) | Severe **False Sharing**. Threads continuously invalidate each other's cache lines, causing Cache Ping-Pong and reducing performance. |
| **✅ Good (Contiguous Blocks)** | Each thread processes one contiguous memory region (e.g., T0: 0–249; T1: 250–499) | Excellent **Data Locality** with minimal False Sharing. Each thread primarily works on its own cache lines, maximizing throughput. |

### Example Benchmark Output

```text
Starting benchmark with 100000000 elements and 4 threads...
--------------------------------------------------------
[*] Bad Approach (False Sharing)      took: 39.6858 ms
[*] Good Approach (Contiguous Blocks) took: 17.1162 ms
--------------------------------------------------------
```

In this benchmark, the contiguous-block approach is approximately **2.3× faster** than the interleaved approach. The improvement comes from reducing False Sharing, minimizing cache invalidation between CPU cores, and improving cache locality.

## Data Proximity (Spatial Locality)

Data proximity affects both single-thread and multi-thread performance:
- **Cache Hits:** If a thread's working data is stored contiguously, the hardware prefetcher efficiently loads consecutive cache lines, drastically reducing memory latency.
- **Context-Switch Survival:** If the OS reschedules a thread to a different core, contiguous data ensures the new core can rebuild its cache with far fewer memory fetches compared to scattered data.


# 4. Designing Data Structures for Multithreaded Performance

Applying the hardware considerations discussed in Section 3—Data Contention, False Sharing, and Data Locality—directly influences how data structures should be designed and accessed. The objective is to organize memory so that data processed by the same thread remains close together, while data accessed by different threads is sufficiently separated to minimize cache interference.

## Array Division Strategy: Complex Operations (Matrix Multiplication)

When processing large arrays or matrices, the way work is divided determines the memory access pattern. Consider multiplying two **1000 × 1000** matrices (one million elements each) using **100 threads**.

Three common work-partitioning strategies are:

1. **Divide by Columns**
   - Each thread computes a set of adjacent columns.
   - Prevents False Sharing because threads write to different memory regions.
   - However, every thread must repeatedly read the entire first matrix, resulting in poor cache utilization.

2. **Divide by Rows**
   - Each thread computes a contiguous set of rows.
   - Threads write to continuous memory regions without interfering with one another.
   - This significantly improves data locality and cache efficiency.

3. **Divide into Rectangular Blocks (Recommended)**
   - Instead of assigning an entire row range, each thread processes a **100 × 100** block.
   - Although each thread still computes **10,000** output elements, it only needs to read **100 rows** of the first matrix and **100 columns** of the second matrix.

**Impact**

Compared with assigning full rows, this approach reduces the amount of data that must be loaded from approximately **1,010,000** elements to **200,000** elements per thread.

As a result:

- Significantly fewer memory accesses are required.
- Cache misses are greatly reduced.
- Cache reuse is improved.
- Overall throughput increases substantially.

---

## The Padding Technique (Avoiding False Sharing in Objects)

False Sharing is not limited to arrays. It can also occur when objects or structures place frequently modified variables within the same cache line.

### The Problem

Suppose a `std::mutex` is stored immediately before the data it protects.

```cpp
struct protected_data
{
    std::mutex m;
    my_data data_to_protect;
};
```

Locking or unlocking a mutex modifies the mutex's internal state. If the mutex and the protected data reside in the same cache line, every lock operation invalidates that cache line.

As a result, another CPU core accessing the protected data may be forced to reload the cache line, even though only the mutex was modified.

### The Solution

Separate frequently modified objects by inserting padding so they occupy different cache lines.

```cpp
struct protected_data
{
    std::mutex m;

    // Much larger than a typical cache line (64 bytes).
    // Separates the mutex from the protected data.
    char padding[65536];

    my_data data_to_protect;
};
```

The same idea can be applied to arrays of independent objects.

```cpp
struct my_data
{
    data_item1 d1;
    data_item2 d2;

    // Pushes the next object far enough away
    // to avoid sharing the same cache line.
    char padding[65536];
};

my_data some_array[256];
```

Each array element is now physically separated in memory, reducing the likelihood that different threads will modify data located within the same cache line.

> **Note:** Padding reduces False Sharing by trading memory usage for performance. It should only be applied after profiling confirms that False Sharing is a significant performance bottleneck.


# 5. Advanced Considerations for Concurrent Design

Beyond hardware optimizations and memory layout, production-ready concurrent code must address correctness, stability, and system-level architecture.

## Exception Safety in Parallel Algorithms

Exception safety in multithreading is critical. In a sequential program, an unhandled exception might just terminate the current operation. In a concurrent program, if a thread exits with an unhandled exception, `std::terminate` is called, crashing the entire application immediately. Furthermore, throwing exceptions without proper cleanup leads to leaked threads.

### The Solutions
There are two primary ways to handle exceptions safely across multiple threads:
1. **RAII (Resource Acquisition Is Initialization):** Creating a wrapper class (e.g., `join_threads`) that automatically calls `.join()` on all active threads in its destructor. This guarantees cleanup even if the main thread throws an exception.
2. **Using `std::async` (Recommended):** `std::async` inherently handles exception safety. If the asynchronous task throws, the exception is captured in the returned `std::future` and safely rethrown when `.get()` is called. Additionally, destroying a future returned by `std::async` automatically blocks until the thread completes, preventing dangling threads.

**View the code example:** [`exceptionSafety.cpp`](exceptionSafety.cpp)

## Scalability and Amdahl's Law

Scalability is the ability of a program to take advantage of additional processors to reduce execution time or increase throughput. The theoretical limit of this performance gain is described by **Amdahl's Law**:

$P = \frac{1}{f_s + \frac{1 - f_s}{N}}$

Where:
- $P$: Overall performance gain (speedup).
- $f_s$: The fraction of the program that must be executed serially (e.g., setting up threads, waiting for mutex locks).
- $N$: The number of available processors.

**Takeaway:** Even with an infinite number of processors, the maximum speedup is bottlenecked by the serial portion of your code. To maximize scalability, you must reduce $f_s$ by minimizing lock contention and keeping processors constantly fed with parallel work.

## Hiding Latency

Threads rarely run at 100% CPU capacity. They frequently block while waiting for disk I/O, network responses, or mutex locks. When a thread is blocked, its corresponding CPU core sits idle.

You can "hide" this latency and maximize hardware utilization by:
- **Strategic Oversubscription:** If tasks are heavily I/O bound (e.g., a virus scanner reading files from a disk), spawn more threads than physical CPU cores. While one thread waits for the disk, the OS can context-switch to another thread (e.g., the scanning engine) that is ready to process data.
- **Asynchronous Operations:** Use async I/O or lock-free data structures to allow the thread to perform other useful work instead of just waiting idly.

## Improving Responsiveness (Separation of Concerns)

A classic architectural mistake is performing heavy computations on the GUI (Graphical User Interface) thread or the main Event Loop. If a task takes too long, the event loop blocks, and the application "freezes" (becomes unresponsive to user input).

**The Solution:**
Apply Separation of Concerns. The GUI thread should only be responsible for capturing user input and rendering. Time-consuming tasks must be offloaded to dedicated background worker threads. The background thread communicates with the GUI thread via safe messaging (e.g., atomic flags), ensuring the application remains highly responsive and can even cancel ongoing tasks safely.

**View the architectural example:** [`guiResponsiveness.cpp`](guiResponsiveness.cpp)



# 6. Designing Concurrent Code in Practice

Applying concurrent design principles to real-world tasks requires careful consideration of data access patterns, exception safety, and synchronization overhead. To demonstrate these concepts, we analyze the parallelization of three standard C++ algorithms, each presenting a unique architectural challenge.

All code examples for this section are located in the [`practice/`](practice/) directory.

## Parallel `std::for_each`: Independent Execution

`std::for_each` applies a function to every element in a range. The parallel version differs fundamentally from the sequential version in that the order of execution is completely arbitrary. 

Since each element is processed independently, this algorithm is perfectly suited for **Recursive Data Division**. By recursively splitting the array in half and utilizing `std::async`, we allow the C++ Standard Library to automatically manage thread scaling (avoiding oversubscription) while inherently guaranteeing exception safety.

**View the implementation:** [`practice/parallelForEachAsync.hpp`](practice/parallelForEachAsync.hpp)

## Parallel `std::find`: Early Termination

Unlike `for_each`, algorithms like `std::find`, `std::any_of`, or `std::equal` possess a crucial property: **Early Termination**. If the desired element is found at the beginning of the sequence, the algorithm should stop immediately.

**The Design Challenge:**
In a parallel context, if Thread A finds the target, Threads B, C, and D must be interrupted. If we fail to interrupt the other threads, the parallel version might actually perform *slower* than the serial version, as the serial version returns the moment it finds a match.

**The Solution:**
We introduce a shared `std::atomic<bool> done` flag. Every thread frequently checks this flag during its execution loop. If a thread finds a match (or throws an exception), it sets `done = true`, signaling all other threads to abort their searches immediately.

**View the implementation:** [`practice/parallelFindAsync.hpp`](practice/parallelFindAsync.hpp)

## Parallel `std::partial_sum`: Handling Data Dependencies

`std::partial_sum` calculates a running total (e.g., `1, 2, 3` becomes `1, 3, 6`). This is notoriously difficult to parallelize because the result of element $N$ strictly depends on the result of element $N-1$. 

Depending on the hardware architecture, there are two distinct ways to solve this:

### Approach 1: Block-based Forward Propagation (For standard multi-core CPUs)
The array is divided into chunks. Each thread calculates the partial sum of its own chunk. Then, the final element of Chunk 1 is added to all elements in Chunk 2, and so forth. This works well when there are far more data elements than CPU cores.

### Approach 2: Incremental Pairwise Algorithm (For massively parallel/SIMD systems)
Elements are updated by adding values from an increasing stride (1, 2, 4, 8...). This requires threads to execute in strict **lockstep** to prevent race conditions (i.e., a thread running too fast and reading stale data before another thread has updated it).

To enforce lockstep execution, we implement a **Barrier**. A barrier forces threads to wait until exactly $N$ threads have arrived at the checkpoint. Once the "seats" are filled, the barrier opens, allowing all threads to proceed to the next iteration simultaneously.

**View the implementation:** [`practice/parallelPartialSum.hpp`](practice/parallelPartialSum.hpp)

# Chapter Summary

Designing concurrent code goes far beyond basic thread-safe data structures. It demands a holistic view of the system:
1. **Work Division:** Should we divide data upfront, recursively, or by task type (pipelines)?
2. **Hardware Realities:** Preventing False Sharing and optimizing Data Proximity to keep CPU caches efficient.
3. **Robustness:** Ensuring strict Exception Safety and preventing Thread Leaks.
4. **Scalability:** Minimizing lock contention to obey Amdahl's Law and utilizing lock-free synchronization (like Barriers) when necessary.

Repeatedly spawning and joining threads for every algorithm incurs significant OS overhead. To build high-performance, large-scale applications, we need a mechanism to reuse threads efficiently. This leads us directly to the concept of **Thread Pools**.