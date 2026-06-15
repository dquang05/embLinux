# Time-Based Waiting and Timeouts in Modern C++

## Overview

In concurrent programming, waiting indefinitely is often undesirable. A thread may need to stop waiting if an event does not occur within a specified period.

Modern C++ provides a comprehensive time management framework through the `<chrono>` library, allowing timeouts to be expressed safely and portably.

---

# 1. Time-Based Waiting Mechanisms

There are two common ways to specify a timeout.

## Duration-Based Timeout (`_for`)

Wait for a specific amount of time.

Examples:

```cpp
std::this_thread::sleep_for(
    std::chrono::milliseconds(30)
);
```

```cpp
future.wait_for(
    std::chrono::seconds(1)
);
```

### Characteristics

* Relative timeout.
* Expressed as a duration from the current moment.
* Uses the `_for` suffix.

---

## Absolute Timeout (`_until`)

Wait until a specific point in time.

Examples:

```cpp
auto deadline =
    std::chrono::steady_clock::now() +
    std::chrono::seconds(5);

future.wait_until(deadline);
```

### Characteristics

* Absolute timeout.
* Expressed as a specific deadline.
* Uses the `_until` suffix.

---

# 2. Time Management Components (`<chrono>`)

The `<chrono>` library provides three fundamental concepts:

1. Clocks
2. Durations
3. Time Points

---

# A. Clocks

A clock provides a source of time.

---

## `std::chrono::steady_clock`

### Concept

A monotonic clock whose time always moves forward at a constant rate.

```cpp
std::chrono::steady_clock
```

### Characteristics

* Never goes backward.
* Not affected by changes to system time.
* Ideal for measuring elapsed time.
* Recommended for implementing timeouts.

Example:

```cpp
auto start =
    std::chrono::steady_clock::now();
```

---

## `std::chrono::system_clock`

### Concept

Represents the system's real-world clock.

```cpp
std::chrono::system_clock
```

### Characteristics

* Represents calendar time.
* Can be modified by:

  * User settings
  * NTP synchronization
  * Operating system updates

### Limitation

Not recommended for timeout calculations because clock adjustments can shorten or extend waiting periods unexpectedly.

---

## `std::chrono::high_resolution_clock`

### Concept

Provides the highest available clock precision on a platform.

```cpp
std::chrono::high_resolution_clock
```

### Characteristics

* Highest available resolution.
* Often implemented as either:

  * `steady_clock`
  * `system_clock`

depending on the platform.

---

# B. Durations

## Concept

A duration represents a length of time.

---

## Syntax

```cpp
std::chrono::duration<Rep, Period>
```

### Parameters

| Parameter | Meaning                     |
| --------- | --------------------------- |
| `Rep`     | Numeric representation type |
| `Period`  | Tick period (frequency)     |

---

## Common Duration Types

```cpp
std::chrono::nanoseconds
std::chrono::microseconds
std::chrono::milliseconds
std::chrono::seconds
std::chrono::minutes
std::chrono::hours
```

Example:

```cpp
auto timeout =
    std::chrono::milliseconds(500);
```

---

## Duration Conversion

Use:

```cpp
std::chrono::duration_cast<Target>(source)
```

Example:

```cpp
auto ms =
    std::chrono::duration_cast<
        std::chrono::milliseconds
    >(duration);
```

---

# C. Time Points

## Concept

A time point represents a specific moment on a clock's timeline.

It is measured relative to the clock's epoch.

---

## Syntax

```cpp
std::chrono::time_point<
    Clock,
    Duration
>
```

---

## Obtaining Current Time

```cpp
auto now =
    std::chrono::steady_clock::now();
```

---

## Calculating Future Deadlines

A duration can be added to a time point:

```cpp
auto deadline =
    std::chrono::steady_clock::now() +
    std::chrono::seconds(5);
```

This creates a deadline five seconds in the future.

---

# 3. Using Timeouts in Thread Programming

## Condition Variables

Timeouts prevent threads from waiting forever.

Recommended pattern:

```cpp
auto timeout =
    std::chrono::steady_clock::now() +
    std::chrono::milliseconds(500);

std::unique_lock<std::mutex> lk(m);

while (!done)
{
    if (cv.wait_until(lk, timeout) ==
        std::cv_status::timeout)
    {
        break;
    }
}
```

### Why Use `wait_until()`?

Because condition variables may experience:

* Spurious wakeups
* Multiple notifications
* Delayed scheduling

Using a fixed deadline prevents timeout accumulation caused by repeated wakeups.

---

## Sleeping Threads

### Sleep for a Duration

```cpp
std::this_thread::sleep_for(
    std::chrono::seconds(1)
);
```

---

### Sleep Until a Deadline

```cpp
std::this_thread::sleep_until(
    deadline
);
```

---

## Futures

### Wait for a Duration

```cpp
future.wait_for(
    std::chrono::seconds(1)
);
```

---

### Wait Until a Deadline

```cpp
future.wait_until(
    deadline
);
```

---

## Timed Mutexes

Standard mutexes do not support timeouts.

Use:

```cpp
std::timed_mutex
```

or

```cpp
std::recursive_timed_mutex
```

### Wait for a Duration

```cpp
mutex.try_lock_for(
    std::chrono::milliseconds(100)
);
```

---

### Wait Until a Deadline

```cpp
mutex.try_lock_until(deadline);
```

---

# 4. Timeout Parameter Summary

| Parameter Type | C++ Type                    | Purpose                |
| -------------- | --------------------------- | ---------------------- |
| Duration       | `std::chrono::duration<>`   | Length of waiting time |
| Time Point     | `std::chrono::time_point<>` | Absolute deadline      |

---

# 5. Important Warnings

## Spurious Wakeups

Condition variables may wake up without receiving a notification.

Incorrect:

```cpp
cv.wait_until(lock, deadline);
```

Correct:

```cpp
while (!condition)
{
    if (cv.wait_until(lock, deadline) ==
        std::cv_status::timeout)
    {
        break;
    }
}
```

Always re-check the condition after waking.

---

## Timeout Accuracy

Timeout values are not guaranteed to be exact.

Actual waiting time may be longer due to:

* Operating system scheduling
* Context switching
* Hardware timer resolution
* System load

Example:

```cpp
sleep_for(100ms);
```

may actually sleep for:

```text
101ms
103ms
110ms
```

depending on the platform.

---

## System Clock Adjustments

When using:

```cpp
std::chrono::system_clock
```

for timeout calculations, system time changes can affect waiting behavior.

Examples:

* NTP synchronization
* Manual clock adjustment
* Time zone corrections

These events may cause waits to:

* End too early
* Last longer than expected

---

## Prefer `steady_clock` for Timeouts

Recommended:

```cpp
std::chrono::steady_clock
```

because:

* Monotonic
* Immune to clock adjustments
* Reliable for measuring elapsed time

Example:

```cpp
auto deadline =
    std::chrono::steady_clock::now() +
    std::chrono::seconds(5);
```

This is the preferred way to implement timeouts in concurrent applications.

---

# Choosing the Right Clock

| Clock                   | Use Case                         | Recommended for Timeouts    |
| ----------------------- | -------------------------------- | --------------------------- |
| `steady_clock`          | Measuring elapsed time, timeouts | Yes                       |
| `system_clock`          | Calendar time, timestamps        | No                        |
| `high_resolution_clock` | High-precision measurements      | Depends on implementation |

---

# Key Takeaways

| Feature            | Purpose                                                       |
| ------------------ | ------------------------------------------------------------- |
| `wait_for()`       | Wait for a relative duration                                  |
| `wait_until()`     | Wait until an absolute deadline                               |
| `sleep_for()`      | Suspend thread for a duration                                 |
| `sleep_until()`    | Suspend thread until a deadline                               |
| `try_lock_for()`   | Attempt mutex lock for a duration                             |
| `try_lock_until()` | Attempt mutex lock until a deadline                           |
| `steady_clock`     | Preferred clock for timeouts                                  |
| `system_clock`     | Real-world system time                                        |
| `duration`         | Represents a time interval                                    |
| `time_point`       | Represents a specific moment in time                          |
| Spurious Wakeup    | Requires condition re-checking                                |
| Clock Adjustment   | Can invalidate timeout calculations when using `system_clock` |
