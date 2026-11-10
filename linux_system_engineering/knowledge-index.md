# Linux systems knowledge index

All links resolve relative to this file. Topics for modules marked `not-reviewed` are discovery hints based on their names and file inventory, not claims about their contents or correctness. Capability owners are candidates, not implemented skills.

Status: `not-reviewed` means no documented technical review is known; `reviewed` means review and approved corrections were completed but verification is not exhaustive; `verified` is reserved for individual claims backed by explicit evidence, not entire modules. A reviewed module can contain unverified claims and intentional simplifications.

| Module | Topics and candidate capability | Documentation and examples | Status |
| --- | --- | --- | --- |
| [01_linux_basic_knowledge](01_foundation/01_linux_basic_knowledge/) | Linux basics; owner: cross-cutting/unassigned | [concepts](01_foundation/01_linux_basic_knowledge/docs/concepts.md), [main.c](01_foundation/01_linux_basic_knowledge/src/main.c) | not-reviewed |
| [02_c_programming_and_libraries](01_foundation/02_c_programming_and_libraries/) | C, libraries, dynamic loading, error handling; owner: cross-cutting/unassigned | [concepts](01_foundation/02_c_programming_and_libraries/docs/concepts.md), [src](01_foundation/02_c_programming_and_libraries/src/), [include](01_foundation/02_c_programming_and_libraries/include/) | not-reviewed |
| [03_file_system](01_foundation/03_file_system/) | VFS, links, I/O, ACL, inotify; owner: linux-system-io | [docs](01_foundation/03_file_system/docs/), [src](01_foundation/03_file_system/src/) | not-reviewed |
| [04_process](01_foundation/04_process/) | process lifecycle, exec, wait, descriptors, scheduling; owner: linux-process-control | [detailed entries below](#04_process) | reviewed |
| [05_thread](01_foundation/05_thread/) | threads, synchronization, deadlocks, pthreads; owner: linux-concurrency | [docs](01_foundation/05_thread/docs/), [src](01_foundation/05_thread/src/) | not-reviewed |
| [06_systemd_and_daemons](02_advanced_ipc/06_systemd_and_daemons/) | systemd, daemon; owner: linux-process-control candidate | [concepts](02_advanced_ipc/06_systemd_and_daemons/docs/concepts.md), [daemon example](02_advanced_ipc/06_systemd_and_daemons/src/01_simple_daemon.c) | not-reviewed |
| [07_signal](02_advanced_ipc/07_signal/) | signals, masks, sigaction, signalfd; owner: linux-process-control for lifecycle signals | [concepts](02_advanced_ipc/07_signal/docs/concepts.md), [src](02_advanced_ipc/07_signal/src/) | not-reviewed |
| [08_socket](02_advanced_ipc/08_socket/) | sockets, distributed systems; owner: linux-ipc-networking | [docs](02_advanced_ipc/08_socket/docs/), [src directory](02_advanced_ipc/08_socket/src/) | not-reviewed |
| [09_ipc_mechanisms](02_advanced_ipc/09_ipc_mechanisms/) | pipes, semaphores, shared memory, messages, sockets; owner: linux-ipc-networking | [docs](02_advanced_ipc/09_ipc_mechanisms/docs/), [src](02_advanced_ipc/09_ipc_mechanisms/src/) | not-reviewed |
| [10_memory_management](02_advanced_ipc/10_memory_management/) | memory, mmap, mlock; owner: linux-system-io candidate | [docs](02_advanced_ipc/10_memory_management/docs/), [src](02_advanced_ipc/10_memory_management/src/) | not-reviewed |
| [11_time_and_timers](03_system_and_io/11_time_and_timers/) | time, POSIX timers, timerfd; owner: linux-system-io candidate | [concepts](03_system_and_io/11_time_and_timers/docs/concepts.md), [src](03_system_and_io/11_time_and_timers/src/) | not-reviewed |
| [12_io_model](03_system_and_io/12_io_model/) | blocking I/O, select, poll, epoll; owner: linux-system-io | [docs](03_system_and_io/12_io_model/docs/), [src](03_system_and_io/12_io_model/src/) | not-reviewed |
| [13_debug_tools](03_system_and_io/13_debug_tools/) | GDB, Valgrind, heaptrack; owner: linux-debugging | [docs](03_system_and_io/13_debug_tools/docs/), [src directory](03_system_and_io/13_debug_tools/src/) | not-reviewed |

## 04_process

Status: **reviewed**, not verified. The previous Codex report records a technical review and approved corrections, compilation with `-Wall -Wextra -Werror -g`, passing normal runtime tests, and a passing clean parallel build. Selected EOF, `EINTR`, `EPIPE`, and exec failure paths remain untested. These are reported results, not independent verification during index creation.

Read only what the task needs:

- [Process concepts](01_foundation/04_process/docs/concepts.md): process lifecycle, IPC, job control, and scheduling overview.
- [Process creation and execution](01_foundation/04_process/docs/process_creation_execution.md): `fork`, COW, `vfork`, `clone`, `exec`, descriptors, and `system`.
- [CPU scheduling](01_foundation/04_process/docs/cpu_scheduling.md): scheduling and priorities.
- [Process resources](01_foundation/04_process/docs/process_resources.md): resource usage and limits.
- [process_management.c](01_foundation/04_process/src/process_management.c): `fork`, `execve`, `waitpid`, and status example.
- [ipc_pipe.c](01_foundation/04_process/src/ipc_pipe.c): parent-child pipe example.
- [Makefile](01_foundation/04_process/Makefile) and [module README](01_foundation/04_process/README.md): local build and module entry point.

Do not promote a claim to `verified` without recording its scope and supporting evidence separately.
