# 1. Core Challenge: Memory Reclamation

In lock-free data structures, inserting a node (`push`) is often straightforward and only requires a `compare_exchange_weak` loop.

The major challenge lies in removing and deleting a node (`pop`). If Thread A deletes a node while Thread B still holds a pointer to that node, a dangling pointer (undefined behavior) occurs.

## Memory Reclamation Strategies

### Counting Threads in `pop`

Maintain an atomic counter representing the number of threads currently executing `pop`.

* If the count is greater than 1, popped nodes are placed into a `to_be_deleted` list.
* If the count is equal to 1, the thread may safely delete both the node it just popped and all nodes in the pending deletion list.

#### Limitation

Under high load, the counter may never reach 1, causing the pending deletion list to grow indefinitely.

### Hazard Pointers

Each thread owns a hazard pointer, typically stored in `thread_local` storage.

When a thread wants to read a node, it stores the node's address in its hazard pointer, indicating that the node must not be deleted.

When a thread wants to delete a node, it must scan the hazard pointers of all other threads.

* If no thread references the node, it may be deleted.
* Otherwise, the node is placed into a `reclaim_later` list.

#### Limitation

Scanning and accessing the atomic hazard pointer list introduces significant overhead.

### Split Reference Counting

Each node maintains two counters:

* `internal_count`
* `external_count`

The external count is stored together with the pointer in a structure so that both can be updated atomically using CAS.

* Increase `external_count` when reading the pointer.
* Decrease `internal_count` when finished using the node.

A node is deleted only when the total reference count reaches zero.

---

# 2. Concurrent Operations on Both Ends (Queue Design)

Unlike a stack, which only operates on the head, a queue performs operations on both the head (`pop`) and tail (`push`).

## Problem

When multiple threads execute `push` concurrently, they may read the same tail pointer and attempt to update the same node's `data` and `next` fields simultaneously, resulting in a data race.

## Solution: Helping Mechanism

To preserve lock-free behavior and avoid busy-waiting that blocks progress, threads may help each other complete unfinished work.

For example:

* Thread A is delayed before updating the tail pointer.
* Thread B detects the incomplete update.
* Thread B completes the tail update on behalf of Thread A.

This ensures that system-wide progress continues.

To support this mechanism, structural variables such as a node's `next` pointer must be atomic.

---

# 3. Design Guidelines

## Guideline 1: Use `std::memory_order_seq_cst` During Prototyping

This is the strictest memory ordering and makes reasoning about correctness easier.

Only relax ordering to `acquire`, `release`, or `relaxed` after the algorithm works correctly and optimization is required.

Such optimizations are typically based on established happens-before relationships.

## Guideline 2: Always Provide a Lock-Free Memory Reclamation Mechanism

Never call `delete` directly unless it is guaranteed that no thread still holds a reference to the object.

Use techniques such as:

* Hazard Pointers
* Reference Counting

## Guideline 3: Beware of the ABA Problem

### Failure Scenario

1. Thread 1 reads value A.
2. Thread 1 is suspended.
3. Thread 2 changes A to B.
4. Thread 3 changes B back to A, but as a different object occupying the same memory location.
5. Thread 1 resumes, sees A, and its CAS succeeds.

As a result, the internal state becomes corrupted.

### Prevention

Attach an ABA counter to the value.

Each modification increments the counter, and CAS operates on both the value and the counter.

Example:

* `(A,1) → (B,2) → (A,3)`

A CAS expecting `(A,1)` will fail because the counter has changed.

## Guideline 4: Identify Waiting Loops and Help Other Threads

If an algorithm contains a busy-wait loop that depends on another thread completing work, it is no longer lock-free.

Instead, design the algorithm so that idle threads can complete unfinished work on behalf of busy or suspended threads.

# Example
After understanding the core challenges and design principles of lock-free data structures, let's look at a simple data structure that can be implemented to demonstrate these concepts.
[dataStructures.hpp](dataStructures.hpp)
