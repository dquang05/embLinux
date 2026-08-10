# 🚀 C/C++ Embedded Linux & Modern Concurrency

[![Language](https://img.shields.io/badge/Language-C%20%7C%20C%2B%2B20%2F23-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)](https://kernel.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

> *A high-performance repository evolving from low-level Linux System Programming studies into a production-grade Modern C++ Core Library for Embedded Systems.*

## 🌟 The Core Library: `modern_cpp_core/`

The crown jewel of this repository. Built for **Real-Time Embedded Linux** and **High-Performance Backend** environments, it completely replaces legacy threading with strict **C++20/23** paradigms.

**Key Architectural Constraints:**
- **Zero Real-Time Allocations:** Strict avoidance of OS-heap allocation (`new`/`malloc`) in critical wait-free paths to guarantee deterministic O(1) execution time.
- **No Exceptions:** Utilizes `std::expected` (C++23) for predictable, zero-overhead error handling.
- **Modern Concurrency:** Replaces `std::thread` and `std::condition_variable` with `std::jthread`, `std::latch`, `std::counting_semaphore`, and Coroutines.
- **Memory Safety & Testing:** Rigorous stress testing under **ThreadSanitizer (TSan)** to automatically detect Data Races, Deadlocks, and validate lock-free algorithms against the ABA problem.

### Modules:
- **`concurrency/`**: JThreadPool, Spinlocks, atomic wait/notify, and Thread Affinity bindings.
- **`coroutines/`**: Asynchronous `Task<T>`, Generators, CSP Channels, and non-blocking timers.
- **`data_structures/`**: Wait-Free / Lock-Free SPSC Ring Buffers, LockFreeStack (with pre-allocated Node Pools), and Thread-safe HashMaps.
- **`memory/`**: `MonotonicMemoryPool`, `AlignedCacheLine` to prevent False Sharing, and lock-free thread-safe initializations.
- **`patterns/` & `utils/`**: C++20 Concepts, Ranges-based data pipelines, and relaxed telemetry counters.

---

## 📚 Legacy & Learning References

The architecture of `modern_cpp_core` was heavily inspired by the following foundational studies contained in this repository:

### 1. `LSPI/` (Linux System Programming Interface)
Implemented in standard **C**, focusing on the kernel-user space boundary. Inspired by *Operating System Concepts(10th edition)*.
- **`lib/`**: Custom utility library (error handling, numeric parsing) designed to streamline system call error checking.
- **`syntax/`**: Topic-based practical implementations:
  - `01_io/`: Low-level File I/O operations and buffering strategies.
  - `02_process/`: Process lifecycle management (features a custom `miniShell` implementation).
  - `03_signal/`: POSIX signal handling and safe asynchronous execution.
  - `04_ipc/`: Inter-Process Communication (Pipes, FIFOs, POSIX Message Queues, Memory Mapping `mmap`).
  - `05_threads/`: POSIX Threads (pthreads) implementation basics.

### 2. `C++_ConcurrencyInAction/`
Legacy implementations based on *C++ Concurrency in Action*. These contain the classic C++11/14 threading and synchronization patterns that `modern_cpp_core` aims to solve and deprecate.
- **`threads/`**: Thread lifecycle management and data sharing fundamentals.
- **`synchronizingConcurrency/`**: Advanced synchronization (`std::condition_variable`, `std::future`, async tasks).
- **`memory_atomicOperation/`**: Deep dive into the C++ Memory Model, `std::atomic`, and lock-free structures (e.g., Treiber Stack).
- **`designLockBase/` & `designLockFree/`**: Designing thread-safe data structures with and without locks.
- **`designConcurrentCode/`**: Real-world concurrency issues (False Sharing, Exception Safety) and parallel algorithms.
- **`advancedThreadsManagement/`**: Advanced architectural patterns (Work-stealing Thread Pools, Deadlock-free pools).

---

## 🛠️ Build & Test Instructions

The project uses **CMake**. For `modern_cpp_core`, we strictly enforce Sanitizer-based testing.

### Standardized Test Routine (with ThreadSanitizer)
```bash
cd modern_cpp_core/build

# 1. Clear CMake cache to avoid ASan/TSan conflicts
rm -f CMakeCache.txt

# 2. Configure with TSan enabled
cmake -DENABLE_ASAN=OFF -DENABLE_TSAN=ON ..

# 3. Build using all available cores
make -j$(nproc)

# 4. Run tests with ASLR disabled (Workaround for TSan kernel bugs)
setarch x86_64 -R ctest -V
```

*(Note: Legacy modules like `LSPI/` contain their own independent `CMakeLists.txt` or `Makefile` for isolated building).*
