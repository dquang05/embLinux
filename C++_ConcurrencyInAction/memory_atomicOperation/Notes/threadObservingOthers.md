# Memory Ordering and Synchronization

## 1. Core Concepts

This chapter addresses a fundamental question:

> How can threads observe each other's work in the correct order?

### Happens-Before

Defines a logical ordering relationship.

If operation **A happens-before B**, then all effects of **A** are visible to **B**.

---

### Synchronizes-With

A relationship between an atomic store operation and an atomic load operation.

It acts as a bridge for establishing a happens-before relationship between threads.

---

### Release Sequence

A sequence of consecutive Read-Modify-Write (RMW) operations.

A Store-Release operation at the beginning of the sequence can synchronize with any Load-Acquire operation that observes a value from that sequence.

---

## 2. Memory Ordering Modes

Every atomic operation accepts a `memory_order` parameter.

If omitted, the default ordering is:

```cpp
std::memory_order_seq_cst
```

| Mode                   | Purpose                                                                          | Performance |
| ---------------------- | -------------------------------------------------------------------------------- | ----------- |
| `memory_order_relaxed` | Guarantees atomicity only. No ordering constraints.                              | Highest     |
| `memory_order_acquire` | Used with loads. Prevents subsequent operations from moving before the load.     | Medium      |
| `memory_order_release` | Used with stores. Ensures prior operations complete before publishing the value. | Medium      |
| `memory_order_acq_rel` | Used with RMW operations. Combines Acquire and Release semantics.                | Medium      |
| `memory_order_seq_cst` | All threads observe a single global order of operations.                         | Lowest      |

---

## 3. Syntax and Usage

### Atomic Operations

```cpp
std::atomic<int> a;

// Store
a.store(10, std::memory_order_release);

// Load
int val = a.load(std::memory_order_acquire);

// Read-Modify-Write
a.fetch_sub(1, std::memory_order_acq_rel);
```

---

### Fences

Fences enforce ordering constraints without requiring more complex atomic operations.

```cpp
// Typically used together with relaxed atomics

std::atomic_thread_fence(std::memory_order_release);

// ... operations ...

std::atomic_thread_fence(std::memory_order_acquire);
```

#### Intuition

```text
Instructions Above
        │
        ▼
----------------------------
 Release Fence
----------------------------
        │
        ▼
   Shared State
        │
        ▼
----------------------------
 Acquire Fence
----------------------------
        │
        ▼
Instructions Below
```

* A Release Fence prevents preceding operations from moving below it.
* An Acquire Fence prevents subsequent operations from moving above it.

---

## 4. Common Pitfalls

### Data Race

Never read or write a non-atomic variable from multiple threads without using atomic operations or a mutex to establish a happens-before relationship.

---

### Spurious Failure

`compare_exchange_weak()` may fail even when the expected value matches.

Always use it inside a loop.

```cpp
while (!obj.compare_exchange_weak(expected, desired)) {
    // retry
}
```

---

### Overconfidence with Weak Memory Ordering

Using memory orders weaker than `seq_cst` can easily introduce subtle logic bugs.

These bugs are often difficult to reproduce and may not appear during print-based debugging.

---

### Transitivity

Synchronization does not automatically propagate through unrelated operations.

A complete Acquire-Release chain is required to establish ordering across threads.

---

## 5. Usage Guidelines

### Default Choice: `seq_cst`

```cpp
std::atomic<int> counter{0};
```

Use the default ordering whenever possible.

#### Recommended When

* Correctness is the primary concern.
* The code is not a performance bottleneck.

This is the preferred choice for the vast majority of applications.

---

### Performance-Oriented: Acquire / Release

Use explicit Acquire-Release synchronization.

#### Recommended When

* Implementing lock-free queues.
* Implementing lock-free stacks.
* Maximizing throughput in concurrent data structures.

---

### Simple Counters: Relaxed

Use `memory_order_relaxed`.

#### Recommended When

* Counting events.
* Tracking completed tasks.
* Gathering statistics.

The value is not used to control the ordering of other shared data.

---

### Working with Non-Atomic Data

If a large object or structure cannot be made atomic, use Release and Acquire fences to enforce ordering around it.

---

## Final Recommendation

Do not attempt to optimize performance using `relaxed` ordering or fences unless profiling has shown that synchronization is a bottleneck.

Prioritize correctness first, then optimize only when necessary.
