# C++ Concurrency: Thread Lifecycle and Management

## 1. Thread Lifecycle and Launching Threads

Every C++ program always starts with at least one thread: the thread executing the `main()` function. When a new thread is created using `std::thread`, it runs concurrently with the main thread until its entry function returns.

`std::thread` is highly flexible and accepts any **callable object** as the thread entry point. There are three common ways to launch a thread:

```cpp
#include <iostream>
#include <thread>

// Method 1: Plain function
void do_some_work() {
    /* Code... */
}

// Method 2: Function Object (Functor)
class BackgroundTask {
public:
    void operator()() const {
        /* Code... */
    }
};

int main() {
    // Launch using a regular function
    std::thread t1(do_some_work);

    // Avoid the "Most Vexing Parse" issue:
    // Incorrect: std::thread t2(BackgroundTask());
    // The compiler may interpret it as a function declaration.
    // Correct: use uniform initialization.
    std::thread t2{BackgroundTask()};

    // Method 3: Lambda Expression (most convenient)
    std::thread t3([]() {
        std::cout << "Running from lambda!\n";
    });

    t1.join();
    t2.join();
    t3.join();
}
```

---

## 2. The Critical Decision: `join()` or `detach()`

Immediately after a thread is launched, you must decide whether to:

* **Wait for its completion** using `join()`
* **Allow it to run independently** using `detach()`

This decision must be made before the corresponding `std::thread` object is destroyed. Otherwise, `std::terminate()` will be invoked, causing the application to abort immediately.

### Dangling Reference Hazard with `detach()`

When `detach()` is called, the thread becomes completely independent and its lifetime is managed by the C++ runtime.

A detached thread must never access local variables belonging to a parent function that may already have returned. If it does, those variables may have been destroyed, resulting in **undefined behavior (UB)**.

```cpp
void dangerous() {
    int value = 42;

    std::thread t([&]() {
        // Potentially dangerous
        std::cout << value << std::endl;
    });

    t.detach();
} // value is destroyed here
```

If the detached thread executes after `value` has been destroyed, the program behavior becomes unpredictable.

---

## 3. Thread Safety Through RAII: `scoped_thread`

A common problem occurs when an exception is thrown after a thread starts but before `join()` is called. The function exits prematurely, skipping the `join()` call and causing program termination.

To solve this, the book introduces the **RAII (Resource Acquisition Is Initialization)** pattern. By encapsulating the thread inside a management object, `join()` can be automatically executed inside the destructor.

### Improved `scoped_thread` Implementation

```cpp
#include <thread>
#include <stdexcept>

class scoped_thread {
    std::thread t;

public:
    // Acquire thread ownership via move semantics
    explicit scoped_thread(std::thread t_)
        : t(std::move(t_))
    {
        if (!t.joinable()) {
            throw std::logic_error("No thread (Not joinable)");
        }
    }

    // Automatically join when leaving scope
    ~scoped_thread() {
        t.join();
    }

    // Prevent copying
    scoped_thread(scoped_thread const&) = delete;
    scoped_thread& operator=(scoped_thread const&) = delete;
};
```

### Benefits

* Guarantees `join()` execution.
* Exception-safe.
* Prevents accidental ownership sharing.
* Follows modern C++ resource management principles.

---

## 4. Argument Passing and Move Semantics

### Implicit Copy Trap

By default, arguments passed to `std::thread` are copied into internal thread storage before being passed to the target function.

This behavior can lead to subtle bugs.

### Dangling Pointer Example

```cpp
void process(std::string str);

char buffer[1024];
std::thread t(process, buffer);
```

The thread stores only the pointer initially. If `buffer` is destroyed before conversion to `std::string` occurs, undefined behavior may result.

**Safe approach:**

```cpp
std::thread t(process, std::string(buffer));
```

The conversion occurs immediately in the caller thread.

### Lost Reference Problem

```cpp
void update(Data& data);

Data d;
std::thread t(update, d);
```

Although `update()` expects a reference, `std::thread` creates a copy of `d`.

As a result, modifications affect only the copied object.

**Correct solution:**

```cpp
std::thread t(update, std::ref(d));
```

`std::ref()` explicitly instructs `std::thread` to pass a reference wrapper.

---

### Thread Ownership Transfer

`std::thread` behaves similarly to `std::unique_ptr`:

* Non-copyable
* Movable

Ownership can be transferred using `std::move()`.

```cpp
std::thread t1(do_some_work);

std::thread t2 = std::move(t1);
```

After the move:

* `t2` owns and manages the thread.
* `t1` becomes empty and no longer represents a thread.

This property enables:

* Returning threads from functions
* Dynamic thread ownership management
* RAII wrapper implementations

---

## 5. Run-Time Thread Management

### `std::thread::hardware_concurrency()`

This function returns the number of threads that can potentially execute concurrently on the current hardware.

```cpp
unsigned int n =
    std::thread::hardware_concurrency();
```

Typical results:

| CPU Configuration      | Returned Value |
| ---------------------- | -------------- |
| 4-Core CPU             | 4              |
| 4-Core / 8-Thread CPU  | 8              |
| 8-Core / 16-Thread CPU | 16             |

### Oversubscription

Creating significantly more threads than available hardware execution units may degrade performance.

Excessive thread counts cause:

* Frequent context switching
* Increased scheduling overhead
* Cache inefficiency
* Reduced throughput

For CPU-bound workloads, thread counts should generally be based on the value returned by `hardware_concurrency()`.

The book demonstrates this concept through a `parallel_accumulate` implementation, which partitions data into blocks according to the available hardware resources.

---

## 6. Thread Identification with `std::thread::id`

Each thread has a unique identifier represented by `std::thread::id`.

### Obtaining a Thread ID

From a thread object:

```cpp
std::thread t(do_some_work);

std::thread::id id = t.get_id();
```

From the currently executing thread:

```cpp
std::thread::id id =
    std::this_thread::get_id();
```

### Common Uses

* Logging and debugging
* Mapping work to specific threads
* Master/worker thread distinction
* Thread-local resource management

Since `std::thread::id` supports comparison operators (`==`, `<`, etc.), it can be used as a key in associative containers:

```cpp
std::map<std::thread::id, std::string> thread_names;

thread_names[std::this_thread::get_id()] =
    "Worker Thread";
```

This allows applications to assign specialized responsibilities or metadata to individual threads.

---

## Key Takeaways

* Every C++ application starts with the main thread and may create additional concurrent threads using `std::thread`.
* A thread must eventually be either **joined** or **detached**.
* Failing to do so before destruction triggers `std::terminate()`.
* Detached threads must never access destroyed objects or local variables.
* RAII wrappers such as `scoped_thread` provide exception-safe thread management.
* `std::thread` copies arguments by default; use `std::ref()` for references.
* Thread objects are movable but not copyable.
* `hardware_concurrency()` helps determine an efficient thread count.
* Each thread possesses a unique `std::thread::id` that can be used for identification, debugging, and workload management.
