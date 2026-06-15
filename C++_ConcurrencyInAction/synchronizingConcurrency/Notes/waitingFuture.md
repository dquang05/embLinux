# One-Off Event Synchronization with Futures in Modern C++

Futures provide a mechanism for synchronizing **one-time events** between threads. Unlike condition variables, which are often used for repeatedly changing conditions, futures are designed for situations where a result will become available exactly once.

---

# 1. `std::future<T>` (Unique Future)

## Concept

A `std::future` allows a thread to wait for the result of an operation that will complete in the future.

It represents **exclusive ownership** of the result associated with an asynchronous operation.

The result may come from:

* `std::async`
* `std::packaged_task`
* `std::promise`

---

## Usage

A future is typically obtained through:

```cpp
std::future<T> future = ...;
```

The result can be retrieved using:

```cpp
future.get();
```

If the result is not yet available, the calling thread blocks until the future becomes ready.

---

## Warnings

### Non-Copyable

A `std::future` cannot be copied.

```cpp
std::future<int> f1 = ...;
std::future<int> f2 = f1; // Error
```

It can only be moved:

```cpp
std::future<int> f2 = std::move(f1);
```

---

### No Internal Synchronization

Member functions of a particular `std::future` instance are not synchronized.

Never access the same future object from multiple threads simultaneously without external synchronization.

Doing so may result in a data race.

---

### One-Shot Behavior

A future's result can only be retrieved once.

```cpp
auto value = future.get();
```

After calling `get()`:

```cpp
future.get(); // Invalid
```

The future no longer owns a valid shared state.

---

# 2. `std::async`

## Concept

`std::async` is the simplest way to launch asynchronous tasks in Modern C++.

It automatically manages task execution and creates a corresponding future.

In many situations, it can replace manual thread creation.

---

## Syntax

```cpp
auto future = std::async(policy, function, args...);
```

### Parameters

#### Launch Policy

Determines how the task is executed.

```cpp
std::launch::async
```

Forces execution on a separate thread.

---

```cpp
std::launch::deferred
```

Defers execution until `get()` or `wait()` is called.

The task runs synchronously in the calling thread.

---

```cpp
std::launch::async | std::launch::deferred
```

Allows the implementation to choose either strategy.

---

#### Function

The callable object to execute:

```cpp
function
```

Can be:

* Function pointer
* Lambda
* Functor
* Member function

---

#### Arguments

Parameters passed to the callable:

```cpp
args...
```

---

## Warning

When using:

```cpp
std::launch::deferred
```

the task may never execute if neither:

```cpp
future.get();
```

nor

```cpp
future.wait();
```

is called.

---

# 3. `std::packaged_task<Signature>`

## Concept

A `std::packaged_task` wraps a callable object and automatically connects its return value to a future.

It separates:

* Task creation
* Task execution

This makes it particularly useful in:

* Thread pools
* GUI applications
* Job scheduling systems

---

## Syntax

```cpp
std::packaged_task<int(int, int)> task(func);

std::future<int> future =
    task.get_future();
```

---

## Example

```cpp
int add(int a, int b)
{
    return a + b;
}

std::packaged_task<int(int, int)> task(add);

std::future<int> future =
    task.get_future();

task(10, 20);

int result = future.get();
```

Result:

```text
30
```

---

## Typical Workflow

1. Create a packaged task.
2. Obtain its future.
3. Transfer the task to another thread or worker queue.
4. Execute the task.
5. Retrieve the result through the future.

---

# 4. `std::promise<T>`

## Concept

A `std::promise` is a low-level synchronization primitive that allows one thread to manually provide a result to another thread.

The producer thread owns the promise.

The consumer thread owns the future associated with that promise.

---

## Usage

Create the promise:

```cpp
std::promise<int> promise;
```

Obtain the future:

```cpp
std::future<int> future =
    promise.get_future();
```

---

### Setting a Value

```cpp
promise.set_value(42);
```

The associated future immediately becomes ready.

---

### Setting an Exception

```cpp
promise.set_exception(
    std::current_exception()
);
```

The stored exception is re-thrown when `future.get()` is called.

---

## Warning: Broken Promise

If a promise is destroyed before calling either:

```cpp
set_value()
```

or

```cpp
set_exception()
```

the future automatically receives:

```cpp
std::future_error
```

with error code:

```cpp
std::future_errc::broken_promise
```

This indicates that the producer failed to fulfill its contract.

---

# 5. `std::shared_future<T>`

## Concept

A `std::shared_future` is the copyable version of `std::future`.

Multiple threads can wait for the same event and access the same result.

All copies refer to the same shared state.

---

## Creation

From an existing future:

```cpp
std::future<int> future =
    std::async(task);

std::shared_future<int> shared =
    future.share();
```

or

```cpp
std::shared_future<int> shared(
    std::move(future)
);
```

---

## Advantages

Unlike `std::future`:

```cpp
shared.get();
shared.get();
shared.get();
```

is valid.

The result remains available for all copies.

---

## Warning

Although a `shared_future` can be copied, the member functions of a single instance are not guaranteed to be thread-safe.

The recommended approach is:

```cpp
auto copy1 = shared;
auto copy2 = shared;
auto copy3 = shared;
```

Each thread keeps its own copy.

This minimizes synchronization requirements and avoids accidental data races.

---

# 6. Exception Handling in Futures

## Concept

Futures provide a built-in mechanism for transferring exceptions between threads.

If an asynchronous task throws an exception:

```cpp
throw std::runtime_error("Failure");
```

the exception is stored in the shared state.

When another thread calls:

```cpp
future.get();
```

the exception is automatically re-thrown.

---

## Using `std::promise`

```cpp
try
{
    promise.set_value(calculate());
}
catch (...)
{
    promise.set_exception(
        std::current_exception()
    );
}
```

The consumer thread receives the same exception when calling:

```cpp
future.get();
```

---

## Using `std::copy_exception`

When the exception object is already available, using:

```cpp
promise.set_exception(
    std::copy_exception(exception_object)
);
```

can produce cleaner code than wrapping logic in a generic `try-catch` block.

---

# Key Takeaways

| Tool                 | Purpose                                                  |
| -------------------- | -------------------------------------------------------- |
| `std::future`        | Exclusive ownership of an asynchronous result            |
| `std::async`         | Launch asynchronous tasks with automatic future creation |
| `std::packaged_task` | Separate task creation from task execution               |
| `std::promise`       | Manually provide a value or exception                    |
| `std::shared_future` | Share one asynchronous result among multiple consumers   |

---

# Summary

| Feature               | Description                                                         |
| --------------------- | ------------------------------------------------------------------- |
| `std::future`         | Unique ownership of a result                                        |
| `std::shared_future`  | Shared ownership of a result                                        |
| `std::async`          | Simplified asynchronous execution                                   |
| `std::packaged_task`  | Future-producing callable wrapper                                   |
| `std::promise`        | Manual producer-consumer synchronization                            |
| `get()`               | Wait and retrieve result                                            |
| `wait()`              | Wait without retrieving result                                      |
| Exception Propagation | Exceptions automatically cross thread boundaries                    |
| Broken Promise        | Generated when a promise is destroyed without fulfilling its future |
| One-Off Event         | Futures are designed for events that occur exactly once             |
