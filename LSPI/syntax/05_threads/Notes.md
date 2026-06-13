# POSIX Threads (Pthreads): Theory, Synchronization, and Thread Lifecycle

---

# Part I. Thread Fundamentals and Lifecycle

## 1. What Is a Thread?

A **thread** is the smallest execution unit that can be scheduled by the operating system.

A single process may contain multiple threads executing concurrently within the same program.

### Shared Resources (Process-Wide)

All threads belonging to the same process share:

* Virtual address space

  * Global variables (`.data`)
  * Static variables
  * Uninitialized variables (`.bss`)
  * Heap memory
* Process ID (PID)
* Parent Process ID (PPID)
* Open file descriptors
* Signal dispositions
* Current working directory
* User ID (UID)
* Group ID (GID)

Because memory is shared, communication between threads is significantly faster than communication between separate processes.

### Private Resources (Per Thread)

Each thread owns its own execution context:

* Thread ID (TID)
* Stack memory
* CPU register set
* Program Counter (PC)
* Thread-local `errno`
* Signal mask
* Scheduling policy and priority

The private stack contains:

* Local variables
* Function call frames
* Return addresses

---

# 2. Thread Lifecycle Management

The basic lifecycle of a thread is:

```text
Create
   ↓
Running
   ↓
Exit
   ↓
Join or Automatic Cleanup
```

---

## Creating a Thread: `pthread_create()`

```c
#include <pthread.h>

int pthread_create(
    pthread_t *thread,
    const pthread_attr_t *attr,
    void *(*start_routine)(void *),
    void *arg
);
```

### Parameters

#### `thread`

Pointer to a variable that will receive the newly created thread ID.

#### `attr`

Thread attribute object.

Examples:

* Stack size
* Scheduling policy
* Detached state

Pass:

```c
NULL
```

to use default settings.

#### `start_routine`

Entry point of the thread.

Prototype:

```c
void *thread_function(void *arg);
```

#### `arg`

Single argument passed to the thread function.

Can point to any data type.

---

## Exiting a Thread: `pthread_exit()`

```c
#include <pthread.h>

void pthread_exit(void *retval);
```

### Parameter

#### `retval`

Pointer to the thread's return value.

This value can later be collected through:

```c
pthread_join()
```

### Important Warning

Never return the address of a local stack variable:

```c
void *worker(void *arg)
{
    int result = 42;

    pthread_exit(&result);    /* WRONG */
}
```

The thread's stack is destroyed immediately after termination.

The pointer becomes invalid.

---

## Waiting for a Thread: `pthread_join()`

```c
#include <pthread.h>

int pthread_join(
    pthread_t thread,
    void **retval
);
```

### Parameters

#### `thread`

Target thread ID.

#### `retval`

Receives the value returned by:

```c
pthread_exit()
```

or:

```c
return value;
```

Pass:

```c
NULL
```

if the return value is not needed.

### Purpose

* Wait for thread termination.
* Retrieve thread results.
* Release thread resources.

Failure to join a terminated joinable thread causes resource leakage.

Such threads are commonly referred to as:

```text
Zombie Threads
```

---

## Detaching a Thread: `pthread_detach()`

```c
#include <pthread.h>

int pthread_detach(pthread_t thread);
```

### Parameter

#### `thread`

Target thread ID.

### Behavior

Once detached:

* The thread executes normally.
* The operating system automatically releases its resources after termination.
* No `pthread_join()` is required.

Detached threads cannot be joined.

---

# Part II. Data Synchronization with Mutexes

## 1. The Race Condition Problem

A race condition occurs when multiple threads simultaneously access shared data and at least one thread performs a write operation.

Example:

```c
global++;
```

Although it appears as a single statement, it actually consists of:

```text
Read
  ↓
Modify
  ↓
Write
```

If another thread interrupts execution between these steps, updates may be lost.

The code region accessing shared data is called a:

```text
Critical Section
```

---

## 2. Mutex Fundamentals

A mutex guarantees that only one thread at a time can enter a critical section.

Typical workflow:

```text
Lock
  ↓
Access Shared Data
  ↓
Unlock
```

---

## Initializing and Destroying a Mutex

```c
int pthread_mutex_init(
    pthread_mutex_t *mutex,
    const pthread_mutexattr_t *attr
);

int pthread_mutex_destroy(
    pthread_mutex_t *mutex
);
```

### Parameters

#### `mutex`

Mutex object.

#### `attr`

Mutex attributes.

Pass:

```c
NULL
```

for default behavior.

A mutex must not be destroyed while locked.

---

## Locking and Unlocking

```c
int pthread_mutex_lock(
    pthread_mutex_t *mutex
);

int pthread_mutex_trylock(
    pthread_mutex_t *mutex
);

int pthread_mutex_unlock(
    pthread_mutex_t *mutex
);
```

### `pthread_mutex_lock()`

Blocks until the mutex becomes available.

### `pthread_mutex_trylock()`

Does not block.

Returns:

```text
EBUSY
```

if the mutex is already locked.

Useful for deadlock prevention.

### `pthread_mutex_unlock()`

Releases the mutex.

Only the owner thread may unlock it.

---

## Mutex Types

### NORMAL (Default)

Behavior:

```text
Thread locks mutex twice
        ↓
Permanent self-deadlock
```

---

### ERRORCHECK

Detects misuse.

Examples:

* Locking twice
* Unlocking from another thread

Returns an error immediately.

Very useful during debugging.

---

### RECURSIVE

Allows the same thread to acquire the mutex multiple times.

An internal counter tracks lock ownership.

The mutex becomes available only when the counter reaches zero.

---

## Deadlock Prevention

Deadlock example:

```text
Thread A owns Lock 1
         waits for Lock 2

Thread B owns Lock 2
         waits for Lock 1
```

Both threads wait forever.

---

### Lock Hierarchy

Define a strict lock acquisition order.

Example:

```text
Lock 1
   ↓
Lock 2
```

Every thread must follow the same order.

---

### Try-and-Backoff Strategy

```text
Acquire Lock 1
       ↓
Try Lock 2
       ↓
Fail?
       ↓
Release Lock 1
       ↓
Sleep briefly
       ↓
Retry
```

Implemented using:

```c
pthread_mutex_trylock()
```

---

# Part III. Condition Variables

Using only mutexes often leads to:

```text
Busy Waiting
```

where a thread repeatedly checks a condition while wasting CPU time.

Condition variables eliminate this problem.

---

## Initialization and Destruction

```c
int pthread_cond_init(
    pthread_cond_t *cond,
    const pthread_condattr_t *attr
);

int pthread_cond_destroy(
    pthread_cond_t *cond
);
```

---

## Principle of Operation

A condition variable must always be associated with a mutex.

A waiting thread:

* Releases the CPU completely.
* Sleeps until another thread signals it.

---

## Waiting for a Condition

```c
int pthread_cond_wait(
    pthread_cond_t *cond,
    pthread_mutex_t *mutex
);
```

### Atomic Behavior

This function performs three operations atomically:

```text
Unlock Mutex
       ↓
Sleep on Condition Variable
       ↓
Wake Up
       ↓
Relock Mutex
```

---

## The Mandatory `while` Rule

Always write:

```c
while (!condition)
{
    pthread_cond_wait(&cond, &mutex);
}
```

Never:

```c
if (!condition)
{
    pthread_cond_wait(&cond, &mutex);
}
```

### Reason 1: Stolen State

Another thread may modify shared data before the awakened thread reacquires the mutex.

### Reason 2: Spurious Wakeups

Threads may occasionally wake up without receiving any signal.

This behavior is allowed by POSIX.

---

## Signaling

```c
int pthread_cond_signal(
    pthread_cond_t *cond
);

int pthread_cond_broadcast(
    pthread_cond_t *cond
);
```

### `pthread_cond_signal()`

Wakes at least one waiting thread.

### `pthread_cond_broadcast()`

Wakes all waiting threads.

---

# Part IV. Thread Safety and Thread-Local Data

## 1. Thread-Safe and Reentrant Functions

A function is **thread-safe** if multiple threads can execute it concurrently without causing incorrect behavior.

The most robust approach is writing:

```text
Reentrant Functions
```

Characteristics:

* No global variables
* No static mutable variables
* All state passed through parameters
* Results stored in thread-local or stack memory

---

## One-Time Initialization: `pthread_once()`

```c
int pthread_once(
    pthread_once_t *once_control,
    void (*init_routine)(void)
);
```

### Parameters

#### `once_control`

Must be initialized with:

```c
PTHREAD_ONCE_INIT
```

#### `init_routine`

Initialization function.

### Purpose

Guarantees the initialization code executes exactly once during the entire process lifetime.

Common uses:

* Mutex initialization
* Library initialization
* Singleton objects

---

## Thread-Specific Data (TSD)

TSD allows each thread to associate its own private data with a shared key.

---

### Creating a Key

```c
int pthread_key_create(
    pthread_key_t *key,
    void (*destructor)(void *)
);
```

### Setting Thread Data

```c
int pthread_setspecific(
    pthread_key_t key,
    const void *value
);
```

### Retrieving Thread Data

```c
void *pthread_getspecific(
    pthread_key_t key
);
```

### Destructor

When a thread terminates:

```text
Thread Exit
      ↓
Destructor Called
      ↓
Memory Released
```

This mechanism was historically used to make older libraries thread-safe.

---

## Thread-Local Storage (TLS)

A modern alternative to TSD.

Example:

```c
__thread int thread_counter;
```

Each thread automatically receives its own independent copy.

Advantages:

* Simpler syntax
* Better performance
* Compiler support

---

# Part V. Safe Thread Cancellation

## 1. Sending a Cancellation Request

```c
int pthread_cancel(
    pthread_t thread
);
```

### Parameter

#### `thread`

Target thread ID.

### Behavior

The function only sends a cancellation request and returns immediately.

Actual termination depends on the target thread's cancellation settings.

---

## 2. Cancellation State and Type

```c
int pthread_setcancelstate(
    int state,
    int *oldstate
);

int pthread_setcanceltype(
    int type,
    int *oldtype
);
```

---

### Cancellation State

#### Enable Cancellation

```c
PTHREAD_CANCEL_ENABLE
```

#### Disable Cancellation

```c
PTHREAD_CANCEL_DISABLE
```

Useful during critical operations.

---

### Cancellation Type

#### Deferred (Recommended)

```c
PTHREAD_CANCEL_DEFERRED
```

Cancellation occurs only at defined cancellation points.

---

#### Asynchronous (Dangerous)

```c
PTHREAD_CANCEL_ASYNCHRONOUS
```

The thread may be terminated at any instruction.

Risks:

* Heap corruption
* Locked mutexes
* Resource leaks

---

## 3. Cancellation Points

Examples:

```text
sleep()
read()
waitpid()
pthread_cond_wait()
```

These functions allow pending cancellation requests to take effect.

---

### Artificial Cancellation Point

```c
void pthread_testcancel(void);
```

Useful inside CPU-bound loops:

```c
while (1)
{
    compute();

    pthread_testcancel();
}
```

Without cancellation points, a compute-only thread may never terminate.

---

## 4. Cleanup Handlers

Cleanup handlers guarantee resource release during cancellation.

---

### Registering a Cleanup Handler

```c
pthread_cleanup_push(
    void (*routine)(void *),
    void *arg
);
```

### Removing a Cleanup Handler

```c
pthread_cleanup_pop(
    int execute
);
```

---

### Parameters

#### `routine`

Cleanup function.

Examples:

* `free()`
* Mutex unlock wrapper
* File cleanup function

#### `arg`

Argument passed to the cleanup function.

#### `execute`

```text
0  → Remove only
>0 → Remove and execute
```

---

### Cleanup Stack Behavior

Cleanup handlers form a stack:

```text
Push A
Push B
Push C

Cancellation

Execute C
Execute B
Execute A
```

Order:

```text
LIFO
(Last In, First Out)
```

---

### Important Rule

`pthread_cleanup_push()` and `pthread_cleanup_pop()` must always appear as a matching pair within the same lexical block.

```c
{
    pthread_cleanup_push(...);

    /* protected code */

    pthread_cleanup_pop(1);
}
```

This guarantees proper resource cleanup when a thread is cancelled unexpectedly.

---

# Key Takeaways

* Threads share a process's memory space but maintain independent execution contexts.
* Mutexes protect critical sections and prevent race conditions.
* Condition variables eliminate busy waiting and provide efficient synchronization.
* Always use a `while` loop around `pthread_cond_wait()`.
* `pthread_once()` guarantees one-time initialization.
* Thread-Specific Data (TSD) and Thread-Local Storage (TLS) provide per-thread storage.
* Thread cancellation should typically use deferred mode.
* Cleanup handlers prevent memory leaks and deadlocks when threads terminate unexpectedly.
* Proper synchronization is essential for writing reliable multithreaded applications.
