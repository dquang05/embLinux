> **Note:** This document focuses on advanced thread management architectures (Thread Pools, Work Stealing, Task Dependencies). Fundamental C++ Concurrency concepts (`thread` lifecycle, `std::thread`, `std::mutex`, `std::lock_guard`, etc.) are covered in detail in the [`../threads/`](../threads/) directory.

# 1. Task Queue Supporting `std::future` and Move-only Callables

## Problem

A basic Thread Pool typically stores tasks as `std::function<void()>`. To return a `std::future`, each submitted task must first be wrapped in a `std::packaged_task`.

The problem is that `std::packaged_task` is Move-only, while `std::function` requires its stored callable to be Copy-constructible. As a result, storing a `std::packaged_task` directly in a `std::queue<std::function<void()>>` causes a compilation error.

## Solution

Use a custom type-erasure wrapper with the following requirements:

- Wrap any callable with the signature `void()`. The actual return value is managed by `std::packaged_task` and delivered through the associated `std::future`.
- Support Move semantics while explicitly disabling Copy semantics.

Wrapper implementation: [functionWrapper](functionWrapper.hpp)

## Limitation: Task Dependency Deadlock

This design works well for independent tasks but fails when tasks depend on other tasks in the same Thread Pool.

For example, a task may submit child tasks (such as in recursive Quicksort) and immediately wait for their `std::future`. The waiting worker thread becomes blocked. If all worker threads block in the same way while their child tasks remain in the queue, no thread is available to execute those tasks, resulting in a deadlock.

# 2. Deadlock-Free Pool: Handling Task Dependencies (Active Waiting)

## Problem

Blocking on `std::future::get()` or `std::future::wait()` assumes another worker thread will execute the pending task. In a limited-size Thread Pool, this assumption can fail. If all worker threads are blocked waiting for child tasks, the queue stops making progress and the pool deadlocks.

## Solution

Replace passive waiting with Active Waiting.

Instead of blocking, a waiting worker thread temporarily becomes an executor. While waiting for a specific `std::future`, it repeatedly retrieves and executes pending tasks from the shared queue.

This keeps the queue progressing and ensures that the required child task is eventually executed.

## Concept and Implementation

The task execution logic is extracted into a separate function, `run_pending_task()`.

```cpp
void thread_pool::run_pending_task()
{
    function_wrapper task;

    // Try to retrieve a task without blocking.
    if(work_queue.try_pop(task))
    {
        // Execute the task immediately on the current thread.
        task();
    }
    else
    {
        // No task is available. Yield the CPU to avoid busy waiting.
        std::this_thread::yield();
    }
}
```

`run_pending_task()` performs the following steps:

1. Attempt to retrieve a task using the non-blocking `try_pop()`.
2. If a task is available, execute it immediately on the current thread.
3. Otherwise, call `std::this_thread::yield()` to avoid consuming CPU while waiting.

Wrapper implementation: [deadlockFreePool](deadlockFreePool.hpp)

## Limitation (Queue Contention and Cache Ping-Pong)

While active waiting successfully prevents deadlocks, it introduces a significant performance bottleneck. Because both the idle worker threads and the threads stuck in the active waiting `while` loop are continuously calling `try_pop()` on the *single, shared* `work_queue`, it leads to extreme **Queue Contention**.

This constant locking/unlocking (or heavy atomic operations in a lock-free queue) across multiple cores causes **Cache Ping-Pong**—a phenomenon where the memory cache line containing the queue's state is continuously invalidated and transferred between different CPU caches. This drastically degrades memory bandwidth and ruins scalability as the number of threads increases. 

To achieve true high performance, we must reduce this contention, which sets the stage for the next architectural improvements: **Thread-local Work Queues** and **Work Stealing**.


# 3. Local Work Queues: Reducing Contention

## Problem

Both the Waitable Pool and the basic Deadlock-Free Pool use a single shared work queue. Every `submit()` and `try_pop()` operation accesses the same queue, requiring synchronization.

As the number of CPU cores increases, this design creates two major bottlenecks:

- **Lock Contention:** Multiple threads compete for the same mutex, reducing parallelism.
- **Cache Ping-Pong:** Frequent updates to the shared queue invalidate cache lines across CPU cores, increasing cache coherence traffic even with a lock-free queue.

## Solution

Assign each worker thread its own private work queue using a `thread_local` pointer.

When a worker thread submits a child task, the task is pushed directly into its local queue. Since only the owning thread accesses this queue, it can use a simple container such as `std::queue` without synchronization. The global queue remains available as a fallback for tasks submitted by external threads or when a worker's local queue is empty.

Implementation: [localQueuePool](localQueuePool.hpp)

## Concept and Implementation

[localQueuePool](localQueuePool.hpp)

The execution flow is:

1. If the current thread owns a local queue, `submit()` stores the task there without synchronization.
2. Otherwise, the task is pushed into the shared global queue.
3. `run_pending_task()` always checks the local queue first.
4. If the local queue is empty, it retrieves a task from the global queue.
5. If no task is available, the thread calls `std::this_thread::yield()`.

## Limitation: Work Imbalance

Local work queues significantly reduce contention but introduce uneven work distribution.

In recursive algorithms such as parallel Quicksort, a worker continuously generates child tasks and places them into its own local queue. Since these tasks never enter the global queue, other workers eventually exhaust the shared queue and become idle.

As a result, one thread processes most of the workload while the remaining threads repeatedly call `yield()`. This limitation motivates the next improvement: **Work Stealing**.


# 4. Work Stealing: Eliminating Work Imbalance

## Problem

While thread-local work queues reduce contention, they introduce a severe work imbalance. Threads processing tasks that generate many sub-tasks (such as recursive Quicksort) accumulate massive local queues, while other threads exhaust their local and global workloads early, remaining idle and wasting CPU cycles via continuous `yield()` calls.

To allow an idle thread to take work from a busy one, local queues must be accessible pool-wide. However, exposing these queues introduces cross-thread data contention and requires proper synchronization to protect container invariants.

## Solution

Implement a Work-Stealing Queue and an extended task retrieval pipeline.

### Dual-ended Queue Architecture

Replace the local `std::queue` with a double-ended container (`std::deque`). The queue owner operates on the front (LIFO), while stealing threads extract tasks from the back (FIFO).

### Cache Locality and Stealing Efficiency

Processing the most recent tasks first (LIFO) ensures that the data is likely still warm in the local CPU cache. Conversely, stealing from the opposite end (the back) minimizes locking contention between the owner and the thieves.

### Distributed Stealing

Each thread offsets its stealing starting index by its own thread ID to prevent a "herd effect" where all idle threads target the same worker simultaneously.

## Concept and Implementation: [workStealingPool.hpp](workStealingPool.hpp)

## Limitation: Synchronization Overhead and Static Scaling

While work stealing resolves the load imbalance problem, this implementation carries its own performance boundaries.

### Locking Bottleneck

Wrapping `std::deque` with a coarse-grained `std::mutex` means that `try_pop()` and `try_steal()` still compete for the same lock. A highly scalable production pool requires a lock-free single-writer, multi-reader queue to truly isolate the owner from thieves.

### Static Resizing Lack

The pool allocation is bound strictly to `std::thread::hardware_concurrency()`. It lacks dynamic scalability mechanisms to spin up additional threads if current workers block on synchronous operations such as I/O or external locks.