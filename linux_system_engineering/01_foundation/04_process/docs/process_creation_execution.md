# Process Creation and Execution (Detailed)

## 1. Process Creation Mechanisms

### `fork()` and Copy-On-Write (COW)
- `fork()` creates a child process that is an almost exact duplicate of the parent process.
- **File Descriptor Sharing:** The child receives copies of all open file descriptors from the parent. Both point to the same *open file description* table. To avoid I/O conflicts, unnecessary file descriptors should be closed in the child if not shared.
- **Copy-On-Write (COW):** For performance optimization, modern OSes do not copy the entire RAM immediately upon `fork()`. Instead, parent and child point to the same physical memory pages with *read-only* permissions. Only when one attempts to write/modify a variable does the kernel actually copy that specific page to a private memory area for that process.

### `vfork()` Function
- An optimized version designed specifically for the scenario where a process is created and `exec()` is called immediately after.
- The child process **completely shares** the parent's memory space. The parent process is **blocked** until the child calls `exec()` or terminates.
- > [!WARNING]  
  > If the child modifies a variable or returns from a function using `vfork()`, it will corrupt the parent's memory. Extremely dangerous; use only when micro-optimization is strictly required.

### `clone()` Function (Linux Specific)
- Similar to `fork()`, but `clone()` allows developers extremely fine-grained control over what the parent and child will share (via `flags`).
- This is the foundation for libraries (like NPTL) to create **Threads** on Linux. When `clone()` is called with flags like `CLONE_VM` (share RAM), `CLONE_FILES` (share file descriptors), and `CLONE_THREAD` (share thread group), the resulting child process is effectively a thread.

## 2. Program Execution

### The `exec()` Family of Functions
- The `execve(pathname, argv, envp)` system call discards the current memory space (text, data, heap, stack) and loads a new program to replace it. **The PID remains unchanged**.
- Based on `execve`, the C library provides convenient wrappers:
  - `execl`, `execlp`, `execle` (**l**: accepts arguments as a list).
  - `execv`, `execvp`, `execvpe` (**v**: accepts arguments as a vector/array).
  - **p**: Automatically searches for the executable in the `PATH` environment variable.
  - **e**: Allows passing a custom environment variable array.

### File Descriptors and the `close-on-exec` flag (`FD_CLOEXEC`)
- By default, open File Descriptors **remain open** after calling `exec()`. Shells exploit this to perform I/O Redirection.
- > [!IMPORTANT]
  > For security (preventing sensitive files from being exposed to the new program) and resource leak prevention, developers should set the `FD_CLOEXEC` flag for file descriptors (via `fcntl`). The kernel will automatically close this FD as soon as `exec()` succeeds.

### Warning regarding `system()`
- The `system(command)` function is convenient for running shell commands (e.g., `system("ls -l")`), but it creates at least 2 processes (1 for the `/bin/sh` shell, 1 for the command).
- > [!CAUTION]
  > **Never use `system()` in privileged programs (like Set-User-ID root)**. Attackers can manipulate the `PATH` or `IFS` environment variables to force your program to execute malicious code as root.
