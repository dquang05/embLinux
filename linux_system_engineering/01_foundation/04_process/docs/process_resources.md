# Process Resource Management and Limits

Every running process consumes system resources (RAM, CPU, files, sockets, etc.). The operating system provides mechanisms to measure and limit this consumption to ensure stability and security.

## 1. Checking Resource Usage
The `getrusage()` function allows a process to retrieve statistics regarding the resources consumed by itself or its **child processes**.

```c
#include <sys/resource.h>
int getrusage(int who, struct rusage *res_usage);
```
The `who` parameter can be `RUSAGE_SELF` or `RUSAGE_CHILDREN` (for children that have been `wait()`ed on).
Key metrics collected include:
- CPU time spent in User mode and Kernel mode (`ru_utime`, `ru_stime`).
- Page faults (Soft/Hard Page Faults).
- Number of context switches.

## 2. Resource Limits
The OS imposes limits on each type of resource. Developers use `getrlimit()` and `setrlimit()` (or the `ulimit` shell command) to manipulate them.

Each resource type has 2 limits:
1. **Soft limit:** The actual limit enforced by the kernel. The process can increase or decrease this value, provided it does not exceed the Hard limit.
2. **Hard limit:** The "ceiling" for the soft limit. An unprivileged process can lower its hard limit but cannot raise it again. On Linux, raising it requires `CAP_SYS_RESOURCE` in the initial user namespace, subject to resource-specific limits.

### Common Resource Limits
- **`RLIMIT_AS` (Virtual Memory Size):** Limits virtual address space. When an allocation requiring additional address space exceeds the limit, `malloc()` returns `NULL` or `mmap()` returns `MAP_FAILED`, with `errno` set to `ENOMEM` on Linux. An allocation satisfied from existing memory need not increase address space.
- **`RLIMIT_CORE` (Core file size):** Maximum size of a core dump file (a memory snapshot upon crash).
  - > [!TIP]
    > This is often set to `0` (disabling core dumps) to save disk space and protect sensitive data in RAM from exposure during crashes.
- **`RLIMIT_CPU` (CPU Time):** Maximum CPU time the process is allowed to run. If exceeded, the process receives a `SIGXCPU` signal. If it continues to run past the hard limit, it receives `SIGKILL`.
- **`RLIMIT_FSIZE` (File Size):** Maximum file size the process may create or extend. An attempt to exceed it generates `SIGXFSZ`; if the signal is caught or ignored, the relevant system call can fail with `EFBIG`. The default signal action terminates the process.
- **`RLIMIT_NOFILE` (File descriptors):** One greater than the maximum file descriptor number that can be opened by the process, not the number currently open plus one. Exceeding it normally fails with `EMFILE`.
- **`RLIMIT_NPROC` (Linux/BSD extension):** On Linux, limits the number of extant threads associated with the caller's real UID, rather than a lifetime count of process creations. It is not specified by POSIX.
  - > [!IMPORTANT]
    > This limit can help constrain **Fork Bombs**: when enforced and reached, `fork()` fails with `EAGAIN`. Linux does not enforce it for real UID 0 or processes with `CAP_SYS_ADMIN` or `CAP_SYS_RESOURCE`.
