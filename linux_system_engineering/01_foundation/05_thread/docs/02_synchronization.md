# Synchronization

This document covers the fundamental tools and classic examples of thread and process synchronization based on Operating System Concepts (Chapters 6 and 7).

## 1. Background
- Concurrent access to shared data may result in data inconsistency.
- **Race Condition:** A situation where multiple processes or threads access and manipulate shared data concurrently, and the final outcome depends on the particular order in which the execution takes place. Synchronization mechanisms are required to prevent race conditions.

## 2. The Critical-Section Problem
- **Critical Section:** A segment of code where a process may be accessing and updating shared data.
- The fundamental requirement is that when one process is executing in its critical section, no other process is allowed to execute in its critical section.
- A valid solution to the critical-section problem must satisfy three requirements:
  1. **Mutual Exclusion:** If process $P_i$ is executing in its critical section, then no other processes can be executing in their critical sections.
  2. **Progress:** If no process is executing in its critical section and some processes wish to enter, the selection of the next process cannot be postponed indefinitely.
  3. **Bounded Waiting:** There must be a bound on the number of times that other processes are allowed to enter their critical sections after a process has made a request to enter its critical section.

## 3. Peterson's Solution
- A classic software-based solution to the critical-section problem for two processes. It uses two shared variables: `turn` (whose turn it is to enter) and `flag` (readiness to enter).
- Note: The abstract Peterson algorithm assumes suitable ordering of shared accesses. Ordinary concurrent C accesses to `turn` and `flag` can form data races (undefined behavior); hardware barriers alone do not fix that. A C implementation needs appropriate atomic operations and memory ordering, not just the abstract pseudocode.

## 4. Hardware Support for Synchronization
- **Memory Barriers:** Compiler barriers constrain compiler reordering; hardware barriers constrain specified processor memory ordering. They do not instantly propagate every change to every CPU. C visibility guarantees come from the required atomic operations and synchronization relationships (for example release/acquire or mutex unlock/lock). Cache coherence concerns cache copies; it does not replace software synchronization or make C data races valid.
- **Hardware Instructions:** Atomic (non-interruptible) instructions provided by modern hardware, such as `test_and_set()` and `compare_and_swap()` (CAS).
- **Atomic Variables:** C atomic operations provide indivisible accesses under the language memory model; implementations may use hardware instructions or locks. Ordering of other accesses depends on the selected memory order and synchronization relationships.

## 5. Mutex Locks
- The simplest and most widely used higher-level software synchronization tool (Mutex = Mutual Exclusion).
- A process must `acquire()` the lock before entering a critical section and `release()` the lock when exiting.
- **Spinlock:** A type of mutex where a process "spins" (busy waits) in a loop while waiting for the lock to become available. Spinlocks are useful in multiprocessor systems for very short critical sections because they avoid the overhead of context switching.

## 6. Semaphores
- A more robust synchronization tool that behaves like an integer variable `S` accessed only through two atomic operations: `wait()` (or P, decrement) and `signal()` (or V, increment).
- **Counting Semaphore:** Conceptually tracks available resource instances with a nonnegative count; POSIX semaphore values have a finite upper bound, `SEM_VALUE_MAX`. Posting at that limit can fail with `EOVERFLOW`.
- **Binary Semaphore:** A protocol keeps the count at 0 or 1 and can provide mutual exclusion. Unlike a mutex, a semaphore has no owner: a different thread may post it. A mutex must be unlocked by its owner under the appropriate mutex-type contract; they are not universally interchangeable.
- **Avoiding Busy Waiting:** Instead of spinning, a `wait()` operation can suspend (block) the calling process, placing it in a waiting queue. A subsequent `signal()` operation will wake up a waiting process.

## 7. Monitors
- Using semaphores incorrectly (e.g., swapping the order of wait and signal) can cause insidious timing errors.
- **Monitor:** A high-level abstraction (an Abstract Data Type) that provides a convenient and effective mechanism for process synchronization. It automatically guarantees that only one process at a time can be active within the monitor.
- Monitors provide **Condition Variables** along with `wait()` and `signal()` operations so processes can wait for specific conditions to be met within the monitor.

## 8. Liveness
- Liveness properties ensure that a system makes progress and processes don't wait indefinitely.
- **Deadlock:** A situation where two or more processes are waiting indefinitely for an event that can only be caused by one of the waiting processes.
- **Priority Inversion:** A scenario where a high-priority process is indirectly preempted by a lower-priority process holding a needed resource. Usually solved via a **priority-inheritance protocol** (the lower-priority process temporarily inherits the higher priority until it releases the resource).

## 9. Classic Synchronization Problems
- **Bounded-Buffer Problem (Producer-Consumer):** Producers and consumers share a buffer. One solution uses a mutex and two counting semaphores (`empty` and `full`). Another uses a mutex plus condition variables with full/empty predicates checked in loops, as in `04_producer_consumer.c`; two semaphores are not required.
- **Readers-Writers Problem:** Allows multiple readers to read shared data concurrently, but writers require exclusive access. A reader-writer lock improves concurrency in read-heavy applications.
- **Dining-Philosophers Problem:** A classic model for allocating limited resources among a group of processes in a deadlock-free and starvation-free manner. Philosophers must pick up both left and right chopsticks to eat.

## 10. Kernel Synchronization Examples
- **Windows:** 
  - Uses spinlocks for short code segments on multiprocessors. 
  - Uses dispatcher objects (mutexes, semaphores, events, timers) for synchronization outside the kernel.
- **Linux:** 
  - Kernel preemptibility depends on configuration and execution context; critical or interrupt-disabled regions impose additional constraints.
  - Utilizes atomic integers, spinlocks, and mutex locks.
  - On non-RT uniprocessor configurations, spinlock operations can reduce to preemption control, with interrupt protection depending on context/API. On non-RT multiprocessors, spinlocks spin and disable preemption. PREEMPT_RT changes `spinlock_t` to sleeping RT-mutex-based semantics; `raw_spinlock_t` remains a spinning lock. Configuration and architecture matter; these kernel rules differ from user-space pthread synchronization.

## 11. POSIX Synchronization
- Provides user-level API for UNIX, Linux, and macOS:
  - **POSIX Mutex Locks:** Managed using `pthread_mutex_t`, `pthread_mutex_lock()`, and `pthread_mutex_unlock()`.
  - **POSIX Semaphores:** Available as named semaphores (easily shared among unrelated processes) and unnamed semaphores (memory-based).
  - **POSIX Condition Variables:** Managed using `pthread_cond_t` in conjunction with a mutex to mimic monitor-like behavior (`pthread_cond_wait()`, `pthread_cond_signal()`).

## 12. Java Synchronization
- **Java Monitors:** Every object has a built-in lock. The `synchronized` keyword secures the object lock. Threads use `wait()` and `notify()` to interact with the object's wait set.
- **Reentrant Locks:** Available via the `java.util.concurrent.locks` package (e.g., `ReentrantLock`, `ReentrantReadWriteLock`).
- **Semaphores & Condition Variables:** Available in `java.util.concurrent` for more complex synchronization needs.

## 13. Alternative Approaches
- **Transactional Memory:** Groups memory operations into transactions that may abort and retry. It can reduce explicit locking hazards, but guarantees depend on the transaction model, fallback paths and interaction with nontransactional code; it does not universally eliminate races or deadlocks.
- **OpenMP:** Uses `#pragma omp critical` to automatically mark critical sections for the compiler.
- **Functional Programming:** Immutable data can reduce shared-mutable-state races. Guarantees depend on the language and program: mutable components, message protocols and resource waits can still introduce races or deadlocks.
