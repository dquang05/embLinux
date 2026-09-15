# Pipes and Named Pipes (FIFOs)

Pipes provide a unidirectional byte-stream channel for communicating between processes on the same host.

---

## 1. Ordinary Pipes (Anonymous Pipes)
- **Nature**: Unidirectional (half-duplex). Byte stream without message boundaries.
- **Requirement**: Communicating processes must be related (typically parent-child or sibling processes spawned via `fork()`).
- **API (UNIX)**:
  - `pipe(int pipefd[2])`: `pipefd[0]` is the read end; `pipefd[1]` is the write end.
  - `pipe2(int pipefd[2], int flags)`: Linux-specific call supporting flags like `O_CLOEXEC` and `O_NONBLOCK` atomically upon creation.
  - `popen(const char *command, const char *type)` / `pclose(FILE *stream)`: High-level glibc helper functions that invoke `/bin/sh`, set up a pipe to `stdin` or `stdout`, and return a `FILE *` stream.
- **Lifecycle & Persistence**:
  - **Process persistence**: The pipe exists in kernel memory only as long as at least one descriptor referring to either end remains open.
  - **EOF Detection**: A reader receives an End-Of-File (return value `0` from `read()`) only when **all write descriptors** referencing the pipe across all processes are closed. For this reason, processes **must immediately close unused ends** after `fork()`.
- **Broken Pipe Handling (`SIGPIPE` & `EPIPE`)**:
  - Writing to a pipe whose read end has been closed by all readers generates a `SIGPIPE` signal for the writing process.
  - If `SIGPIPE` is ignored (`signal(SIGPIPE, SIG_IGN)`), the `write()` call returns `-1` with `errno = EPIPE`. Robust system programming always accounts for this to prevent abrupt process termination.

---

## 2. Named Pipes (FIFOs)
- **Nature**: Unidirectional byte stream possessing a path name within the filesystem directory hierarchy.
- **Requirement**: **No relationship required**. Any two unrelated processes with proper filesystem permissions can communicate.
- **API (UNIX)**:
  - `mkfifo(const char *pathname, mode_t mode)`: Creates the FIFO file node.
  - Accessed using standard I/O calls: `open()`, `read()`, `write()`, `close()`.
  - Removed from the filesystem using `unlink(pathname)`.

### 2.1 Opening Semantics and Synchronization
Opening a FIFO enforces rendezvous synchronization between reading and writing processes:

| Open Flag | Peer State | Blocking Behavior (`O_NONBLOCK` not set) | Non-blocking Behavior (`O_NONBLOCK` set) |
| :--- | :--- | :--- | :--- |
| `O_RDONLY` | No writer open | Blocks until a writer opens the FIFO | Returns success immediately |
| `O_WRONLY` | No reader open | Blocks until a reader opens the FIFO | **Fails immediately** returning `-1` with `errno = ENXIO` |

### 2.2 Atomicity and Capacity (`PIPE_BUF` & `F_SETPIPE_SZ`)
- **Write Atomicity**:
  - POSIX guarantees that writes of up to `PIPE_BUF` bytes (defined as 4096 bytes on Linux in `<limits.h>`) are **atomic**.
  - If multiple processes write concurrently to the same FIFO in chunks $\le \text{PIPE\_BUF}$, their data will never be interleaved. Writes larger than `PIPE_BUF` may be split and interleaved with other writers.
- **Buffer Capacity**:
  - Since Linux 2.6.11, the default pipe buffer capacity is 65,536 bytes (64 KB / 16 pages).
  - Can be queried or resized at runtime using `fcntl(fd, F_GETPIPE_SZ)` and `fcntl(fd, F_SETPIPE_SZ, new_size)`.

---

## 3. Comparison: Anonymous Pipe vs Named Pipe (FIFO)

| Feature | Anonymous Pipe | Named Pipe (FIFO) |
| :--- | :--- | :--- |
| **Filesystem Presence** | None (kernel memory object only) | Exists as a FIFO special file (`prw-r--r--`) |
| **Process Relationship** | Requires ancestor/sibling relationship | Can connect arbitrary, unrelated processes |
| **Identification** | Inherited file descriptors (`int fd[2]`) | Filesystem pathname (e.g. `/tmp/my_fifo`) |
| **Persistence** | Process persistence (dies when all FDs closed)| Filesystem persistence (node remains until `unlink`) |
| **Duplex** | Unidirectional | Unidirectional (open read or write) |
