# Coding Rules - `modern_cpp_core`

My library for **Embedded Systems** and **High-Performance Backend** environments. Strictness, performance optimization, and modern practices are the top priorities. Rules:

## 1. Language Standard & Strictness
- **Strictly C++20/23**. Do not use deprecated features from C++11/14 if a better C++20 alternative exists.
- **Concepts over SFINAE**: Do not use `std::enable_if`. Always prefer **Concepts** (`requires`) for template constraints.
- **Compiler Strictness**: All code must compile with `-Wall -Wextra -Werror`. The standard is **Zero Warnings, Zero Errors**.

## 2. Code Formatting & Style
- **Indentation**: Use 1 tab (displayed as 8 spaces) instead of spaces.
- **Braces**: Use K&R style (opening brace at the end of the declaration/control statement line).
- **Pointers**: The asterisk `*` must be placed adjacent to the variable name, not the type (e.g., `char *p`, not `char* p`).
- **Naming Conventions**:
  - **Classes, Structs, Concepts**: `PascalCase` (e.g., `ThreadPool`, `LockFreeQueue`).
  - **Functions, Local Variables**: `snake_case` (e.g., `get_thread_id()`, `buffer_size`).
  - **Class Member Variables**: Prefix `m_` followed by `snake_case` (e.g., `m_worker_threads`, `m_is_running`).
  - **Constants, `constexpr`**: Prefix `k` followed by `PascalCase` (e.g., `kMaxBufferSize`, `kDefaultTimeout`).
  - **Files**: Lowercase separated by underscores (e.g., `thread_pool.hpp`, `core_concepts.hpp`).

## 3. Documentation & Comments
- **Doxygen Style**: All public APIs (classes, structs, functions, concepts) must be documented using standard Doxygen tags (`@brief`, `@param`, `@return`, `@note`).
- **Risk Review**: At the end of implementation files (`.cpp`) or above highly complex functions, include a `// RISK REVIEW:` comment block. This block must list:
  - Remaining potential bugs or concurrency risks.
  - Caller responsibilities (e.g., "Caller must ensure X outlives Y").
  - Unspecified edge cases.

## 4. Memory & Resources
- **NO RAW `new` / `delete`**: Manual memory management is strictly prohibited. Always use Smart Pointers (`std::unique_ptr` is preferred; `std::shared_ptr` only when shared ownership is genuinely required).
- **Ownership & Allocation**: If a function must return raw dynamically allocated memory (e.g., for C-interop), it must be clearly documented that the caller assumes ownership and is responsible for calling `free()`.
- **Fail-Safe Allocation**: If memory allocation fails (e.g., a custom allocator returns `NULL`), the function **must not crash or call `exit()`**. It must return an error state (e.g., `std::expected`) for the caller to handle gracefully.
- **Adhere to RAII** (Resource Acquisition Is Initialization). Wrap all resources (Files, Sockets, Mutexes) inside classes.
- **Read-only Data Views**: Use **`std::span<const T>`** or **`std::string_view`** instead of passing raw pointers with size arguments (`T* ptr, size_t size`).

## 5. Concurrency & Multi-threading
- **NO `std::thread`**: You must use **`std::jthread`**. It auto-joins on destruction and supports safe thread interruption via `std::stop_token`.
- **No Manual Locking**: Never call `lock()` or `unlock()` manually on a Mutex. Always use **`std::scoped_lock`** (or `std::unique_lock` if waiting on a Condition Variable).
- **No `volatile` for Synchronization**: Avoid using `volatile` for thread synchronization (a common mistake in C/Embedded). You must use **`std::atomic<T>`**.

## 6. Safety & Error Handling
- **Const-Correctness**: If a variable does not change, it must be `const`. If a class method does not modify the object's state, it must be marked `const`.
- **Compile-time Evaluation**: Maximize compile-time computation using `constexpr` and `consteval` to save runtime CPU cycles.
- **Minimize Exceptions**: In Embedded Linux, exceptions incur high overhead and cause non-deterministic behavior. Prefer returning errors using **`std::expected<T, Error>`** (C++23) instead of `throw`.

## 7. Interfaces & Libraries
- Limit `#include <iostream>`. Prefer dedicated logging libraries or `std::format` / `fmt::format` for string formatting.
- All utility classes must reside within a dedicated namespace, e.g., `namespace core::concurrency { ... }`.
