# Pthreads API and Implementation Details

This document covers the POSIX Threads (Pthreads) API, synchronization mechanisms, thread safety, and implementation details on Linux, based on *The Linux Programming Interface* (Chapters 29-33).

## 1. Introduction to Pthreads (Chapter 29)
- **Overview:** Pthreads is the POSIX standard for threading. Multiple threads within a process run concurrently, sharing the same virtual address space, global variables, heap, file descriptors, and signal dispositions. However, each thread has its own Thread ID, stack, `errno`, and signal mask.
- **Basic Thread Operations:**
  - **Creation:** `pthread_create()` spawns a new thread which starts executing the provided start function.
  - **Termination:** A thread terminates when it returns from its start function, calls `pthread_exit()`, or is canceled via `pthread_cancel()`. Note: If any thread calls `exit()`, the entire process terminates.
  - **Thread ID:** `pthread_self()` returns the calling thread's ID. The representation of `pthread_t` is implementation-dependent; it has no portable integer print format. Use `pthread_equal()` to compare IDs; the creation example prints an application-level index.
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
- **Reentrant Functions:** Can be re-entered while an earlier invocation is in progress without corrupting state. Caller-provided buffers (for example `strtok_r()` state) help, but do not prove thread safety if callers share mutable buffers or other state. Thread safety concerns concurrent calls; it can also be achieved with locking. Async-signal-safety is a separate API contract: a thread-safe function using locks is not necessarily safe in a signal handler. Ownership and all shared mutable state must be considered.
- **One-Time Initialization:** `pthread_once()` ensures that an initialization function (like dynamic mutex initialization inside a library) is executed exactly once, no matter how many threads attempt to call it.

## 4. Thread Cancellation (Chapter 32)
- **Cancellation:** `pthread_cancel()` sends a request to terminate a thread. The thread's response depends on its cancelability state and type.
- **Cancelability State:**
  - `PTHREAD_CANCEL_DISABLE`: Cancellation requests are ignored and remain pending.
  - `PTHREAD_CANCEL_ENABLE`: Cancellation is allowed (default).
- **Cancelability Type (when enabled):**
  - `PTHREAD_CANCEL_DEFERRED`: The thread is canceled only when it reaches a **Cancellation Point** (e.g., blocking functions like `sleep()`, `read()`, `pthread_join()`). This is the default, but resources and mutex ownership still require cleanup and consistent invariants.
  - `PTHREAD_CANCEL_ASYNCHRONOUS`: The thread can be canceled at any time. Rarely used as it is inherently unsafe for resource management.
- **Explicit Check:** `pthread_testcancel()` creates an artificial cancellation point for compute-bound threads that lack natural ones.
- **Cleanup Handlers:** `pthread_cleanup_push()` registers a handler; `pthread_cleanup_pop()` removes it and optionally executes it. Registered handlers run on cancellation or `pthread_exit()`. Condition waits reacquire their mutex before cancellation cleanup, so a suitable handler may need to unlock it. The finite producer-consumer example never requests cancellation and supplies no cancellation protocol.

## 5. Further Details (Chapter 33)
- **Thread Stacks:** On Linux/NPTL, a finite `RLIMIT_STACK` at program startup determines the default new-thread stack size; an unlimited setting uses an architecture-dependent default. Libc and target configuration matter. Use `pthread_attr_setstacksize()` to request a supported size.
- **Threads and Signals:** A highly complex interaction.
  - Signal actions and dispositions are process-wide.
  - Signal masks are per-thread (manipulated via `pthread_sigmask()`).
  - Thread-directed signals can be sent using `pthread_kill()` or `pthread_sigqueue()`.
  - A process-directed signal goes to one eligible thread; thread-directed signals target a particular thread. `SIGKILL` and `SIGSTOP` cannot be blocked. An optional design blocks a selected blockable set before worker creation and dedicates a `sigwait()` thread to that set.
- **Process Control:**
  - `exec()`: Immediately destroys all other threads except the calling thread.
  - `fork()`: Only the calling thread exists in the Linux/POSIX child. Copied synchronization state may retain locks associated with vanished threads. Until `exec`, the child is restricted to applicable async-signal-safe operations. `pthread_atfork()` can help selected state but cannot universally repair inherited library/synchronization state.
  - `exit()`: Terminates the entire process immediately.
- **Linux Threading Implementations:**
  - **LinuxThreads (Obsolete):** Built over `clone()`, used a manager thread, and relied heavily on signals. It violated many SUSv3 standards (e.g., threads had different PIDs).
  - **NPTL (Native POSIX Threads Library):** The modern 1:1 implementation introduced in Linux 2.6. Highly compliant with SUSv3, leverages the `CLONE_THREAD` flag so all threads share the same PID, with capacity depending on user/task/PID limits, available virtual memory, stack sizes and target configuration. There is no universal 100,000-thread guarantee. No manager thread is needed.
