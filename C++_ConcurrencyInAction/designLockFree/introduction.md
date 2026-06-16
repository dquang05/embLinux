# 1. Classification of Data Structures by Synchronization Mechanism

## Blocking

Algorithms and data structures that use operating system synchronization primitives such as mutexes, condition variables, and futures.

If a thread becomes blocked (for example, while waiting to acquire a lock), the operating system suspends that thread completely.

## Nonblocking

Algorithms and data structures that do not use blocking library functions.

**Note:** Nonblocking does not necessarily mean lock-free.

For example, a spinlock implemented with an `atomic_flag` repeatedly spins in a loop without calling any OS blocking function, but it is still an exclusive lock.

---

# 2. Lock-Free vs. Wait-Free

## Lock-Free

* Multiple threads can access the data structure concurrently.
* If one thread is suspended by the operating system, other threads must still be able to complete their operations without being blocked.
* Typically implemented using compare-and-exchange (CAS) loops.
* If another thread modifies the data, the current thread retries the operation.

### Drawback

Starvation may occur if a thread repeatedly fails its CAS operation due to unfavorable timing.

## Wait-Free

* A stricter subset of lock-free algorithms.
* Every participating thread is guaranteed to complete its operation within a bounded number of steps, regardless of the behavior of other threads.
* Extremely difficult to implement correctly and often results in significantly more complex algorithms.

---

# 3. Lock-Free Design Trade-Offs (Pros & Cons)

## Advantages (When to Use)

### Maximum Concurrency

No thread is blocked by a mutex. At least one thread is always making progress.

### Robustness

If a thread terminates unexpectedly, only that thread's data is lost. The entire system does not become frozen, and deadlock cannot occur.

## Disadvantages (Consider Carefully)

### Extremely Difficult to Write and Debug

Lock-free algorithms are significantly more complex than lock-based designs.

### Live Lock

Two threads may continuously invalidate each other's operations, causing both threads to retry indefinitely.

### Reduced Overall Performance

Atomic operations are often much slower than non-atomic operations.

When many threads continuously contend for the same atomic variable, hardware-level cache ping-pong can occur, significantly reducing performance.
