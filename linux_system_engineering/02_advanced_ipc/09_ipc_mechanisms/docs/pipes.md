# Pipes

Pipes act as a conduit allowing two processes to communicate. 

## 1. Ordinary Pipes (Anonymous Pipes)
- **Nature**: Unidirectional (one-way). Half-duplex. Byte stream (no message boundaries).
- **Requirement**: Requires a parent-child relationship between communicating processes.
- **Usage**: Used for standard producer-consumer fashion on the same machine.
- **API (UNIX)**: 
  - `pipe(int fd[])`: `fd[0]` is read end, `fd[1]` is write end.
  - `popen()` and `pclose()`: High-level functions to execute a shell command and read its output or send input to it via a pipe.
  - Accessed via standard `read()` and `write()`.
- **Atomicity**: Writes of up to `PIPE_BUF` bytes (e.g., 4096 bytes on Linux) are guaranteed to be atomic. Larger writes may be interleaved with other writers.
- **Lifecycle**: Ceases to exist when all communicating processes close their file descriptors (Process persistence). Unused pipe file descriptors must be closed to ensure processes can correctly detect end-of-file (EOF) or broken pipe (`SIGPIPE`).

## 2. Named Pipes (FIFOs)
- **Nature**: Bidirectional (but typically half-duplex in UNIX, full-duplex in Windows). Byte stream.
- **Requirement**: No parent-child relationship required. Can have multiple writers.
- **Usage**: Can be used by unrelated processes. Windows named pipes can even communicate over a network.
- **API (UNIX)**: 
  - Created with `mkfifo()`.
  - Manipulated with `open()`, `read()`, `write()`, `close()`.
  - **Blocking Semantics**: By default, `open()` for reading blocks until another process opens for writing, and vice-versa. This synchronizes the reader and writer.
- **Lifecycle**: Persists in the file system until explicitly deleted.
