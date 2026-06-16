# Atomic Types and Operations

## 1. Atomic Operations

### Definition

An atomic operation is an **indivisible operation**. The system can only observe two states:

* Not performed
* Completely performed

There is no intermediate "partially completed" state.

### Purpose

Atomic operations ensure data integrity when multiple threads access the same object.

If all `load` and `store` operations on an object are atomic, a thread will always observe either:

* The original value
* The value written by a completed atomic store

### Danger of Non-Atomic Access

Without atomic operations, one thread may observe a partially modified value.

```text
Thread A (Writer)
        |
        v
 [ Shared Object ]
        ^
        |
Thread B (Reader)
```

Accessing the same memory location concurrently, combined with a write operation and no synchronization, results in a **data race**, which leads to **undefined behavior**.

---

## 2. Atomic Types (`<atomic>`)

### A. `std::atomic_flag`

The simplest atomic type, representing a boolean flag.

#### Characteristics

* Always lock-free
* Cannot be copied
* Cannot be assigned
* No non-modifying query operations

#### Initialization

```cpp
std::atomic_flag flag = ATOMIC_FLAG_INIT;
```

#### Typical Use

Used as the foundation for implementing synchronization primitives such as spinlocks.

---

### B. `std::atomic<bool>`

A more complete alternative to `atomic_flag`.

#### Supported Operations

* `load()`
* `store()`
* `exchange()`
* `compare_exchange_*()`

Can be initialized and assigned using ordinary boolean values.

---

### C. `std::atomic<T*>` (Atomic Pointers)

Atomic specialization for pointers.

#### Supported Operations

* `fetch_add()`
* `fetch_sub()`
* `+=`
* `-=`
* `++`
* `--`

#### Important Note

```cpp
ptr += 1;
```

returns the new value.

```cpp
ptr.fetch_add(1);
```

returns the old value.

This distinction is important when preventing race conditions.

---

### D. Atomic Integral Types

Examples:

```cpp
std::atomic<int>
std::atomic<long>
std::atomic<unsigned>
```

#### Supported Operations

Arithmetic:

* `+`
* `-`
* `++`
* `--`

Bitwise:

* `&`
* `|`
* `^`

#### Not Supported

* Multiplication
* Division
* Bit shifting

---

### E. Atomic User-Defined Types

A user-defined type can be used with `std::atomic<T>` only if it satisfies strict requirements.

#### Requirements

* Trivial copy-assignment operator
* No virtual functions
* No virtual base classes
* Must support bitwise equality comparison

#### Limitation

Designed for simple structures containing small pieces of data such as flags or pointers.

Not intended for complex containers such as `std::vector`.

---

## 3. Compare-and-Exchange (CAS)

Compare-and-Exchange is the cornerstone of lock-free programming.

### Mechanism

```text
Current Value == Expected ?
          |
      +---+---+
      |       |
    Yes      No
      |       |
      v       v
Write      Update
Desired    Expected
```

The operation:

1. Compares the current atomic value with `expected`.
2. If equal, writes `desired`.
3. Otherwise, updates `expected` with the current value.

### Variants

#### `compare_exchange_weak()`

May fail spuriously even when values are equal.

Typically used inside a loop.

```cpp
while (!obj.compare_exchange_weak(expected, desired)) {
    // retry
}
```

#### `compare_exchange_strong()`

Does not suffer from spurious failure.

Useful when the result must be known precisely without repeated retries.

---

## 4. Available Operations

| Operation        | atomic_flag | atomic<bool> | atomic<T*> | atomic<int> |
| ---------------- | ----------- | ------------ | ---------- | ----------- |
| test_and_set     | ✓           |              |            |             |
| clear            | ✓           |              |            |             |
| is_lock_free     |             | ✓            | ✓          | ✓           |
| load/store       |             | ✓            | ✓          | ✓           |
| exchange         |             | ✓            | ✓          | ✓           |
| compare_exchange |             | ✓            | ✓          | ✓           |
| fetch_add/sub    |             |              | ✓          | ✓           |
| fetch_or/and/xor |             |              |            | ✓           |

---

## 5. Memory Ordering

Every atomic operation accepts a memory-order parameter.

The default is:

```cpp
std::memory_order_seq_cst
```

### `memory_order_relaxed`

Only guarantees atomicity.

No ordering constraints on other operations.

Typical use:

```cpp
Counter
Statistics
Reference counts
```

---

### `memory_order_release`

Used when publishing data.

Ensures previous writes become visible before the release operation.

---

### `memory_order_acquire`

Used when consuming data.

Ensures subsequent reads occur after the acquire operation.

---

### `memory_order_seq_cst`

Strongest ordering guarantee.

All threads observe atomic operations in the same order.

Safest option, but generally the most expensive.

---

## 6. Free Atomic Functions

C++ provides free functions that operate on pointers.

Examples:

```cpp
std::atomic_load()
std::atomic_store()
```

These functions are useful for:

* C compatibility
* Atomic operations on `std::shared_ptr`

### Special Case: `std::shared_ptr`

Although `std::shared_ptr` is not an atomic type, C++11 provides:

```cpp
std::atomic_load(&ptr);
std::atomic_store(&ptr, value);
```

as a special exception.

---

## 7. Why Atomic Objects Cannot Be Copied

Copying an atomic object requires:

1. Reading from one memory location.
2. Writing to another memory location.

An atomic operation is only defined on a single memory location.

Therefore, atomic objects do not support copy construction or copy assignment.

---

## 8. Examples

### Example 1: Spinlock

```cpp
#include <atomic>

class SimpleSpinlock {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

public:
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
        }
    }

    void unlock() {
        flag.clear(std::memory_order_release);
    }
};
```

`test_and_set()` performs the test and lock operation atomically.

---

### Example 2: Atomic Counter

```cpp
#include <atomic>

std::atomic<int> counter(0);

void increment() {
    for (int i = 0; i < 1000; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}
```

`fetch_add()` is a read-modify-write operation.

`memory_order_relaxed` is sufficient because only the final counter value matters.

---

### Example 3: Compare-and-Exchange

```cpp
#include <atomic>

std::atomic<int> shared_val(10);

void update_val(int expected, int desired) {
    if (shared_val.compare_exchange_strong(expected, desired)) {
        // Success
    } else {
        // Failure
        // expected now contains the current value
    }
}
```

---

## 9. Core Summary

```text
Atomic
│
├── atomic_flag
│     └── Spinlocks
│
├── atomic<bool>
│
├── atomic<T*>
│
├── atomic<Integral>
│
└── atomic<User Defined Type>
```

### Key Concepts

* Atomic = Indivisible.
* Atomic operations prevent data races on shared memory locations.
* `is_lock_free()` indicates whether an implementation uses direct CPU instructions.
* `std::atomic_flag` is always lock-free.
* Compare-and-Exchange (CAS) is the foundation of lock-free programming.
* Memory ordering controls visibility and execution ordering between threads.
