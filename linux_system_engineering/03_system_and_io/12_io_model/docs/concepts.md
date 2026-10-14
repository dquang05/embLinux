# Core Concepts: I/O Systems & Linux I/O Models

## Reading Limits
- **OS.txt**: Chapter 12 (I/O Systems) from line **32352** to **34430**.
- **Syntax.txt**: Chapter 13 (File I/O Buffering) from line **13997** to **14950**.
- **Syntax.txt**: Chapter 63 (Alternative I/O Models) from line **70175** to **72800**.

---

This document synthesizes core concepts of **I/O Systems and Models** spanning foundational Operating System theory (OSC) to concrete Linux Kernel implementations (TLPI). The content is structured into two tiers: **Core Foundations** required for every Embedded/Systems Engineer, and **Advanced Optimizations** for high-concurrency (C10K/C1000K) and real-time streaming services.

---

## 1. Core Foundations
Essential principles required for system programming, file operations, and networking in Linux.

👉 **Detailed Guide: [core_knowledge.md](core_knowledge.md)**

**Key Takeaways:**
- **Hardware Architecture & I/O Coordination**:
  - CPU-Controller communication via **Memory-Mapped I/O (MMIO)**: device registers are mapped into the physical address space, manipulated using standard memory instructions.
  - Three coordination mechanisms: **Polling** (busy-wait: zero latency when ready, CPU-wasteful when slow), **Interrupt-Driven** (two-tiered FLIH/Top-half and SLIH/Bottom-half, plus hybrid NAPI under high packet loads), and **Direct Memory Access (DMA)** (cycle-stealing data transfers directly between device and RAM without CPU intervention).
- **Two-Tier Buffering Architecture**:
  - **User-space (`stdio`) buffering**: Reduces system call overhead (`_IONBF`, `_IOLBF`, `_IOFBF`). Managed by `setvbuf()`, flushed with `fflush()`. Be aware of output reordering when mixing `printf()` and `write()`.
  - **Kernel Buffer Cache (Page Cache)**: Optimizes disk I/O through delayed writes and kernel dirty page flusher threads.
- **Disk Synchronization & Data/File Integrity**:
  - **File Integrity** (`fsync()`, `O_SYNC` flag): Guarantees both data content and all filesystem metadata (including timestamps) are committed to storage.
  - **Data Integrity** (`fdatasync()`, `O_DSYNC` flag): Flushes only data content and essential retrieval metadata (file size, extent pointers), avoiding unnecessary disk head movement to the inode table.
- **Four I/O Models & Readiness Semantics**:
  - Contrasting **Blocking**, **Non-blocking** (`O_NONBLOCK`, returning `EAGAIN`/`EWOULDBLOCK`), **I/O Multiplexing**, and **Asynchronous I/O (AIO)**.
  - SUSv3 Readiness definition: A file descriptor is **Ready** if an I/O operation on it **will not block**. Regular files are always ready. Sockets/pipes are ready for read on available data or peer shutdown (`POLLIN`, `POLLRDHUP`), and ready for write when send buffer space is available (`POLLOUT`).
- **I/O Multiplexing: `select()`, `poll()`, and `epoll()`**:
  - `select()`: Hardcoded `FD_SETSIZE` (1024) limit, value-result bitmasks requiring loop re-initialization, $\mathcal{O}(N)$ scaling.
  - `poll()`: Dynamically sized `struct pollfd` array, separates requested `events` from returned `revents`, but still scales as $\mathcal{O}(N)$.
  - `epoll` (`epoll_create1`, `epoll_ctl`, `epoll_wait`): Achieves $\mathcal{O}(1)$ active event dispatching via an in-kernel Red-Black Tree (Interest List) and a doubly linked list (Ready List).
- **Level-Triggered (LT) vs. Edge-Triggered (ET)**:
  - LT reports the current readiness state (repeatedly wakes up while unread data remains).
  - ET reports state transitions $\rightarrow$ **Requires non-blocking descriptors** and an exhaustive loop draining data until `EAGAIN`.
- **Signal & I/O Coordination**:
  - Eliminating race conditions between signal handlers and I/O wait calls using atomic primitives (`pselect()`, `ppoll()`, `epoll_pwait()`) or the classic **Self-Pipe Trick** / `signalfd()`.

---

## 2. Advanced Optimizations & High-Performance Patterns
Specialized mechanisms for maximizing throughput, mitigating bottlenecks, and managing memory in resource-constrained embedded systems.

👉 **Detailed Guide: [advanced_optimizations.md](advanced_optimizations.md)**

**Key Takeaways:**
- **Direct I/O (`O_DIRECT`)**:
  - Completely bypasses the kernel page cache to prevent double buffering in custom database engines or bulk data transfers.
  - Enforces strict memory alignment (buffer address, file offset, data length) to multiples of the underlying block size (e.g., 4096 bytes via `posix_memalign()`).
- **Non-blocking Sliding Flushes via `sync_file_range()`**:
  - Flushes specific byte ranges in the background with `SYNC_FILE_RANGE_WRITE`, avoiding full-file stalls and eliminating video frame drops on embedded cameras/dashcams.
- **Active Cache Eviction with `posix_fadvise()`**:
  - Employs `POSIX_FADV_DONTNEED` to evict cached pages immediately after single-pass reads, preventing OOM killer triggers on embedded targets with small RAM (128MB-512MB).
  - Uses `POSIX_FADV_SEQUENTIAL` to double the kernel's read-ahead window.
- **Vectored I/O (`readv` / `writev`) & Zero-Copy (`sendfile`)**:
  - Scatter-Gather I/O sends disjoint buffers (e.g., protocol header + payload) atomically in a single system call without intermediate memory concatenation.
  - Zero-Copy transfers data directly from page cache to network interface card without user-space buffer copies.
- **Edge-Triggered Starvation Mitigation**:
  - Implements user-space ready queues with byte quotas (budgets) and Round-Robin servicing to prevent high-bandwidth sockets from monopolizing the event loop.
- **Multi-Threaded Event Loop with `EPOLLONESHOT`**:
  - Automatically disables event delivery after one worker thread receives it, preventing Thundering Herd and race conditions when multiple workers share a single `epfd`.
- **Open File Description Semantics in `epoll`**:
  - Epoll monitors the underlying kernel *open file description*, not the process *file descriptor*. Descriptors cloned via `dup()` or `fork()` keep the event active until all duplicate instances are closed.
