# Core Concepts: Thread

This chapter covers the fundamentals of threads, process synchronization, and the POSIX threads API. For detailed theory, implementation details, and API usage, refer to the specific topics linked below.

## 1. Threads and Concurrency
Threads are the basic units of CPU utilization within a process. Because threads within the same process share the same memory space and operating-system resources, inter-thread communication and context-switching are highly efficient compared to working with separate processes.
👉 **Deep Dive:** [Threads and Concurrency Details](01_threads_and_concurrency.md)
- Covers concurrency vs. parallelism, multithreading models (User vs Kernel threads), and common threading issues (e.g., `fork()` semantics and signal handling).

## 2. Synchronization
When multiple threads access and manipulate shared resources concurrently, **race conditions** can occur, leading to data inconsistency. Synchronization mechanisms are essential to protect **critical sections** and ensure predictable execution.
👉 **Deep Dive:** [Synchronization Tools & Examples](02_synchronization.md)
- Explores software and hardware synchronization tools including Mutexes, Semaphores, and Monitors. It also covers classic synchronization problems like Bounded-Buffer and Readers-Writers.

## 3. Deadlocks
Improper use of synchronization primitives can lead to **deadlocks**, a state where two or more threads are blocked indefinitely, each waiting for a resource held by another thread in the group.
👉 **Deep Dive:** [Deadlock Theory and Handling](03_deadlocks.md)
- Explains the four necessary conditions for deadlocks, Resource-Allocation Graphs, and standard strategies to handle deadlocks (Prevention, Avoidance, and Detection).

## 4. Pthreads API and Linux Implementation
The POSIX standard defines the **Pthreads** API, which is the standard mechanism for thread management and synchronization on UNIX-like systems, including Linux.
👉 **Deep Dive:** [Pthreads API Details](04_pthreads_api.md)
- Details the API for thread creation, termination, cancellation, mutexes, and condition variables. It also discusses Linux-specific implementations (such as the Native POSIX Threads Library - NPTL) and complex interactions between threads, signals, and process control.
