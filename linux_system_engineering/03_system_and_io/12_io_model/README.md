# Chapter 12: I/O Systems & Linux I/O Models

## Objectives
- Master the hardware and kernel foundations of I/O Systems: Memory-Mapped I/O (MMIO), Polling, Interrupts (Top-half/Bottom-half, NAPI), and Direct Memory Access (DMA).
- Deeply understand Linux Two-Tier Buffering: User-space (`stdio`) buffering vs. Kernel Buffer Cache (Page Cache), and avoid classic pitfalls like mixing `printf()` with `write()`.
- Distinguish Synchronized I/O Data Integrity (`fdatasync()`, `O_DSYNC`) from File Integrity (`fsync()`, `O_SYNC`), and master fine-grained sliding flushes using `sync_file_range()`.
- Understand the 4 standard I/O models: Blocking, Non-blocking (`O_NONBLOCK`), I/O Multiplexing, and Asynchronous I/O (AIO).
- Compare and contrast I/O Multiplexing primitives: `select()`, `poll()`, and `epoll()`, understanding their algorithmic complexities ($\mathcal{O}(N)$ vs. $\mathcal{O}(1)$) and kernel data structures.
- Master `epoll` architecture: Level-Triggered (LT) vs. Edge-Triggered (ET), implementing the canonical non-blocking loop with `EAGAIN` handling and preventing starvation.
- Implement robust multi-threaded server architectures using `EPOLLONESHOT` and resolve Signal-I/O race conditions using `epoll_pwait()` and the Self-Pipe Trick / `signalfd`.
- Apply specialized high-performance I/O optimizations: Direct I/O (`O_DIRECT`) memory alignment, cache tuning with `posix_fadvise()`, and Vectored I/O (`readv`/`writev`).

## References
- **OSC (Operating System Concepts)**:
  - Chapter 12: I/O Systems (I/O Hardware, Port I/O vs. MMIO, Polling, Interrupts, DMA, Application I/O Interface, Kernel I/O Subsystem, Transforming I/O Requests to Hardware Operations, Power Management)
- **TLPI (The Linux Programming Interface)**:
  - Chapter 13: File I/O Buffering (Kernel Buffering, stdio Buffering, `setvbuf`, `fflush`, `fsync`, `fdatasync`, `sync_file_range`, `O_SYNC`, `posix_fadvise`, Direct I/O `O_DIRECT`)
  - Chapter 63: Alternative I/O Models (I/O Multiplexing `select` and `poll`, Readiness Semantics, Signal-Driven I/O, `epoll` API, Level-Triggered vs. Edge-Triggered, `EPOLLONESHOT`, `pselect`, `ppoll`, `epoll_pwait`, Self-Pipe Trick)

## Documentation Guide
- [Core Concepts & Taxonomy](docs/concepts.md)
- [Core Foundations: Hardware, Buffering, Readiness & epoll](docs/core_knowledge.md)
- [Advanced Optimizations: Direct I/O, sync_file_range, ET Starvation & Multi-Threading](docs/advanced_optimizations.md)

## Source Code Demonstrations (`src/`)

| File | Description | Key APIs Used |
| :--- | :--- | :--- |
| `01_blocking_vs_nonblocking.c` | Compares blocking read vs. non-blocking read using pipes, demonstrating `O_NONBLOCK` toggling via `fcntl()` and handling `EAGAIN`/`EWOULDBLOCK`. | `pipe()`, `read()`, `write()`, `fcntl(F_GETFL/F_SETFL, O_NONBLOCK)` |
| `02_select_demo.c` | Demonstrates I/O multiplexing across multiple pipes with `select()`, showing `fd_set` manipulation macros and timeout handling. | `select()`, `FD_ZERO()`, `FD_SET()`, `FD_ISSET()` |
| `03_poll_demo.c` | Monitors multiple child worker pipes using an array of `struct pollfd`, highlighting separation of `events` and `revents` and `POLLHUP` detection. | `poll()`, `struct pollfd`, `POLLIN`, `POLLHUP` |
| `04_epoll_demo.c` | Demonstrates the Linux `epoll` API and clearly contrasts Level-Triggered (LT) vs. Edge-Triggered (`EPOLLET`) notification with the canonical `EAGAIN` draining loop. | `epoll_create1()`, `epoll_ctl()`, `epoll_wait()`, `EPOLLET` |

## Build and Run Instructions

### 1. Compile All Demos
From this directory, run:
```bash
make
```
All binaries will be built into the `build/` directory with strict compiler flags (`-Wall -Wextra -Werror -g`).

### 2. Run Individual Demonstrations
```bash
# Run Blocking vs. Non-blocking demo
./build/01_blocking_vs_nonblocking

# Run select() multiplexing demo
./build/02_select_demo

# Run poll() multiplexing demo
./build/03_poll_demo

# Run epoll() LT vs ET demo
./build/04_epoll_demo
```

### 3. Clean Build Artifacts
```bash
make clean
```
