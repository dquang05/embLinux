# Development Plan - `modern_cpp_core`

This document serves as the architectural roadmap for the `modern_cpp_core` library. It inherits the complex concurrency problems from the old `C++_ConcurrencyInAction` directory but solves them using the paradigms and tools of **Modern C++20/23**.

---

## 1. Module `concurrency` (Synchronization & Threads)

**Goal:** Safely manage thread lifecycles and synchronization, completely replacing the cumbersome usage of `std::thread` and `std::condition_variable`.

**Legacy Reference:** `threads/`, `synchronizingConcurrency/`, `advancedThreadsManagement/`.

**Planned Features:**
- [x] **`JThreadPool` (Modern Thread Pool):** 
  - *Concept:* A pool of threads that reuses OS threads to reduce creation overhead. Must incorporate Thread-Local Work Queues or Work-Stealing mechanisms to prevent contention on a single global mutex queue.
  - *Upgrade:* Replaces the legacy `workStealingPool.hpp`. Utilizes `std::jthread` (auto-joining) and `std::stop_token` for safe cancellation without manual boolean flags.
- [x] **`Barrier / Latch Synchronization`:**
  - *Concept:* Synchronization primitives that allow multiple threads to wait at a synchronization point (Barrier) or count down events (Latch).
  - *Upgrade:* Directly use C++20 `std::barrier` and `std::latch` instead of writing custom wrappers over mutexes and condition variables. This acts as the modern, built-in replacement for one-off broadcast events previously handled by `std::condition_variable::notify_all()`.
- [x] **`Semaphore Wrapper`:** 
  - Wraps C++20 `std::counting_semaphore` to control access to limited resources (e.g., database connection limits).
- [ ] **`Low-Level Atomic Wait/Notify`:**
  - *Concept:* Utilize C++20 `std::atomic::wait`, `notify_one`, and `notify_all` as a lightweight lock-free alternative to `std::condition_variable` (based on futex).
  - *Design Note:* Must explicitly handle Spurious Wakeups by wrapping the `.wait()` call in a loop with a condition check (predicate), similarly to how it was done with `std::condition_variable`.
- [ ] **`Spinlock`:**
  - *Concept:* Ultra-fast lock using C++20 `std::atomic_flag::wait()` and `test()`. Eliminates system call overhead for extremely short critical sections (crucial for Embedded).
- [ ] **`Thread Affinity & Real-time Priority`:**
  - *Concept:* Expose `.native_handle()` in thread utilities to pin threads to specific CPU cores (`pthread_setaffinity_np`) and set POSIX real-time scheduling policies (`SCHED_FIFO`, `SCHED_RR`) to avoid cache ping-pong and ensure determinism.

## 2. Module `coroutines` (Asynchronous Programming)

**Goal:** Handle I/O, Network, and Timers without blocking OS threads.

**Legacy Reference:** `taskBased.cpp`, `waitingFuture.md`.

**Planned Features:**
- [x] **`Task<T>` and `SharedTask<T>`:**
  - *Concept:* A return type for a Coroutine representing future work. Lazy evaluation - execution only begins when `co_await` is called.
  - *Upgrade:* Replaces `std::future` (which typically blocks the thread via `.wait()`). Since C++20 only provides the core language features for coroutines, custom Promise Types must be implemented. Tapping into the internal `promise_type` of C++20 Coroutines completely replaces the need for `std::promise` and `std::packaged_task`, minimizing dynamic allocation overhead. Also includes `SharedTask<T>` to replace `std::shared_future` (allowing multiple coroutines to `co_await` the same one-off result).
- [x] **`Generator<T>`:**
  - *Concept:* Generates an infinite or massive stream of data (e.g., reading log files line by line, sampling sensors) using `co_yield` without allocating the entire array in memory.
- [ ] **`Channels (CSP Model)`:**
  - *Concept:* Implement Message Passing based on Coroutines (similar to Go channels) to allow threads to communicate without shared memory locks. This serves as the foundation for building the **Actor Model**, allowing threads to operate as isolated State Machines without Data Races.
- [ ] **`Async Coroutine Timer / Timeout`:**
  - *Concept:* Replaces `std::condition_variable::wait_for`. Utilizes C++20 Coroutines and `std::chrono` for non-blocking asynchronous waits (e.g., `co_await Timeout(100ms)`) to save thread resources during hardware/network delays.

## 3. Module `data_structures` (Thread-Safe Data Structures)

**Goal:** Provide ultra-fast containers for multi-threading, avoiding Data Races.

**Legacy Reference:** `designLockBase/`, `designLockFree/`.

**Planned Features:**
- [x] **`LockFreeStack` (Treiber Stack):**
  - *Upgrade:* Rewrite `treiberStack.cpp`. Achieve truly lock-free performance on embedded (ARM) systems by replacing `std::atomic<std::shared_ptr>` with Hazard Pointers to solve the ABA problem and Memory Reclamation issues.
- [x] **`ThreadSafeQueue`:**
  - *Concept:* A thread-safe queue leveraging C++20 Concepts to ensure elements are Movable. Must incorporate two forms of retrieval to prevent inherent interface-level race conditions:
    1. **Non-blocking (`try_pop`)**: Combines `front()` and `pop()` returning `std::optional<T>`.
    2. **Blocking (`wait_and_pop`)**: Puts the thread to sleep when the queue is empty, awakening only when new data is pushed. Implemented using C++20 `std::counting_semaphore` or `std::atomic::wait/notify` rather than traditional `std::condition_variable` to reduce overhead.
- [x] **`WaitFreeQueue / LockFreeQueue`:**
  - *Concept:* A high-performance concurrent queue. Emphasizes **Wait-Free** guarantees for SPSC (Single-Producer Single-Consumer) Ring Buffers, ensuring O(1) deterministic execution time without retry loops (crucial for Real-time Embedded ISR communication). Explicitly utilizes `std::memory_order_acquire` and `release` to avoid the overhead of `seq_cst`.
  - *Upgrade:* Uses an **Array-based Ring Buffer** instead of a Linked-List to completely eliminate dynamic memory allocation (`new`/`delete`) at runtime. This permanently avoids the **ABA Problem** and complex memory reclamation overheads (Hazard Pointers/Reference Counting), making it ideal for Real-time ISRs.
- [x] **`ThreadSafeHashMap`:**
  - *Concept:* A concurrent hash map utilizing fine-grained locks or `std::shared_mutex` at the bucket level.
  - *Upgrade:* Provides two internal bucket strategies for different use cases:
    1. **Linked List (Hand-over-hand locking):** Useful for scenarios requiring iterator support or where element removals are frequent, utilizing `std::mutex` per node.
    2. **Flat Array / Open Addressing:** Optimized for read-heavy workloads on Embedded systems, maximizing CPU Cache Locality and avoiding the overhead of thousands of node-level mutexes.
- [ ] **`RCU (Read-Copy-Update)`:**
  - *Concept:* A synchronization mechanism optimized for read-heavy workloads (e.g., configuration data, routing tables in embedded Linux), avoiding read locks entirely. Prepares for the C++26 `std::rcu` proposal.

## 4. Module `memory` (Memory Management & Cache Optimization)

**Goal:** Optimize memory allocation for Embedded systems, preventing fragmentation and False Sharing.

**Legacy Reference:** `memory_atomicOperation/`, `falseSharing.cpp`.

**Planned Features:**
- [ ] **`AlignedCacheLine`:**
  - *Concept:* False Sharing occurs when two independent variables reside on the same CPU Cache Line, causing threads to step on each other's toes (cache ping-pong).
  - *Upgrade:* Nên sử dụng `alignas(std::hardware_destructive_interference_size)` (C++17) cho các biến độc lập thường xuyên bị sửa đổi (như Atomics, Head/Tail index) để tự động căn lề và ngăn chặn hoàn toàn False Sharing mà không cần hack bằng mảng `char` như thời C++11.
- [ ] **`std::atomic_ref`:**
  - *Concept:* Use C++20 `std::atomic_ref` to perform atomic operations on non-atomic objects, saving padding memory for structures on embedded devices.
- [ ] **`MonotonicMemoryPool`:**
  - Use C++17/20 `std::pmr::monotonic_buffer_resource` to create ultra-fast allocation zones that don't require individual element deallocation (ideal for short-lived request processing loops).
- [ ] **`Zero-Cost Thread-safe Initialization (constinit)`:**
  - *Concept:* Replaces `std::call_once` and `std::once_flag` for Singletons or static variables. Uses C++20 `constinit` to ensure compile-time initialization, eliminating the "Static Initialization Order Fiasco" without runtime locking overhead.
- [ ] **`Atomic Floating-Point (C++20)`:**
  - *Concept:* Utilize C++20 `std::atomic<float>` and `std::atomic<double>` which now natively support `fetch_add` and `fetch_sub`. Crucial for lock-free aggregation of sensor data (ADC, PID outputs, Signal Filters) from multiple threads or ISRs.

## 5. Module `patterns` & `utils`

**Goal:** General utilities and algorithms for parallel data processing pipelines.

**Legacy Reference:** `practice/parallel*` algorithms, `exceptionSafety.cpp`.

**Planned Features:**
- [ ] **Library Concepts (`core_concepts.hpp`):**
  - Define type constraints: `template <typename T> requires std::is_integral_v<T>` becomes `template <std::integral T>`.
- [ ] **Data Pipeline (Ranges):**
  - Use C++20 `std::ranges` to process data arrays without generating temporary variables (e.g., `data | views::filter(...) | views::transform(...)`).
- [ ] **Parallel Algorithms:**
  - *Concept:* Integrate parallel algorithms (like `parallel_for_each`, `parallel_quick_sort`) that dispatch tasks directly to `JThreadPool`, replacing the legacy `parallelFindAsync.hpp` and `quickSort.hpp` and avoiding the heavy overhead of default `std::execution::par`.
- [ ] **Error Handling:** 
  - Encourage the use of C++23 `std::expected<T, Error>` for error handling instead of `throw/catch` exceptions (as exceptions incur high overhead in Embedded systems). Within concurrent tasks, `Task<std::expected<T, Error>>` will be used to safely propagate errors across threads, completely replacing the heavy `std::promise::set_exception()` and `future::get()` throw mechanisms of C++11.
- [ ] **`Relaxed Counters (Telemetry/Metrics)`:**
  - *Concept:* Strictly enforce the use of `std::memory_order_relaxed` for all atomic counters used in telemetry, performance profiling, or frame drop counts. This explicitly eliminates cache-synchronization overhead across cores for data that does not require sequential consistency.

## 6. Module `testing` (Concurrency Testing)

**Goal:** Ensure thread safety and prove the absence of Data Races and Deadlocks in lock-free structures.

**Planned Features:**
- [ ] **ThreadSanitizer (TSan):** Integrate TSan into CMake for testing and CI to detect data races at runtime.
- [ ] **Stress Testing:** Write tests simulating heavy contention to intentionally provoke the ABA problem in `LockFreeStack` and verify the logic of `LockFreeQueue`.
- [ ] **Concurrent GO Signal (C++20 `std::latch`):** When writing Unit Tests for concurrent containers, use `std::latch` to ensure all testing threads start their critical sections at exactly the same time. This avoids thread-creation latency skewing the test results and maximizes the chance of reproducing race conditions, replacing the cumbersome `std::promise`/`std::shared_future` trick used in C++11.
