# Process Creation and Execution (Detailed)

## 1. Process Creation Mechanisms

### `fork()` and Copy-On-Write (COW)
- `fork()` creates a child process that is an almost exact duplicate of the parent process.
- **File Descriptor Sharing:** After `fork()`, parent and child have separate file descriptor tables. Corresponding inherited descriptors refer to the same kernel *open file description*, sharing its file offset and file status flags. Descriptor flags such as `FD_CLOEXEC` belong to each descriptor, not to that shared open file description. Close unnecessary inherited descriptors in each process.
- **Copy-On-Write (COW):** Linux commonly implements inherited private writable mappings using COW: parent and child initially share pages, and a write may trigger a private copy. `MAP_SHARED` mappings retain shared-update semantics. COW is an implementation strategy, not a POSIX requirement.

### `vfork()` Function
- On Linux, `vfork()` shares the parent's address space until the child successfully executes a new image or terminates, suspending the calling thread during that time. Other parent threads may continue running in the shared address space.
- > [!WARNING]  
  > The child must not return from the calling function, modify shared data except for the `pid_t` return-value variable, or call other functions before an `exec` function or `_exit()`; do not call `exit()`. Violations have undefined behavior under the historical specification. POSIX.1-2008 removed `vfork()`; prefer `posix_spawn()` or `fork()` and `exec()` rather than treating it as a general optimization.

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
  - `execvpe()` is a GNU extension requiring `_GNU_SOURCE` in glibc. Its executable search uses the caller's `PATH`, not the `PATH` in its `envp` argument; a filename containing a slash bypasses PATH lookup.
- After a successful `execve()`, caught signal dispositions are reset to default; ignored dispositions generally remain ignored. POSIX permits an exception for ignored `SIGCHLD`, which Linux retains.

### File Descriptors and the `close-on-exec` flag (`FD_CLOEXEC`)
- By default, open File Descriptors **remain open** after calling `exec()`. Shells exploit this to perform I/O Redirection.
- > [!IMPORTANT]
  > For security (preventing sensitive files from being exposed to the new program) and resource leak prevention, developers should set the `FD_CLOEXEC` flag for file descriptors (via `fcntl`). The kernel will automatically close this FD as soon as `exec()` succeeds.

### Warning regarding `system()`
- The `system(command)` function runs a command through a shell (e.g., `system("ls -l")`); the number of processes involved is not fixed, since commands can be shell builtins or executed using different shell strategies.
- > [!CAUTION]
  > **Never use `system()` in privileged programs (like Set-User-ID root)**. Attackers can manipulate the `PATH` or `IFS` environment variables to force your program to execute malicious code as root.
