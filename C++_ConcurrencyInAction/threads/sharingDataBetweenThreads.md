# Chapter 3: Sharing Data Between Threads

This chapter focuses on one of the most important problems in concurrent programming:

> How can multiple threads safely share and modify data without introducing concurrency bugs?

---

# 1. Problems When Sharing Data

## Read-Only Data Is Safe

If shared data is never modified, multiple threads can access it simultaneously without synchronization issues.

The problem begins when at least one thread modifies the shared data.

---

## Invariants

An **invariant** is a condition that must always remain true for a data structure.

Examples:

* The size of a container matches the number of stored elements.
* A doubly-linked list maintains valid forward and backward links.

While a thread is updating a data structure, its invariants may be temporarily broken.

If another thread accesses the data during this period, unexpected behavior may occur.

---

## Race Conditions

A **race condition** occurs when the outcome of a computation depends on the relative timing or execution order of multiple threads.

### Benign Race Condition

Different execution orders produce different results, but all results are acceptable.

### Problematic Race Condition

One thread observes data while another thread is modifying it, leading to:

* Corrupted data structures
* Incorrect results
* Program crashes

---

## Data Race

A **data race** occurs when:

* Two or more threads access the same memory location simultaneously.
* At least one access is a write.
* No synchronization mechanism is used.

In C++, a data race results in **undefined behavior (UB)**.

---

# 2. Protecting Shared Data with Mutexes

A **mutex** (Mutual Exclusion) acts like a lock.

A thread must:

1. Lock the mutex before accessing shared data.
2. Unlock the mutex when finished.

---

## Basic Usage

The C++ Standard Library provides `std::mutex` in `<mutex>`.

### Manual Locking (Not Recommended)

```cpp
mutex.lock();

/* Access shared data */

mutex.unlock();
```

This approach is dangerous because an early `return` or exception may prevent the mutex from being unlocked.

---

## RAII with `std::lock_guard`

The preferred approach is to use RAII.

```cpp
#include <list>
#include <mutex>

std::list<int> some_list;
std::mutex some_mutex;

void add_to_list(int new_value)
{
    std::lock_guard<std::mutex> guard(some_mutex);

    some_list.push_back(new_value);
}
```

Advantages:

* Locks automatically during construction.
* Unlocks automatically during destruction.
* Exception-safe.

---

## Designing a Safe Interface

Simply protecting member functions with mutexes is not enough.

Dangerous example:

```cpp
T& get_data();
```

or

```cpp
T* get_data();
```

Returning pointers or references exposes protected data outside the lock's lifetime.

### Golden Rule

> Never pass pointers or references to protected data outside the lock scope.

---

# 3. Race Conditions Hidden Inside Interfaces

Even if every member function is individually protected by a mutex, the overall interface may still be unsafe.

---

## Example: `std::stack`

Suppose a stack provides:

```cpp
top();
pop();
```

Thread A:

```cpp
if(!s.empty())
{
    auto value = s.top();
}
```

Thread B:

```cpp
s.pop();
```

Possible sequence:

1. Thread A checks `!empty()`.
2. Thread B removes the last element.
3. Thread A calls `top()`.

Result:

* Undefined behavior.

The mutex protected individual operations, but not the logical sequence.

---

## Better Interface Design

Combine operations into a single atomic action.

### Option 1: Output Parameter

```cpp
void pop(T& value);
```

### Option 2: Smart Pointer Return

```cpp
std::shared_ptr<T> pop();
```

Both approaches eliminate the race window.

---

# 4. Deadlock and How to Avoid It

## What Is Deadlock?

Deadlock occurs when:

* Thread A owns Mutex A and waits for Mutex B.
* Thread B owns Mutex B and waits for Mutex A.

Neither thread can proceed.

The program becomes permanently stuck.

---

## Using `std::lock`

C++ provides `std::lock()` to lock multiple mutexes safely.

```cpp
friend void swap(X& lhs, X& rhs)
{
    if (&lhs == &rhs)
        return;

    std::lock(lhs.m, rhs.m);

    std::lock_guard<std::mutex> lock_a(
        lhs.m, std::adopt_lock);

    std::lock_guard<std::mutex> lock_b(
        rhs.m, std::adopt_lock);

    swap(lhs.some_detail, rhs.some_detail);
}
```

`std::lock()` uses a deadlock-avoidance algorithm internally.

`std::adopt_lock` tells `lock_guard` that the mutex is already locked.

---

## Deadlock Prevention Rules

### Avoid Nested Locks

If possible, do not acquire a second lock while holding another.

---

### Avoid Calling User Code While Holding a Lock

User code may acquire additional locks, causing unexpected deadlocks.

---

### Use a Fixed Locking Order

Always acquire mutexes in the same order.

Example:

```text
Mutex A -> Mutex B
```

Never:

```text
Thread 1: A -> B
Thread 2: B -> A
```

---

### Use Lock Hierarchies

Assign hierarchy levels to mutexes.

A thread may only acquire lower-level mutexes while holding a higher-level mutex.

This guarantees a consistent locking order.

---

# 5. Advanced Locking Mechanisms

## `std::unique_lock`

`std::unique_lock` is more flexible than `std::lock_guard`.

Features:

* Deferred locking (`std::defer_lock`)
* Manual `lock()` and `unlock()`
* Ownership transfer

Example:

```cpp
std::unique_lock<std::mutex> lock(
    m,
    std::defer_lock);

lock.lock();

/* Critical section */

lock.unlock();
```

Trade-offs:

* More memory overhead
* Slightly slower than `std::lock_guard`

---

## Lock Granularity

### Coarse-Grained Locking

One mutex protects a large amount of data.

Advantages:

* Easy to implement
* Easier to reason about

Disadvantages:

* Reduced concurrency
* Poor scalability

---

### Fine-Grained Locking

Many mutexes protect different parts of a system.

Advantages:

* Better parallel performance

Disadvantages:

* Increased complexity
* Higher deadlock risk

---

### Best Practice

> Hold locks for the shortest possible time.

Avoid performing expensive operations while holding a mutex:

* File I/O
* Network communication
* Database access
* Long computations

---

# 6. Alternatives to Traditional Mutexes

## One-Time Initialization with `std::call_once`

A common problem is lazy initialization of shared resources.

Old approaches:

* Locking every access
* Double-Checked Locking (unsafe)

Modern C++ solution:

```cpp
std::shared_ptr<some_resource> resource_ptr;
std::once_flag resource_flag;

void init_resource()
{
    resource_ptr.reset(
        new some_resource);
}

void foo()
{
    std::call_once(
        resource_flag,
        init_resource);

    resource_ptr->do_something();
}
```

### Guarantees

* Initialization occurs exactly once.
* Thread-safe.
* Efficient.

---

## Thread-Safe Local Static Variables

Since C++11, local static variables are initialized safely in multithreaded environments.

```cpp
SomeClass& get_instance()
{
    static SomeClass instance;
    return instance;
}
```

Initialization is automatically synchronized.

---

## Reader-Writer Locks (`shared_mutex`)

Useful when:

* Reads are frequent.
* Writes are rare.

Examples:

* DNS cache
* Configuration cache
* Lookup tables

---

### Shared (Read) Lock

Multiple readers can access simultaneously.

```cpp
std::shared_lock<std::shared_mutex> lock(mutex);
```

---

### Exclusive (Write) Lock

Writers require exclusive ownership.

```cpp
std::unique_lock<std::shared_mutex> lock(mutex);
```

While a writer is active:

* No readers may proceed.
* No other writers may proceed.

---

## Recursive Mutex (`std::recursive_mutex`)

Allows the same thread to lock a mutex multiple times.

```cpp
std::recursive_mutex m;
```

Although useful in some situations, it is generally considered a sign of poor design.

Preferred approach:

* Refactor code.
* Separate locking logic from internal implementation functions.

---

# Key Takeaways

* Data races cause undefined behavior and must be avoided.
* Mutexes protect shared mutable data.
* Use RAII (`std::lock_guard`) instead of manual lock/unlock.
* Interface design is just as important as mutex usage.
* Deadlocks occur when threads wait on each other indefinitely.
* `std::lock()` safely acquires multiple mutexes.
* `std::unique_lock` provides additional flexibility.
* Hold locks for the shortest possible duration.
* Use `std::call_once` for thread-safe one-time initialization.
* `shared_mutex` improves performance for read-heavy workloads.
* `recursive_mutex` should be used sparingly and is often a design smell.
