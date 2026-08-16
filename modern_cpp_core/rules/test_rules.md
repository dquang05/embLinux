# C++ Testing Rules & Guidelines - `modern_cpp_core`

As a strict requirement for the **Embedded Linux** and **High-Performance Backend** environments, testing must be rigorous, focusing heavily on concurrency, deterministic execution, and memory safety.

## 1. General Testing Requirements
- **Coverage**: Every core module or utility function must be accompanied by a dedicated test runner (e.g., `test_module.cpp`).
- **Test Output**: Tests must clearly print `PASS` or `FAIL` for each test case. Any test failure must result in a **non-zero exit code** to ensure CI/CD pipelines can automatically catch regressions.
- **Edge Case Verification**: Tests must explicitly target and verify all edge cases outlined in the `// RISK REVIEW:` comments of the implementation files. This includes:
  - Null or empty inputs.
  - Maximum boundary conditions and **Integer Overflows** (especially when porting between x86_64 host and ARM targets). Always use fixed-width types (`<cstdint>`).
  - Intentional allocation failures (verify that the function returns `std::expected<T, Error>` instead of crashing).
  - **Memory Alignment (Unaligned Access)**: For APIs processing raw byte streams (network/sensors), test inputs with unaligned offsets to ensure the system does not trigger an Alignment Fault (Hardware Trap on ARM).

## 2. Concurrency, Stress & Determinism Testing
- **ThreadSanitizer (TSan)**: All multithreaded code must be compiled and tested with TSan (`-fsanitize=thread`) to automatically detect Data Races and Deadlocks. *(Note: TSan cannot detect Livelocks)*.
- **Livelock Detection (Timeouts)**: Since TSan cannot catch livelocks (where threads actively spin without progress), all stress tests must be wrapped with a strict execution **timeout** (e.g., using `ctest --timeout`). If a test hangs, it must fail automatically as a potential Livelock.
- **Stress & Contention**: Tests for concurrent data structures (e.g., `WaitFreeQueue`, `LockFreeStack`) must intentionally simulate heavy contention to provoke the ABA problem and verify lock-free logic.
- **Concurrent GO Signal (C++20 `std::latch`)**: When writing unit tests for concurrent containers, use `std::latch` to ensure all testing threads start their critical sections at exactly the same time. This avoids thread-creation latency from skewing the test results and maximizes the chance of reproducing race conditions.
- **Determinism & WCET (Worst Case Execution Time)**: For real-time structures, write benchmark tests to measure execution times. Verify that operations (e.g., `push`/`pop` in a `WaitFreeQueue`) do not suffer from unexpected latency spikes due to hidden locks, allocations, or hash collisions.

## 3. Memory & Resource Constraints
- **Zero OS-heap Allocations in Real-Time Paths**: Ensure tests validate that real-time wait-free components (like `WaitFreeQueue`) do not dynamically allocate memory (`new`/`malloc`) at runtime.
- **Memory Pool Boundary Testing**: If an API utilizes pre-allocated `MonotonicMemoryPool` or ring buffers, explicitly test the boundary conditions (e.g., push until the pool is full) to ensure it correctly returns an error state rather than silently overwriting or crashing.

## 4. Sanitizer Rules & ASLR Workaround
- **Mutually Exclusive Sanitizers**: AddressSanitizer (ASan) and ThreadSanitizer (TSan) cannot be mixed. You must clear the CMake cache (`CMakeCache.txt`) before switching between them.
- **ASLR Bug with TSan**: On modern Linux kernels, TSan has a known bug with Address Space Layout Randomization (ASLR). To prevent `unexpected memory mapping` crashes during testing, you must wrap the test execution command with `setarch x86_64 -R`.

## 5. Standardized Test Execution Commands
All developers and CI/CD pipelines must use the following standardized routine to build and execute tests safely:

```bash
cd /home/quangtran/Projects/embLinux/modern_cpp_core/build
rm -f CMakeCache.txt
cmake -DENABLE_ASAN=OFF -DENABLE_TSAN=ON ..
make -j$(nproc)
setarch x86_64 -R ctest -V
```

## 6. Architectural Test Constraints
- **Strictly C++20/23**: Test files must also adhere to the strict C++20/23 standard. Use `std::jthread` instead of `std::thread`.
- **No Exceptions**: Even in tests, verify the returned `std::expected<T, Error>` rather than relying on `try/catch` blocks, maintaining parity with production code constraints. Test cases must explicitly feed invalid data to ensure the API catches it and returns an error without invoking `std::terminate()`.
