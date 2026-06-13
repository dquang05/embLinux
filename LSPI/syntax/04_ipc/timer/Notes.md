# Linux Timers and Sleeping Mechanisms

---

# I. Core Concepts: Timers and Sleeping

## Timer

A **timer** allows a process to schedule a future notification to itself at a specific point in time.

The notification may take the form of:

* A signal
* A POSIX timer event
* A Linux-specific file descriptor event

Timers are fundamental building blocks for:

* Timeouts
* Periodic tasks
* Scheduling
* Performance monitoring
* Event-driven systems

---

## Sleeping

A **sleeping mechanism** allows a process or thread to voluntarily relinquish CPU control and suspend execution for a specified period.

Benefits include:

* Reducing CPU usage
* Waiting for external events
* Implementing delays
* Creating periodic execution loops

Unlike busy-waiting, sleeping allows the scheduler to run other tasks efficiently.

---

# II. Architectural Evolution: Classical vs Modern Approaches

Time management facilities in UNIX and Linux evolved through two major generations, each with significantly different design characteristics.

---

# 1. Classical Branch (Signal-Driven Timers)

This branch contains the original UNIX timing APIs.

Today, these APIs are primarily encountered in:

* Educational materials
* Legacy applications
* Small utilities and scripts

They account for a relatively small portion of modern production systems.

---

## Operating Principle

The mechanism behaves similarly to a hardware interrupt on a microcontroller:

```text
Timer countdown starts
          ↓
Timer expires
          ↓
Kernel delivers a signal
          ↓
Normal execution is interrupted
          ↓
CPU jumps to a signal handler
```

The signal handler executes asynchronously with respect to the main program flow.

---

## Representative APIs

### `sleep()`

```c
unsigned int sleep(unsigned int seconds);
```

Suspends the process for a specified number of seconds.

If interrupted by a signal:

* The function returns early.
* The return value contains the remaining unslept time.

---

### `alarm()`

```c
unsigned int alarm(unsigned int seconds);
```

Schedules delivery of:

```text
SIGALRM
```

Characteristics:

* Real-time based
* One-shot timer
* Automatically disarmed after expiration

---

### `setitimer()`

```c
int setitimer(
    int which,
    const struct itimerval *new_value,
    struct itimerval *old_value);
```

Provides interval timers capable of periodic execution.

---

## Three Independent Timer Types

Each process can maintain up to three interval timers simultaneously.

### `ITIMER_REAL`

Counts elapsed wall-clock time.

Expiration signal:

```text
SIGALRM
```

---

### `ITIMER_VIRTUAL`

Counts only CPU time spent in user mode.

Expiration signal:

```text
SIGVTALRM
```

Useful for measuring application execution time while excluding kernel activity.

---

### `ITIMER_PROF`

Counts total CPU time:

* User mode
* Kernel mode

Expiration signal:

```text
SIGPROF
```

Frequently used by profilers and performance-analysis tools.

---

## Major Limitations

### Limited Timer Count

A process can own only:

```text
3 timer categories
1 timer instance per category
```

which severely restricts scalability.

---

### Low Resolution

Timers are based on:

```c
struct timeval
```

and are ultimately rounded according to the system timer tick (jiffy).

This limits timing precision.

---

### Overrun Information Loss

If a timer expires multiple times while its signal is blocked:

```text
Expiration #1
Expiration #2
Expiration #3
```

the signal handler may still execute only once.

Lost expirations cannot be recovered.

---

### Reentrancy Risks

Signal handlers execute asynchronously.

This can lead to:

* Data races
* Corrupted state
* Reentrancy bugs

if shared data is not carefully protected.

---

# 2. Modern Branch (POSIX.1b and Linux-Specific Timers)

Modern systems overwhelmingly prefer POSIX timer facilities.

They eliminate nearly all limitations of classical UNIX timers and dominate contemporary applications.

---

## High-Resolution Timers

Modern timers use:

```c
struct timespec
```

providing nanosecond granularity.

```c
struct timespec {
    time_t tv_sec;
    long   tv_nsec;
};
```

This enables timing accuracy much closer to hardware capabilities.

---

## Flexible Notification Mechanisms

Notification behavior is controlled through:

```c
struct sigevent
```

Applications may choose to:

* Deliver a signal
* Spawn a dedicated callback thread
* Use other notification methods

---

### `SIGEV_THREAD`

A particularly useful option is:

```text
SIGEV_THREAD
```

The kernel automatically creates a thread that executes a callback function.

Advantages:

* Avoids asynchronous signal-handler complexity
* Easier synchronization
* Safer programming model

---

## Timer Overrun Management

Modern POSIX timers provide:

```c
timer_getoverrun()
```

allowing applications to determine how many timer expirations were missed.

This solves the overrun visibility problem present in traditional signal-driven timers.

---

## Absolute Sleeping (`TIMER_ABSTIME`)

One of the most important modern timing features is absolute-time sleeping.

---

### The Oversleeping Problem

Relative sleeping functions such as:

```c
nanosleep()
```

can accumulate timing errors.

Example:

```text
Sleep 10 ms
Interrupted by signal
Resume sleep
Interrupted again
Resume sleep
...
```

Each restart may introduce rounding errors.

Over time:

```text
Expected Sleep Time
≠
Actual Sleep Time
```

The drift continues to accumulate.

---

### Solution: `clock_nanosleep()`

Using:

```c
clock_nanosleep()
```

with:

```text
TIMER_ABSTIME
```

allows sleeping until a fixed target time.

Example:

```text
Wake exactly at 10:00:00
```

Even if multiple interruptions occur:

* The target timestamp never changes.
* Accumulated timing drift is eliminated.

This technique is widely used in:

* Robotics
* Industrial control
* Real-time software
* Embedded Linux systems

---

# III. POSIX Clock System

POSIX defines several independent clocks that operate inside the kernel.

Applications choose the clock that best matches their timing requirements.

---

## `CLOCK_REALTIME`

The system's wall-clock time.

Characteristics:

* Represents actual calendar time.
* Can be modified manually.
* Can be adjusted automatically by NTP.

Suitable for:

* Timestamps
* Logging
* Scheduling based on real dates and times

---

## `CLOCK_MONOTONIC`

A continuously increasing clock that starts at system boot.

Characteristics:

* Never moves backward.
* Immune to manual time changes.
* Immune to NTP time jumps.

Recommended for:

* Measuring elapsed time
* Timeouts
* Periodic scheduling

---

## `CLOCK_MONOTONIC_RAW`

Provides direct access to the underlying hardware clock source.

Characteristics:

* Not adjusted by NTP.
* Represents the raw hardware counter.

Useful when maximum timing consistency is required.

---

## `CLOCK_PROCESS_CPUTIME_ID`

Measures CPU time consumed by the entire process.

Includes only actual CPU execution time.

Useful for:

* Profiling
* CPU accounting
* Performance analysis

---

## `CLOCK_THREAD_CPUTIME_ID`

Measures CPU time consumed by an individual thread.

Useful for:

* Thread profiling
* Performance debugging
* Scheduler analysis

---

# IV. Advanced Practical Usage: Linux-Specific `timerfd`

Large event-driven systems often find signals and callback threads difficult to integrate cleanly into their architecture.

Linux introduces a powerful alternative:

```text
timerfd
```

---

## Concept

`timerfd_create()` creates a timer and exposes it as a normal file descriptor.

```c
int timerfd_create(
    int clockid,
    int flags
);
```

The timer behaves similarly to:

* Files
* Pipes
* Sockets

from the application's perspective.

---

## Integration with I/O Multiplexing

Because a timer is represented by a file descriptor, it can participate directly in:

```c
select()
poll()
epoll()
```

This allows timers to be integrated into a unified event loop.

---

## Read Behavior

### Before Expiration

```c
read(fd, ...)
```

blocks until the timer expires.

---

### After Expiration

When one or more timer events occur:

```c
read(fd, &count, sizeof(count));
```

returns immediately.

The returned value is a:

```c
uint64_t
```

containing the number of expirations that have occurred.

Example:

```text
5
```

means the timer expired five times since the last successful read.

---

## Automatic Overrun Accounting

Unlike traditional signal-based timers:

```text
No expiration events are lost.
```

The returned counter includes:

* Normal expirations
* Missed expirations
* Overruns

making timerfd particularly reliable for event-driven systems.

---

# Key Takeaways

* Classical UNIX timers rely on asynchronous signal delivery and suffer from scalability and reliability limitations.
* POSIX timers provide high-resolution timing, flexible notifications, and overrun tracking.
* `CLOCK_MONOTONIC` is generally the safest clock source for measuring elapsed time.
* `TIMER_ABSTIME` eliminates accumulated timing drift caused by repeated interruptions.
* Linux `timerfd` transforms timers into file descriptors that integrate naturally with `select()`, `poll()`, and `epoll()`.
* Modern Linux applications overwhelmingly favor POSIX timers and `timerfd` over traditional signal-driven timer mechanisms.
