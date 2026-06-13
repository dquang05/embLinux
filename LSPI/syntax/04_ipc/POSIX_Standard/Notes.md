# POSIX IPC: Practical Usage, Runtime Pitfalls

---

# 1. Practical Importance and Real-World Applications (Core Knowledge)

In industrial software systems, selecting the appropriate IPC (Inter-Process Communication) mechanism directly impacts system performance, scalability, and reliability.

POSIX IPC is generally preferred over System V IPC because:

* It follows the UNIX file I/O model consistently.
* Most resources are accessed through file descriptors.
* It provides a cleaner API design.
* Resource management benefits from reference-counting semantics.

## POSIX IPC API Summary

| Mechanism      | Header          | Create / Open              | Close / Destroy                | Data Transfer / Control     | Typical Real-World Usage                                                                                                                                                                            |
| -------------- | --------------- | -------------------------- | ------------------------------ | --------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Shared Memory  | `<sys/mman.h>`  | `shm_open()`, `mmap()`     | `munmap()`, `shm_unlink()`     | Direct RAM access           | **Zero-copy data transfer**. Ideal for transferring large datasets such as LiDAR point clouds, camera frames, radar data, or machine vision results between processes without kernel copy overhead. |
| Semaphores     | `<semaphore.h>` | `sem_open()`, `sem_init()` | `sem_close()`, `sem_destroy()` | `sem_wait()`, `sem_post()`  | **Synchronization**. Protecting shared memory regions (mutex-like behavior) or coordinating worker threads/processes when new data becomes available.                                               |
| Message Queues | `<mqueue.h>`    | `mq_open()`                | `mq_close()`, `mq_unlink()`    | `mq_send()`, `mq_receive()` | **Command and Control (C&C)**. Sending commands, status messages, alarms, and prioritized events while preserving message boundaries.                                                               |


### Linux Build Notes

Always link against:

```bash
gcc main.c -o app -pthread -lrt
```

Historically:

* `-pthread` is required for thread and synchronization primitives.
* `-lrt` provides POSIX real-time extensions such as message queues.

On modern glibc versions, many realtime APIs have been merged into libc, but using `-lrt` remains common for portability.

---

# 2. Runtime Pitfalls, Debugging, and Critical Constraints

Low-level IPC programming is highly sensitive to memory management, alignment, synchronization, and resource lifecycle management.

The following issues are among the most common production failures.

---

## SIGBUS Caused by Shared Memory

### Cause

A newly created shared memory object has an initial size of:

```text
0 bytes
```

If you immediately call:

```c
mmap()
```

without first extending the object via:

```c
ftruncate()
```

the mapping technically exists, but accessing it exceeds the actual backing storage.

Result:

```text
SIGBUS (Bus Error)
```

### Correct Sequence

```text
shm_open()
      ↓
ftruncate()
      ↓
mmap()
      ↓
Access Memory
```

---

## EMSGSIZE with POSIX Message Queues

### Cause

`mq_receive()` requires the receive buffer to be at least:

```c
mq_attr.mq_msgsize
```

bytes long.

This requirement applies even if the actual incoming message is much smaller.

Incorrect:

```c
char buf[16];
mq_receive(...);
```

when:

```text
mq_msgsize = 256
```

Result:

```text
EMSGSIZE
```

### Recommended Approach

Always query queue attributes:

```c
mq_getattr()
```

and allocate buffers accordingly.

---

## Resource Leaks and Kernel Persistence

Unlike pipes and sockets, POSIX IPC objects are persistent kernel resources.

Examples:

```c
shm_open()
mq_open()
sem_open()
```

If a process crashes or exits without calling:

```c
shm_unlink()
mq_unlink()
```

the objects remain alive.

On Linux they continue to exist under:

```text
/dev/shm
```

until:

* Explicitly removed
* System reboot

### Best Practice

Always handle termination signals:

```text
SIGINT
SIGTERM
SIGQUIT
```

and perform cleanup before exiting.

---

## Unnamed Semaphore Lifecycle Rules

### Rule #1: Initialize Exactly Once

```c
sem_init()
```

must be called only once for a given semaphore.

Reinitializing an active semaphore results in:

```text
Undefined Behavior
```

---

### Rule #2: Destroy Before Releasing Memory

Before:

```c
free()
munmap()
```

you must call:

```c
sem_destroy()
```

Otherwise the program may leak internal synchronization resources.

---

## Permanent Blocking (Deadlock)

POSIX semaphores do not provide an automatic recovery mechanism equivalent to:

```text
SEM_UNDO
```

found in System V semaphores.

### Failure Scenario

Process A:

```c
sem_wait()
```

acquires the semaphore.

Before executing:

```c
sem_post()
```

the process receives:

```text
SIGKILL
```

or crashes.

Result:

* Semaphore remains locked.
* Waiting processes block forever.
* System enters deadlock.

### Practical Solution

For critical systems requiring crash recovery, use:

```text
Robust Mutexes
```

instead of semaphores.

Robust mutexes can detect owner death and allow recovery.

---

# 3. Advanced and Less Common Features (Quick Reference)

The following features are less portable across UNIX-like systems and are often avoided in production unless a specific design requirement exists.

---

## Asynchronous Message Notification (`mq_notify()`)

Allows a process to register for notification when a previously empty queue receives a message.

Notifications can be delivered through:

* Signals
* Thread creation

### Why It Is Rarely Used

Problems include:

* Complex control flow.
* One-shot registration (must re-register every time).
* Race conditions between message consumers and notification handlers.
* Difficult debugging.

### Preferred Alternative

On Linux, many developers prefer:

```c
poll()
```

or

```c
epoll()
```

on message queue descriptors.

Benefits:

* Simpler event loops.
* Better scalability.
* Easier debugging.

---

## Reading Semaphore Values (`sem_getvalue()`)

```c
int sem_getvalue(sem_t *sem, int *sval);
```

Returns the current semaphore count.

### Why It Is Dangerous

The function suffers from a classic:

```text
TOCTOU
(Time Of Check To Time Of Use)
```

problem.

Example:

```c
sem_getvalue(...);
```

returns:

```text
1
```

Immediately afterward, another thread acquires the semaphore.

Your information is already obsolete.

### Rule

Never use `sem_getvalue()` to make synchronization decisions.

Use it only for diagnostics or debugging.

---

## System Limits and Non-Blocking Operation

### Message Priority Limits

POSIX requires:

```text
MQ_PRIO_MAX >= 32
```

Linux typically supports:

```text
MQ_PRIO_MAX = 32768
```

priorities.

---

### Queue Capacity Limits

Queue limits are controlled through:

```text
/proc/sys/fs/mqueue/msg_max
```

Common default:

```text
10 messages
```

Increasing these limits often requires:

```text
CAP_SYS_RESOURCE
```

privileges.

---

## Non-Blocking Message Queues

Enable:

```c
O_NONBLOCK
```

when opening a queue.

Behavior changes:

### Queue Full

```c
mq_send()
```

returns immediately with:

```text
EAGAIN
```

instead of blocking.

### Queue Empty

```c
mq_receive()
```

returns immediately with:

```text
EAGAIN
```

instead of waiting.

This is useful when implementing:

* Event-driven systems
* Polling loops
* Real-time control software
* High-performance servers

---

# Key Takeaways

* Shared memory provides the highest IPC throughput because data is accessed directly from RAM.
* Semaphores are primarily synchronization tools, not data-transfer mechanisms.
* Message queues are ideal for commands, events, and prioritized control messages.
* Always call `ftruncate()` before mapping newly created shared memory objects.
* POSIX IPC objects persist in the kernel until explicitly unlinked.
* Semaphores can deadlock permanently if a process dies while holding one.
* `mq_notify()` and `sem_getvalue()` are available but should be used cautiously.
* `O_NONBLOCK`, `poll()`, and `epoll()` are often preferred for scalable event-driven designs.
