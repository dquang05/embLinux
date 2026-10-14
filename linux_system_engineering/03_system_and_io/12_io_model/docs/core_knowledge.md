# Core Knowledge: I/O Systems & Linux I/O Models

This document outlines the essential concepts that every Embedded Linux and Systems Engineer must master when designing high-performance I/O applications, network daemons, and low-latency device interfaces.

---

## 1. Hardware I/O Foundations & Coordination Mechanisms

Every computer system balances two fundamental workloads: **Computation** and **Input/Output (I/O)**. In modern network servers, databases, and embedded edge devices, I/O latency and throughput dominate overall application performance.

### 1.1 CPU - Device Controller Communication: Port I/O vs. Memory-Mapped I/O (MMIO)
- **Port I/O (Isolated I/O)**: The CPU uses dedicated machine instructions (e.g., `in` and `out` on x86 architectures) to address a distinct hardware I/O port address space separate from physical RAM.
- **Memory-Mapped I/O (MMIO - Modern Standard)**:
  - Device controller registers are mapped directly into physical system memory address space.
  - The CPU communicates with hardware peripherals using standard load/store memory instructions (`MOV`, pointer dereferencing). This offers superior bandwidth, simplifies driver code, and integrates cleanly with paging and DMA subsystems.
- **Four Standard Registers of a Device Controller**:
  1. `data-in`: Read by the host to receive input data from the hardware.
  2. `data-out`: Written by the host to transmit output data to the hardware.
  3. `status`: Read by the host to inspect flags (`busy`, `command-ready`, `error`).
  4. `control`: Written by the host to issue commands, set duplex mode, or configure device parameters.

### 1.2 Three I/O Coordination Mechanisms
```
+-----------------------------------------------------------------------------------+
| Polling (Busy-Waiting)  | Interrupt-Driven I/O         | Direct Memory Access (DMA) |
+-------------------------+------------------------------+----------------------------+
| Host continuously reads | Controller asserts an IRQ    | DMA Controller transfers   |
| the controller status   | line when data is ready; CPU | blocks of data directly    |
| register in a tight     | saves state and invokes an   | between device and RAM.    |
| loop until 'busy' is 0. | Interrupt Service Routine.   | Host CPU is interrupted    |
| Zero latency when data  | Two-tier handling: Top-half  | only once after the entire |
| is immediately ready;   | (FLIH) and Bottom-half (SLIH)| multi-kilobyte block is    |
| wastes CPU cycles when  | Hybrid NAPI: interrupts on   | transferred, using bus     |
| hardware is slow.       | low load, polling on floods. | Cycle Stealing.            |
+-----------------------------------------------------------------------------------+
```

---

## 2. Linux Two-Tier Buffering Architecture

When a C application writes to or reads from a file, pipe, or socket, data travels through two distinct buffering layers:

```
+--------------------------------------------------------------------+
| User Space: Standard C Library (stdio) Buffering                   |
| Functions: printf(), fprintf(), fputs(), fread(), fwrite()         |
| Managed by: glibc (FILE* structure allocated in heap / memory)     |
+---------------------------------+----------------------------------+
                                  | System Calls: read(), write()
                                  v
+--------------------------------------------------------------------+
| Kernel Space: Buffer Cache (Page Cache)                            |
| Managed by: Linux Kernel (RAM pages, Dirty Page tracking)          |
+---------------------------------+----------------------------------+
                                  | Disk I/O: pdflush / flusher / fsync
                                  v
+--------------------------------------------------------------------+
| Physical Storage: NVMe / SSD / Flash NAND / HDD                    |
+--------------------------------------------------------------------+
```

### 2.1 User-Space Buffering (`stdio`)
- **Primary Goal**: Minimize user-to-kernel context switches by aggregating small I/O operations into larger contiguous blocks.
- **Three Buffering Modes (`setvbuf`)**:
  - `_IONBF` (Unbuffered): Operations are immediately passed to system calls (Default for `stderr`).
  - `_IOLBF` (Line-buffered): Data is buffered until a newline `\n` is encountered or buffer capacity is reached (Default for terminal `stdout`).
  - `_IOFBF` (Fully-buffered): Flushed only when the internal buffer is completely full (Default for regular disk files, pipes, and sockets; typically 4096 or 8192 bytes).
- **Core Functions**:
  - `setvbuf(FILE *stream, char *buf, int mode, size_t size)`: Configures buffering; must be invoked before any I/O on the stream.
  - `fflush(FILE *stream)`: Forces unwritten data in the `stdio` buffer into the **Kernel Page Cache** (does not guarantee physical disk writes).
  - `fileno(FILE *stream)`: Extracts the underlying file descriptor integer (`fd`).
  - `fdopen(int fd, const char *mode)`: Creates a `FILE*` stream wrapper around an open descriptor (common when using `fprintf` over sockets).
- **Classic Output Ordering Pitfall**:
  - When redirecting `stdout` to a file (making it fully-buffered), calling `printf("A\n"); write(STDOUT_FILENO, "B\n", 2);` writes `"B\nA\n"` to the file because `write()` directly enters the kernel while `printf()` remains buffered in user space.

### 2.2 Kernel-Space Buffering (Page Cache)
- System call `write()` returns immediately after copying data from user buffers into Kernel Page Cache. The modified page is marked as a **Dirty Page**.
- Kernel background flusher threads (`pdflush`, `flusher`) flush dirty pages to storage periodically (e.g., every 30 seconds or when dirty ratios exceed memory thresholds).

---

## 3. Storage Synchronization: Data Integrity vs. File Integrity

POSIX and Linux distinguish two levels of persistence guarantees when writing to persistent storage:
1. **Synchronized I/O Data Integrity**: Ensures that the file's data bytes and the *minimum essential metadata required to retrieve that data* (e.g., file size, block/extent pointers) are committed to physical media. Non-essential metadata (`st_atime`, `st_mtime`) are omitted.
2. **Synchronized I/O File Integrity**: Guarantees that *all* file data and *all* metadata attributes (including timestamps) are written to physical media.

### 3.1 Comparison of Synchronization APIs
| API / Flag | Scope | Integrity Level | Execution Behavior |
| :--- | :--- | :--- | :--- |
| **`fsync(fd)`** | Single file | **File Integrity** | Blocks until all file data and full inode metadata are persisted to media. |
| **`fdatasync(fd)`** | Single file | **Data Integrity** | Flushes data and critical extent metadata; saves an extra disk head seek to the inode table. |
| **`sync()`** | Entire OS | System-wide | Queues all dirty buffers across the filesystem for immediate writeout. |
| **`O_SYNC`** | Open flag | **File Integrity** | Every `write()` call implicitly performs an `fsync()` before returning. |
| **`O_DSYNC`** | Open flag | **Data Integrity** | Every `write()` call implicitly performs an `fdatasync()` before returning. |

---

## 4. The Four Canonical I/O Models

```
1. Blocking I/O:
   Process ---> [ System Call: read() ] ---------------------> (Sleep in Wait Queue)
                                                                       | Data arrives
   Process <--- [ Return with data     ] <---------------------+ (Wake up)

2. Non-blocking I/O:
   Process ---> [ System Call: read() ] ---> Data ready? NO ---> Returns EAGAIN immediately.
   Process ---> [ System Call: read() ] ---> Data ready? YES --> Copies data & returns.

3. I/O Multiplexing (select / poll / epoll):
   Process ---> [ epoll_wait()        ] ---------------------> (Sleep until ANY fd is ready)
                                                                       | FD ready
   Process <--- [ Returns ready FDs    ] <---------------------+ (Wake up)
   Process ---> [ read(fd)            ] ---------------------> Non-blocking read.

4. Asynchronous I/O (POSIX AIO / io_uring):
   Process ---> [ aio_read()          ] ---> Kernel initiates transfer & returns immediately.
   Process continues other execution...
   Process <--- [ Signal / Completion  ] <--- Notified when data is ALREADY in user buffer.
```

---

## 5. File Descriptor Readiness Semantics

According to SUSv3: A file descriptor is considered **Ready** if an I/O operation on it **will not block**, regardless of whether data is actually transferred.

- **Regular Files**: Always reported as ready for reading and writing (`POLLIN | POLLOUT`) because file operations access the in-memory Page Cache directly and do not wait in device wait queues. Therefore, **I/O multiplexing is never used for regular files**.
- **Sockets and Pipes**:
  - **Readable (`POLLIN` / `EPOLLIN`)**:
    - Data is present in the socket receive buffer.
    - A listening socket has a pending connection available (`accept()` will not block).
    - Peer closed its write half (`shutdown(SHUT_WR)` or `close()`): `read()` immediately returns `0` (EOF).
  - **Writable (`POLLOUT` / `EPOLLOUT`)**:
    - Sufficient space exists in the socket send buffer.
    - A non-blocking `connect()` call completed the 3-way TCP handshake.
  - **Hangup & Errors**:
    - `POLLRDHUP` / `EPOLLRDHUP` (Linux 2.6.17+): Peer closed connection or shut down writing.
    - `POLLHUP` / `EPOLLHUP`: Hangup occurred on the channel.
    - `POLLERR` / `EPOLLERR`: Asynchronous error condition detected (e.g., TCP RST).

---

## 6. Comparing `select()`, `poll()`, and `epoll()`

### 6.1 Architectural Differences
| Feature | `select()` | `poll()` | `epoll()` |
| :--- | :--- | :--- | :--- |
| **Max Descriptors** | Capped by `FD_SETSIZE` (1024) | Uncapped (dynamic array size) | Uncapped (governed by kernel memory) |
| **Data Structure** | Bitmasks (`fd_set`) | Array of `struct pollfd` | Red-Black Tree + Ready Doubly Linked List |
| **Parameter Passing**| Value-result (overwritten by kernel; requires loop re-initialization) | Separate `events` (in) and `revents` (out) | Configured once via `epoll_ctl()`; wait via `epoll_wait()` |
| **Time Complexity** | $\mathcal{O}(N)$ (scans 0 to `max_fd`) | $\mathcal{O}(N)$ (scans input array) | $\mathcal{O}(1)$ per active event |
| **Notification Mode**| Level-Triggered only | Level-Triggered only | Both Level-Triggered and Edge-Triggered |
| **Invalid FD Report**| Returns `-1` with `EBADF` | Flags `POLLNVAL` on exact element | Automatically deregistered upon `close()` |

### 6.2 Why `select()` and `poll()` Fail to Scale (The C10K Bottleneck)
1. **Memory Copy Overhead**: Every call transfers data structures describing all $N$ descriptors from user space to kernel space and back.
2. **Kernel Iteration Cost**: The kernel must traverse all $N$ descriptors to register with wait queues and poll device states.
3. **User-Space Scanning Overhead**: After the call returns, user space must loop over all $N$ descriptors to test which ones are ready.
$\rightarrow$ At 10,000 connections with only 2 active events, `select`/`poll` waste 99% of CPU time scanning idle descriptors.

---

## 7. The Linux `epoll` API

`epoll` solves the C10K problem by **recording an in-kernel Interest List across calls**, dispatching only ready descriptors via a dedicated **Ready List**.

### 7.1 Core System Calls
```c
#include <sys/epoll.h>

// 1. Create an epoll instance (returns an epoll file descriptor)
int epoll_create1(int flags); // Recommended flag: EPOLL_CLOEXEC

// 2. Add, modify, or remove descriptors in the Interest List
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
// op: EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL

// 3. Block waiting for I/O events from the Ready List
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```

### 7.2 Event Structures
```c
struct epoll_event {
    uint32_t     events;  /* Event bitmask: EPOLLIN, EPOLLOUT, EPOLLET, EPOLLONESHOT... */
    epoll_data_t data;    /* User data union */
};

typedef union epoll_data {
    void        *ptr;     /* Pointer to custom connection context struct */
    int          fd;      /* File descriptor */
    uint32_t     u32;
    uint64_t     u64;
} epoll_data_t;
```

---

## 8. Level-Triggered (LT) vs. Edge-Triggered (ET)

Understanding this distinction is critical for robust server architecture:

```
Scenario: 100 bytes of data arrive at a socket. The application reads only 20 bytes.
- Level-Triggered (Default):
  Kernel check: "80 bytes remain in buffer" -> Next epoll_wait() immediately returns READY again.
- Edge-Triggered (EPOLLET):
  Kernel reports only when the state TRANSITIONS (from empty to non-empty).
  Since no new data arrived after the partial read, epoll_wait() BLOCKS INDEFINITELY!
  The remaining 80 bytes are stuck unhandled!
```

### 8.1 The Canonical Non-Blocking Edge-Triggered Loop Pattern
To safely use `EPOLLET`, applications **must adhere to two mandatory rules**:
1. All monitored file descriptors **must be set to Non-blocking mode (`O_NONBLOCK`)**.
2. When notified, the program **must loop over `read()` or `write()` until it fails with `EAGAIN` or `EWOULDBLOCK`**.

```c
// Canonical read loop for EPOLLET:
while (1) {
    ssize_t bytes_read = read(fd, buf, sizeof(buf));
    if (bytes_read == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // All available kernel buffer data drained; safe to resume epoll_wait()
            break;
        }
        perror("read error");
        close(fd);
        break;
    } else if (bytes_read == 0) {
        // Peer disconnected cleanly (EOF)
        close(fd);
        break;
    }
    // Process the received 'bytes_read' payload...
}
```

---

## 9. Signal and I/O Synchronization (Self-Pipe Trick & Atomic APIs)

### 9.1 The Classic Signal Race Condition
When an application monitors both signals (`SIGINT`, `SIGTERM`) and I/O descriptors:
- If a signal arrives *after* checking a termination flag but *before* calling `select()` / `epoll_wait()`, the call blocks indefinitely and the signal event is lost.

### 9.2 Atomic Waiting Primitives (`pselect`, `ppoll`, `epoll_pwait`)
These system calls atomically unmask signals and enter the kernel sleep state in a single step:
```c
int epoll_pwait(int epfd, struct epoll_event *events, int maxevents, 
                int timeout, const sigset_t *sigmask);
```

### 9.3 The Self-Pipe Trick (and `signalfd`)
- Create a non-blocking pipe: `pipe(pfd)`.
- Register read end `pfd[0]` with `epoll` or `select`.
- In the signal handler, invoke an async-signal-safe write: `write(pfd[1], "x", 1);`.
- `epoll_wait()` unblocks seamlessly as if handling standard I/O!
- *On modern Linux systems, `signalfd()` provides a cleaner mechanism by creating a dedicated file descriptor for receiving signals directly.*
