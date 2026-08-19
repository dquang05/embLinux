# Pthreads API and Implementation Details

This document covers the POSIX Threads (Pthreads) API, synchronization mechanisms, thread safety, and implementation details on Linux, based on *The Linux Programming Interface* (Chapters 29-33).

## 1. Introduction to Pthreads (Chapter 29)
- **Overview:** Pthreads is the POSIX standard for threading. Multiple threads within a process run concurrently, sharing the same virtual address space, global variables, heap, file descriptors, and signal dispositions. However, each thread has its own Thread ID, stack, `errno`, and signal mask.
- **Basic Thread Operations:**
  - **Creation:** `pthread_create()` spawns a new thread which starts executing the provided start function.
  - **Termination:** A thread terminates when it returns from its start function, calls `pthread_exit()`, or is canceled via `pthread_cancel()`. Note: If any thread calls `exit()`, the entire process terminates.
  - **Thread ID:** `pthread_self()` returns the calling thread's ID. Use `pthread_equal()` to compare two thread IDs.
  - **Joining:** `pthread_join()` blocks until the specified thread terminates, retrieving its return value. Unless detached, threads must be joined to avoid zombie threads.
  - **Detaching:** `pthread_detach()` marks a thread as detached. When it terminates, the system automatically reclaims its resources without requiring a join. A detached thread cannot be joined.

## 2. Thread Synchronization (Chapter 30)
- **Mutexes:** 
  - Protects critical sections.
  - Initialized statically via `PTHREAD_MUTEX_INITIALIZER` or dynamically via `pthread_mutex_init()`. Cleaned up with `pthread_mutex_destroy()`.
  - Used via `pthread_mutex_lock()` and `pthread_mutex_unlock()`. Variants include `pthread_mutex_trylock()` (fails with `EBUSY` instead of blocking) and `pthread_mutex_timedlock()` (blocks with a timeout).
  - **Mutex Types:** Normal (no deadlock check), Errorcheck (returns an error if the thread already owns the lock), Recursive (allows multiple locks by the same thread), and Default.
  - **Deadlock Avoidance:** Establish a strict locking hierarchy (lock ordering) or use a "try and back off" strategy to prevent deadlocks when locking multiple mutexes.
- **Condition Variables:**
  - Allows a thread to sleep (wait) until another thread signals that the state of a shared variable has changed. Always used in conjunction with a mutex.
  - Initialized with `PTHREAD_COND_INITIALIZER` or `pthread_cond_init()`.
  - **Waiting:** `pthread_cond_wait()` atomically unlocks the mutex and blocks the thread. Upon waking, it reacquires the mutex. It must always be placed inside a `while` loop to re-check the condition due to the possibility of *spurious wakeups*.
  - **Signaling:** `pthread_cond_signal()` wakes up at least one waiting thread, while `pthread_cond_broadcast()` wakes up all waiting threads.

## 3. Thread Safety and Per-Thread Storage (Chapter 31)
- **Thread Safety:** A function is thread-safe if it can be safely called from multiple threads concurrently. Non-thread-safe functions typically rely on shared global or static variables.
- **Reentrant Functions:** Achieve thread safety without mutexes by avoiding global/static variables entirely. Instead, the caller provides buffers. POSIX provides reentrant versions of many standard functions, suffixed with `_r` (e.g., `strtok_r()`).
- **One-Time Initialization:** `pthread_once()` ensures that an initialization function (like dynamic mutex initialization inside a library) is executed exactly once, no matter how many threads attempt to call it.

## 4. Thread Cancellation (Chapter 32)
- **Cancellation:** `pthread_cancel()` sends a request to terminate a thread. The thread's response depends on its cancelability state and type.
- **Cancelability State:**
  - `PTHREAD_CANCEL_DISABLE`: Cancellation requests are ignored and remain pending.
  - `PTHREAD_CANCEL_ENABLE`: Cancellation is allowed (default).
- **Cancelability Type (when enabled):**
  - `PTHREAD_CANCEL_DEFERRED`: The thread is canceled only when it reaches a **Cancellation Point** (e.g., blocking functions like `sleep()`, `read()`, `pthread_join()`). This is the default and safe type.
  - `PTHREAD_CANCEL_ASYNCHRONOUS`: The thread can be canceled at any time. Rarely used as it is inherently unsafe for resource management.
- **Explicit Check:** `pthread_testcancel()` creates an artificial cancellation point for compute-bound threads that lack natural ones.
- **Cleanup Handlers:** `pthread_cleanup_push()` and `pthread_cleanup_pop()` register functions that automatically execute when a thread is canceled or calls `pthread_exit()`. They are critical for releasing resources like locked mutexes before the thread dies.

## 5. Further Details (Chapter 33)
- **Thread Stacks:** The default stack size (usually 2MB on x86-32) can be altered using `pthread_attr_setstacksize()`.
- **Threads and Signals:** A highly complex interaction.
  - Signal actions and dispositions are process-wide.
  - Signal masks are per-thread (manipulated via `pthread_sigmask()`).
  - Thread-directed signals can be sent using `pthread_kill()` or `pthread_sigqueue()`.
  - **Best Practice:** Block all asynchronous signals in all threads and dedicate a single thread to wait for signals synchronously using `sigwait()`.
- **Process Control:**
  - `exec()`: Immediately destroys all other threads except the calling thread.
  - `fork()`: Duplicates only the calling thread into the child process. This is extremely dangerous in multithreaded applications as mutexes held by vanished threads will remain locked permanently. `fork()` should typically be followed immediately by `exec()`. `pthread_atfork()` can be used to manage state around a fork.
  - `exit()`: Terminates the entire process immediately.
- **Linux Threading Implementations:**
  - **LinuxThreads (Obsolete):** Built over `clone()`, used a manager thread, and relied heavily on signals. It violated many SUSv3 standards (e.g., threads had different PIDs).
  - **NPTL (Native POSIX Threads Library):** The modern 1:1 implementation introduced in Linux 2.6. Highly compliant with SUSv3, leverages the `CLONE_THREAD` flag so all threads share the same PID, and scales exceptionally well (can manage up to 100,000 threads). No manager thread is needed.
