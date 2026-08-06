# Development Plan - `modern_cpp_core`

This document serves as the architectural roadmap for the `modern_cpp_core` library. It inherits the complex concurrency problems from the old `C++_ConcurrencyInAction` directory but solves them using the paradigms and tools of **Modern C++20/23**.

---

## 1. Module `concurrency` (Synchronization & Threads)

**Goal:** Safely manage thread lifecycles and synchronization, completely replacing the cumbersome usage of `std::thread` and `std::condition_variable`.

**Legacy Reference:** `threads/`, `synchronizingConcurrency/`, `advancedThreadsManagement/`.

**Planned Features:**
- [x] **`JThreadPool` (Modern Thread Pool):** 
  - *Concept:* A pool of threads that reuses OS threads to reduce creation overhead.
  - *Upgrade:* Replaces the legacy `workStealingPool.hpp`. Utilizes `std::jthread` (auto-joining) and `std::stop_token` for safe cancellation without manual boolean flags.
- [x] **`Barrier / Latch Synchronization`:**
  - *Concept:* Synchronization primitives that allow multiple threads to wait at a synchronization point (Barrier) or count down events (Latch).
  - *Upgrade:* Directly use C++20 `std::barrier` and `std::latch` instead of writing custom wrappers over mutexes and condition variables.
- [x] **`Semaphore Wrapper`:** 
  - Wraps C++20 `std::counting_semaphore` to control access to limited resources (e.g., database connection limits).

## 2. Module `coroutines` (Asynchronous Programming)

**Goal:** Handle I/O, Network, and Timers without blocking OS threads.

**Legacy Reference:** `taskBased.cpp`, `waitingFuture.md`.

**Planned Features:**
- [ ] **`Task<T>`:**
  - *Concept:* A return type for a Coroutine representing future work. Lazy evaluation - execution only begins when `co_await` is called.
  - *Upgrade:* Replaces `std::future` (which typically blocks the thread via `.wait()`).
- [ ] **`Generator<T>`:**
  - *Concept:* Generates an infinite or massive stream of data (e.g., reading log files line by line, sampling sensors) using `co_yield` without allocating the entire array in memory.

## 3. Module `data_structures` (Thread-Safe Data Structures)

**Goal:** Provide ultra-fast containers for multi-threading, avoiding Data Races.

**Legacy Reference:** `designLockBase/`, `designLockFree/`.

**Planned Features:**
- [ ] **`LockFreeStack` (Treiber Stack):**
  - *Upgrade:* Rewrite `treiberStack.cpp`. Use C++20 `std::atomic<std::shared_ptr>` to fundamentally solve the ABA problem and Memory Reclamation issues without writing complex Hazard Pointers.
- [ ] **`ThreadSafeQueue`:**
  - A thread-safe queue utilizing `std::mutex` safely wrapped in `std::scoped_lock`, leveraging C++20 Concepts to ensure elements are Movable.

## 4. Module `memory` (Memory Management & Cache Optimization)

**Goal:** Optimize memory allocation for Embedded systems, preventing fragmentation and False Sharing.

**Legacy Reference:** `memory_atomicOperation/`, `falseSharing.cpp`.

**Planned Features:**
- [ ] **`AlignedCacheLine`:**
  - *Concept:* False Sharing occurs when two independent variables reside on the same CPU Cache Line, causing threads to step on each other's toes.
  - *Upgrade:* Use C++17/20 `std::hardware_constructive_interference_size` to automatically pad atomic variables.
- [ ] **`MonotonicMemoryPool`:**
  - Use C++17/20 `std::pmr::monotonic_buffer_resource` to create ultra-fast allocation zones that don't require individual element deallocation (ideal for short-lived request processing loops).

## 5. Module `patterns` & `utils`

**Goal:** General utilities and algorithms for parallel data processing pipelines.

**Legacy Reference:** `practice/parallel*` algorithms, `exceptionSafety.cpp`.

**Planned Features:**
- [ ] **Library Concepts (`core_concepts.hpp`):**
  - Define type constraints: `template <typename T> requires std::is_integral_v<T>` becomes `template <std::integral T>`.
- [ ] **Data Pipeline (Ranges):**
  - Use C++20 `std::ranges` to process data arrays without generating temporary variables (e.g., `data | views::filter(...) | views::transform(...)`).
- [ ] **Error Handling:** 
  - Encourage the use of C++23 `std::expected<T, Error>` for error handling instead of `throw/catch` exceptions (as exceptions incur high overhead in Embedded systems).
