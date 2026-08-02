# 🚀 C/C++ Learning Journey: Embedded Linux & Modern Concurrency

[![Language](https://img.shields.io/badge/Language-C%20%7C%20C%2B%2B-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)](https://kernel.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

> *A structured learning repository focusing on low-level Linux System Programming and high-performance Modern C++ Concurrency.*

This repository contains my source code, exercises, and architectural notes compiled during my study of advanced Linux internals and C++ multithreading. It serves as both a personal knowledge base and a practical reference for embedded software development.

## 📚 Core References

The architecture and implementations within this repository are heavily inspired by two industry-standard books:
- **[The Linux Programming Interface (TLPI)](https://man7.org/tlpi/)** by Michael Kerrisk
- **[C++ Concurrency in Action](https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition)** by Anthony Williams

## 🗂️ Repository Structure

### 1. `LSPI/` (Linux System Programming Interface)
Implemented in standard **C**, focusing on the kernel-user space boundary and OS primitives.
- **`lib/`**: Custom utility library (error handling, numeric parsing) designed to streamline system call error checking.
- **`syntax/`**: Topic-based practical implementations:
  - `01_io/`: Low-level File I/O operations and buffering strategies.
  - `02_process/`: Process lifecycle management (features a custom `miniShell` implementation).
  - `03_signal/`: POSIX signal handling and safe asynchronous execution.
  - `04_ipc/`: Inter-Process Communication (Pipes, FIFOs, POSIX Message Queues, Memory Mapping `mmap`).
  - `05_threads/`: POSIX Threads (pthreads) implementation basics.

### 2. `C++_ConcurrencyInAction/`
Implemented in **Modern C++ (11/14/17/20)**, focusing on high-performance concurrent design patterns.
- **`threads/`**: Thread lifecycle management and data sharing fundamentals.
- **`synchronizingConcurrency/`**: Advanced synchronization (`std::condition_variable`, `std::future`, async tasks).
- **`memory_atomicOperation/`**: Deep dive into the C++ Memory Model, `std::atomic`, and lock-free structures (e.g., Treiber Stack).
- **`designLockBase/` & `designLockFree/`**: Designing thread-safe data structures with and without locks.
- **`designConcurrentCode/`**: Real-world concurrency issues (False Sharing, Exception Safety) and parallel algorithms.
- **`advancedThreadsManagement/`**: Advanced architectural patterns (Work-stealing Thread Pools, Deadlock-free pools).

### 3. `modern_cpp_core/` (Modern C++ Utility Library)
A living collection of reusable, high-performance C++ utility components built for real-world applications.
- Target: Production use in embedded systems and backend environments.
- Tech Stack: Pure Modern C++ (C++20/23), upgrading legacy concurrency patterns with `std::jthread`, Coroutines, Concepts, and Ranges.
- **Modules**:
  - `concurrency/`: Modern threading (`std::jthread`, `std::latch`, `std::semaphore`).
  - `coroutines/`: Async programming and lazy evaluation (`co_await`, generators).
  - `data_structures/`: Lock-free and thread-safe containers.
  - `memory/`: Memory pools, allocators, and non-owning views (`std::span`).
  - `patterns/`: Design patterns leveraging C++20 Concepts.
  - `utils/`: Data processing pipelines using C++20 Ranges.

## 🛠️ Build Instructions

This project uses **CMake** as its primary build system. To compile the examples:

```bash
mkdir build && cd build
cmake ..
make
```

*(Note: Specific modules like `LSPI/syntax` contain their own `CMakeLists.txt` for isolated building and testing).*

---
*Developed with a focus on writing clean, scalable, and deterministic embedded system software.*
