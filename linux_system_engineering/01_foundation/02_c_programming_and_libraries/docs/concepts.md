# Core Concepts: C Programming and Libraries

This document covers essential system programming concepts and the management of libraries in Linux, based on *The Linux Programming Interface (TLPI)*. All examples and concepts here assume the usage of the GNU C Library (`glibc`) in a POSIX-compliant environment.

---

## 1. System Programming Concepts

### System Calls vs. Library Functions
- **System Call:** A controlled entry point into the kernel, allowing a process to request the kernel to perform a privileged action on its behalf. System calls change the processor state from User Mode to Kernel Mode.
- **Library Function:** A standard function provided by a library (like `glibc`). While some library functions (e.g., `strlen()`, `strcpy()`) do their work entirely in User Mode, others (e.g., `printf()`, `malloc()`) act as wrappers that eventually invoke one or more system calls.

### The Standard C Library (glibc)
The GNU C Library (`glibc`) is the core library for Linux system programming. It provides the system call wrappers, standard C functions, and POSIX thread (`pthread`) implementations.

### Error Handling in Linux
When a system call or library function fails, it typically returns a specific value (usually `-1` or `NULL`) and sets a global integer variable named `errno` to indicate the specific cause of the error.
- You MUST include `<errno.h>` to access `errno`.
- Always check the return value before checking `errno`.
- **Useful functions:**
  - `perror(const char *msg)`: Prints `msg` followed by the error description corresponding to `errno` to standard error.
  - `strerror(int errnum)`: Returns a pointer to the string describing the error number.

> **Warning:** Never assume `errno` is set to `0` upon success. A successful function call may leave `errno` unmodified or overwrite it with an internal value. Only evaluate `errno` when a function explicitly returns an error indicator.

---

## 2. Fundamentals of Shared Libraries

### Static vs. Shared Libraries
- **Static Libraries (`.a`):** An archive of object files. When a program is linked against a static library, the linker copies the required object code directly into the final executable.
  - *Pros:* Independent executable, execution speed.
  - *Cons:* Large executable size, requires recompilation if the library is updated.
- **Shared Libraries (`.so`):** The library code is loaded into memory only once and shared among all processes that use it.
  - *Pros:* Saves disk space and RAM, easy to upgrade without recompiling dependent applications.
  - *Cons:* Slightly slower execution due to dynamic symbol resolution, potential "dependency hell" if versions mismatch.

### Position-Independent Code (PIC)
To create a shared library, the object files must be compiled with the `-fPIC` flag. This generates code that can execute correctly regardless of where it is loaded in the process's virtual memory space.

### Library Naming Conventions (Soname)
Shared libraries typically have a *soname* (Shared Object Name), which is used to manage versioning. The format is usually `lib<name>.so.<major_version>`. The linker embeds the *soname* inside the executable, ensuring the program loads the compatible major version at run-time.

### Dynamic Linker and Search Path
At run-time, the dynamic linker (`ld.so`) searches for shared libraries in standard directories (e.g., `/lib`, `/usr/lib`). If your library is in a custom directory, you must inform the dynamic linker by:
- Setting the `LD_LIBRARY_PATH` environment variable.
- Or using the linker's `-rpath` flag during compilation.

---

## 3. Advanced Features of Shared Libraries

### Dynamically Loaded Libraries (dlopen API)
Instead of linking a library at compile-time, a program can load a shared library explicitly at run-time using the `dlopen` API. This is the standard mechanism for implementing **Plugins**.
- `dlopen()`: Loads a shared library into the process's memory.
- `dlsym()`: Looks up the memory address of a specific function (symbol) within the loaded library.
- `dlerror()`: Returns a string describing the last error that occurred in the `dl` API.
- `dlclose()`: Unloads the shared library.

### Controlling Symbol Visibility
By default, all functions in a shared library are visible (exported). To prevent name collisions and improve linking speed, you should hide internal functions:
- Use the compiler flag `-fvisibility=hidden`.
- Explicitly mark public functions with `__attribute__((visibility("default")))`.

> **Tip - Preloading Shared Libraries:** By setting the `LD_PRELOAD` environment variable, you can force the dynamic linker to load a specific shared library *before* any others. This powerful technique allows you to intercept and override standard functions (like `malloc` or `open`) without recompiling the original program!
