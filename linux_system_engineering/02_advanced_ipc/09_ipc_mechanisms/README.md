# Chapter 09: Interprocess Communication (IPC) Mechanisms

## Objectives
- Master all major Linux Interprocess Communication mechanisms: Pipes, FIFOs, Message Queues, Semaphores, Shared Memory, and Sockets.
- Contrast POSIX and System V IPC paradigms and understand when to choose each.
- Understand deep memory mapping concepts: Shared vs Private mappings, Anonymous vs File-backed mappings, and Copy-On-Write (COW).
- Build bidirectional full-duplex communication channels using `socketpair()`.

## References
- **TLPI (The Linux Programming Interface)**:
  - Chapters 43–48: Pipes, FIFOs, and System V IPC (Message Queues, Semaphores, Shared Memory)
  - Chapters 51–54: POSIX IPC (POSIX Message Queues, Semaphores, Shared Memory)
  - Chapter 49–50: Memory Mappings (`mmap`)
- **OSC (Operating System Concepts)**:
  - Chapter 3.4 – 3.8: IPC Models (Shared Memory and Message Passing)
- **Cross-Reference**:
  - Chapter 08: Sockets and Network Programming ([08_socket](../08_socket/docs/concepts.md))

## Documentation Guide
- [Core Concepts & Taxonomy](docs/concepts.md)
- [Pipes and Named Pipes (FIFOs)](docs/pipes.md)
- [Message Passing (System V & POSIX Message Queues)](docs/message_passing.md)
- [Semaphores & Synchronization (System V & POSIX)](docs/semaphores.md)
- [Shared Memory & Memory Mapping (Anonymous, COW, POSIX, SysV)](docs/shared_memory.md)
- [Sockets & IPC Comparison Matrix](docs/sockets_and_rpc.md)

## Source Code Demonstrations (`src/`)

| File | Description | Key APIs Used |
| :--- | :--- | :--- |
| `01_pipe_demo.c` | Unidirectional anonymous pipe between parent and child | `pipe()`, `read()`, `write()` |
| `02_msgq_demo.c` | System V Message Queue with typed message passing | `msgget()`, `msgsnd()`, `msgrcv()`, `msgctl()` |
| `03_sem_demo.c` | System V Semaphore set with `SEM_UNDO` | `semget()`, `semctl()`, `semop()` |
| `04_shm_demo.c` | System V Shared Memory (`IPC_PRIVATE`) with semaphores | `shmget()`, `shmat()`, `shmdt()`, `shmctl()` |
| `05_posix_mq_demo.c`| POSIX Message Queue with priority ordering | `mq_open()`, `mq_send()`, `mq_receive()`, `mq_unlink()` |
| `06_posix_sem_demo.c`| POSIX Named Semaphore for mutual exclusion | `sem_open()`, `sem_wait()`, `sem_post()`, `sem_unlink()` |
| `07_posix_shm_demo.c`| POSIX Shared Memory object backed by `/dev/shm` | `shm_open()`, `ftruncate()`, `mmap()`, `munmap()` |
| `08_anonymous_shm_demo.c`| Anonymous Shared Memory, COW proof & process-shared unnamed semaphore | `mmap(MAP_SHARED/PRIVATE \| MAP_ANONYMOUS)`, `sem_init(pshared=1)` |
| `09_fifo_demo.c` | Named Pipe (FIFO) stream IPC with filesystem node | `mkfifo()`, `open()`, `read()`, `write()`, `unlink()` |
| `10_socketpair_demo.c`| Full-duplex bidirectional stream IPC between related processes | `socketpair(AF_UNIX, SOCK_STREAM)`, `read()`, `write()` |

## Build and Run Instructions

To compile all examples into the `build/` directory:
```bash
make clean
make
```

To run the demos:
```bash
./build/08_anonymous_shm_demo
./build/09_fifo_demo
./build/10_socketpair_demo
```
