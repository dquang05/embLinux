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
2. **Hard limit:** The "ceiling" for the soft limit. Normal users can only *decrease* the hard limit (and cannot increase it again). Only privileged users (root) have the authority to increase the hard limit.

### Common Resource Limits
- **`RLIMIT_AS` (Virtual Memory Size):** Maximum virtual memory size. When exceeded, dynamic allocation functions like `malloc()` or `mmap()` will fail and return `ENOMEM`.
- **`RLIMIT_CORE` (Core file size):** Maximum size of a core dump file (a memory snapshot upon crash).
  - > [!TIP]
    > This is often set to `0` (disabling core dumps) to save disk space and protect sensitive data in RAM from exposure during crashes.
- **`RLIMIT_CPU` (CPU Time):** Maximum CPU time the process is allowed to run. If exceeded, the process receives a `SIGXCPU` signal. If it continues to run past the hard limit, it receives `SIGKILL`.
- **`RLIMIT_FSIZE` (File Size):** Largest file size that can be created. Exceeding this causes `write()` to fail with `EFBIG` and the process receives a `SIGXFSZ` signal.
- **`RLIMIT_NOFILE` (Number of open files):** Maximum number of open file descriptors (plus 1). This limit must be increased on application servers handling many concurrent socket connections.
- **`RLIMIT_NPROC` (Number of processes):** Maximum number of processes a *single user* can create.
  - > [!IMPORTANT]
    > This limit is critical for defending against **Fork Bomb** attacks (malicious code that continuously forks child processes until the system hangs). Once the limit is reached, `fork()` fails with `EAGAIN`.
