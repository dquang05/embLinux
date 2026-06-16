# Concurrent Data Structures: Design Evolution and Synchronization Strategies

This section summarizes the progression of concurrent data structures presented throughout the chapter, highlighting how each design improves concurrency, scalability, and synchronization efficiency.

Example in [dataStructures.hpp](dataStructures.hpp)
---

# 1. Basic Lock-Based Thread-Safe Stack

The simplest approach is to wrap a standard container (such as `std::stack`) with a single `std::mutex`.

## Data Protection

Every member function uses `std::lock_guard` to ensure that only one thread can access the stack at a time.

## Race Condition Prevention

The traditional interface (`top()` followed by `pop()`) is removed because it is vulnerable to race conditions.

Instead, `pop()` is designed to return the value while removing the element, either through:

* A reference parameter.
* A `std::shared_ptr`.

## Exception Safety

Operations that may throw exceptions (such as memory allocation) are performed before the container is actually modified (`data.pop()`).

## Limitation (Performance Bottleneck)

Access is heavily serialized because all operations share the same mutex.

In addition, the lack of a waiting mechanism forces threads to continuously poll the stack using `empty()`, wasting CPU resources.

---

# 2. Thread-Safe Queue with Condition Variables

To solve the waiting problem of the stack, the queue design incorporates a `std::condition_variable`.

## Waiting Mechanism (Wait and Pop)

The `wait_and_pop()` function allows a thread to sleep instead of continuously polling.

It wakes up only when another thread calls `push()` and signals the condition variable with `notify_one()`.

## Improved Exception Safety

To prevent data loss if an exception occurs after a waiting thread is awakened, the queue stores `std::shared_ptr<T>` objects instead of storing values directly.

## Reduced Lock Holding Time

Using `std::shared_ptr` allows memory allocation to occur outside the mutex-protected section of `push()`, reducing lock holding time.

## Limitation

Although improved, this design still wraps a standard `std::queue`, meaning the entire queue remains protected by a single mutex.

As a result, push and pop operations cannot run concurrently.

---

# 3. Fine-Grained Lock Queue

To achieve true concurrency, where push and pop operations can execute simultaneously, the underlying queue implementation is replaced with a singly linked list.

## Separate Pointers

The queue maintains two separate pointers:

* `head` (used for pop operations)
* `tail` (used for push operations)

## Dummy Node

The dummy node is the key to the design.

If the queue is empty or contains only one element, `head` and `tail` may point to the same node, creating contention.

A permanently existing dummy node ensures that `head` and `tail` always refer to different nodes.

## Two Mutexes

Thanks to the dummy node, two independent locks can be used:

* `head_mutex` protects pop operations.
* `tail_mutex` protects push operations.

## Maximized Concurrency

### Push

Creation of the new data object and node is performed entirely outside the lock.

`tail_mutex` is held only briefly to update pointers.

### Pop

`head_mutex` is used only while safely updating the head pointer.

Deletion of the old node is performed after the lock has been released.

---

# 4. Thread-Safe Lookup Table (Hash Table)

The next step extends concurrency support beyond linear data structures to associative containers.

The `threadsafe_lookup_table` is implemented as a hash table consisting of multiple independent buckets.

## Bucket-Based Partitioning

Instead of protecting the entire table with a single lock, the data is divided into multiple buckets.

Each bucket maintains:

* Its own container for key-value pairs.
* Its own read-write lock (`std::shared_mutex`).

This allows operations on different buckets to proceed independently.

## Reader-Writer Synchronization

Each bucket uses a reader-writer locking strategy:

* Multiple readers may access the bucket simultaneously.
* Writers obtain exclusive access when modifying data.

This significantly improves scalability for read-heavy workloads.

## Increased Concurrency

Because buckets are independent, threads accessing different buckets do not contend for the same lock.

In practice, the potential concurrency can increase by approximately the number of buckets in the table.

## Snapshot Retrieval (`get_map()`)

Obtaining a complete copy of the table requires access to every bucket.

To avoid deadlock while acquiring multiple locks, the implementation follows a strict lock ordering strategy:

1. Lock bucket 0.
2. Lock bucket 1.
3. Continue sequentially.
4. Lock the final bucket.

Since every thread acquires locks in the same order, circular wait conditions cannot occur.

## Implementation Note

The original implementation uses `boost::shared_mutex`.

In modern C++ implementations, it can be replaced with `std::shared_mutex` (C++17) to eliminate external dependencies while preserving the same synchronization behavior.

---

# 5. Thread-Safe Singly Linked List with Per-Node Locks

The `threadsafe_list` introduces an even finer level of synchronization.

Instead of protecting the entire container or large sections of it, each node owns its own mutex.

## Per-Node Synchronization

Each node contains:

* Stored data.
* Pointer to the next node.
* A dedicated `std::mutex`.

This allows different threads to operate on different regions of the list concurrently.

## Hand-Over-Hand Locking (Lock Coupling)

Safe traversal is achieved through hand-over-hand locking.

The traversal process follows this pattern:

1. Lock the current node.
2. Lock the next node.
3. Release the current node.
4. Move forward.
5. Repeat until the target node is reached.

At every step, at least one relevant node remains protected.

## Advantages

Compared with a single global lock:

* Multiple threads can traverse different portions of the list simultaneously.
* Lock contention is greatly reduced.
* Scalability improves as the list grows.

## Trade-Off

The design introduces:

* More mutex objects.
* Increased locking and unlocking operations.
* Greater implementation complexity.

However, it provides significantly better concurrency than a list protected by a single global mutex.
