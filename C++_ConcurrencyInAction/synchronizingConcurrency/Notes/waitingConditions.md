# Thread Synchronization in Modern C++

## 1. Why Do We Need Synchronization?

When threads run concurrently, the challenge is not only protecting shared data from race conditions. In many cases, one thread must wait until a specific event or condition occurs before it can continue execution.

### Inefficient Approach: Polling with a Flag

A common mistake is to use a shared flag and repeatedly check its value inside a loop:

```cpp
while (!data_ready) {
    // Keep checking
}
```

This technique is called **polling** or **busy waiting**.

#### Problems

* Wastes CPU resources by continuously checking the condition.
* May hold a mutex longer than necessary.
* Can slow down the thread responsible for producing the required data.

### Preferred Approach

Modern C++ provides synchronization primitives that allow threads to sleep efficiently until a condition is satisfied:

* `std::condition_variable`
* `std::future` and `std::promise`

---

# 2. Condition Variable (`std::condition_variable`)

## Concept

A **condition variable** is a synchronization mechanism that allows threads to block (sleep) until a specific condition becomes true.

Instead of repeatedly polling a flag, a waiting thread sleeps and is awakened only when another thread signals that the condition may have changed.

## Declaration

Defined in the `<condition_variable>` header:

```cpp
#include <condition_variable>

std::condition_variable data_cond;
```

## Important Functions

### wait()

```cpp
data_cond.wait(lock, predicate);
```

#### Parameters

* `std::unique_lock<std::mutex>& lock`

  * A locked `unique_lock`.
* `Predicate predicate`

  * A function object (typically a lambda) returning `bool`.

#### Behavior

1. Checks the predicate.
2. If the predicate is `true`, execution continues immediately.
3. If the predicate is `false`:

   * Unlocks the mutex.
   * Puts the thread into a waiting state.
   * Waits for a notification.
4. When notified:

   * Re-locks the mutex automatically.
   * Re-checks the predicate.
   * Continues only when the predicate becomes `true`.

Example:

```cpp
data_cond.wait(lock, [] {
    return data_ready;
});
```

---

### notify_one()

```cpp
data_cond.notify_one();
```

Wakes up **one** thread currently waiting on the condition variable.

---

### notify_all()

```cpp
data_cond.notify_all();
```

Wakes up **all** waiting threads.

---

## Important Warnings

### 1. Spurious Wakeups

A waiting thread may wake up without receiving a notification.

Because of this, the condition must always be checked after waking up.

Correct usage:

```cpp
data_cond.wait(lock, [] {
    return data_ready;
});
```

or

```cpp
while (!data_ready) {
    data_cond.wait(lock);
}
```

---

### 2. Mutex Requirement

`std::condition_variable` works only with:

```cpp
std::mutex
```

and requires:

```cpp
std::unique_lock<std::mutex>
```

instead of:

```cpp
std::lock_guard<std::mutex>
```

because `wait()` must temporarily unlock and re-lock the mutex.

---

### 3. Avoid Side Effects in Predicates

The predicate may be executed multiple times.

Bad example:

```cpp
data_cond.wait(lock, [] {
    return ++counter > 5;
});
```

The predicate should only check state and should not modify it.

---

# 3. Condition Variable Any (`std::condition_variable_any`)

## Concept

`std::condition_variable_any` is a more flexible version of `std::condition_variable`.

It provides the same synchronization functionality but is not limited to `std::mutex`.

## Declaration

```cpp
#include <condition_variable>

std::condition_variable_any cv_any;
```

## Usage

The interface is essentially identical:

```cpp
cv_any.wait(lock, predicate);
cv_any.notify_one();
cv_any.notify_all();
```

## Key Difference

It can operate with any lock type that satisfies the **BasicLockable** requirements, such as:

```cpp
std::shared_mutex
std::recursive_mutex
Custom mutex-like classes
```

## Warning

### Performance Cost

Because of its greater flexibility, `std::condition_variable_any` may incur:

* Additional memory overhead
* Lower performance
* Higher system resource usage

Whenever possible, prefer:

```cpp
std::condition_variable
```

for maximum efficiency.

---

# 4. Standard Condition Variable Pattern

## Thread-Safe Queue Example

### Shared Resources

```cpp
std::queue<T> queue;
std::mutex mtx;
std::condition_variable cv;
```

---

## Producer Thread

The producer inserts data into the queue and notifies waiting consumers.

```cpp
{
    std::lock_guard<std::mutex> lock(mtx);
    queue.push(data);
}

cv.notify_one();
```

### Workflow

1. Lock mutex.
2. Push data.
3. Unlock mutex.
4. Notify waiting thread.

---

## Consumer Thread

The consumer waits until data becomes available.

```cpp
std::unique_lock<std::mutex> lock(mtx);

cv.wait(lock, [] {
    return !queue.empty();
});

auto value = queue.front();
queue.pop();
```

### Workflow

1. Lock mutex.
2. Wait until queue is not empty.
3. Retrieve data.
4. Remove data from queue.
5. Unlock mutex.

---

# 5. Best Practices and Warnings

## Mutable Mutex

Sometimes a class provides `const` member functions such as:

```cpp
bool empty() const;
```

Even though the function is `const`, it may still need to lock a mutex for safe access.

Use:

```cpp
mutable std::mutex mtx;
```

Example:

```cpp
class ThreadSafeQueue {
private:
    mutable std::mutex mtx;
    std::queue<int> data;

public:
    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return data.empty();
    }
};
```

---

## Minimize Lock Scope

Avoid holding a mutex while performing expensive operations.

Bad:

```cpp
std::unique_lock<std::mutex> lock(mtx);

auto item = queue.front();
queue.pop();

process(item);  // Long operation while mutex is locked
```

Good:

```cpp
auto item = queue.front();
queue.pop();

lock.unlock();

process(item);
```

### Benefits

* Reduces contention.
* Improves parallelism.
* Allows other threads to access the shared queue sooner.

---

# Summary

| Feature                       | Purpose                            | Notes                                        |
| ----------------------------- | ---------------------------------- | -------------------------------------------- |
| `std::condition_variable`     | Efficient thread waiting           | Requires `std::mutex` and `std::unique_lock` |
| `std::condition_variable_any` | Flexible waiting mechanism         | Works with any mutex-like lock object        |
| `wait()`                      | Block until condition becomes true | Always use a predicate                       |
| `notify_one()`                | Wake one waiting thread            | Common producer-consumer usage               |
| `notify_all()`                | Wake all waiting threads           | Useful for broadcasts/shutdown events        |
| Spurious Wakeup               | Unexpected wake-up                 | Always re-check condition                    |
| Mutable Mutex                 | Locking inside `const` functions   | Use `mutable std::mutex`                     |
| Lock Scope                    | Keep critical sections short       | Unlock before heavy processing               |
