# I. Anonymous Pipes

## 1. Nature and Technical Characteristics

### Concept

An anonymous pipe is a kernel-managed buffer located in RAM. Since Linux 2.6.11, the default pipe capacity is typically **64 KB**.

### Key Characteristics

#### Unidirectional Communication

A pipe transfers data in only one direction:

- One end is dedicated to writing.
- One end is dedicated to reading.

#### Byte Stream

A pipe is a byte-stream communication channel:

- No message boundaries are preserved.
- Data is delivered in FIFO (First-In, First-Out) order.
- Random access is not supported.
- Calling `lseek()` on a pipe always fails.

#### Communication Scope

Anonymous pipes can only be used between **related processes**, such as:

- Parent ↔ Child
- Sibling ↔ Sibling

All communicating processes must inherit the pipe from a common ancestor that created it before calling `fork()`.

---

## 2. System Call and Descriptor Management

### Pipe Creation

```c
int pipe(int pfd[2]);
```

Return value:

- `0` on success
- `-1` on failure

Descriptors:

```text
pfd[0] -> Read end
pfd[1] -> Write end
```

### Mandatory Descriptor Closing Rules

Immediately after `fork()`:

#### Writer Process

Must close the read end:

```c
close(pfd[0]);
```

#### Reader Process

Must close the write end:

```c
close(pfd[1]);
```

### Consequences of Forgetting to Close Unused Ends

#### Reader Keeps a Write Descriptor Open

If the reader accidentally keeps a write descriptor open:

```text
read()
```

will block forever instead of returning:

```text
0 (EOF)
```

even after the actual writer process has terminated.

#### Writer Keeps a Read Descriptor Open

If the writer keeps a read descriptor open:

- It may never receive `SIGPIPE`.
- `write()` may not fail with `EPIPE`.
- The writer can become blocked when the pipe buffer fills up.

---

## 3. Connecting Filters and Using `popen()`

### Descriptor Redirection with `dup2()`

The `dup2()` system call is used to redirect standard streams into pipe endpoints.

```c
dup2(oldfd, newfd);
```

Typical usage:

```c
dup2(pfd[1], STDOUT_FILENO);
dup2(pfd[0], STDIN_FILENO);
```

Defensive programming is recommended:

```c
if (pfd[1] != STDOUT_FILENO)
{
    dup2(pfd[1], STDOUT_FILENO);
}
```

This protects against situations where standard descriptors were previously closed.

---

### High-Level Interface: `popen()` and `pclose()`

```c
FILE *popen(const char *command, const char *mode);
```

Internally, `popen()` automatically performs:

```text
pipe()
    ↓
fork()
    ↓
dup2()
    ↓
exec("/bin/sh -c command")
```

It returns a high-level `FILE *` stream in user space.

---

### Buffering Behavior

Since the stream is not connected to a terminal, the C library usually applies:

```text
Block Buffering
```

Therefore, writers should explicitly flush data:

```c
fflush(fp);
```

or disable buffering entirely:

```c
setbuf(fp, NULL);
```

to prevent communication stalls.

---

### Security Warning

Never use `popen()` in privileged (Set-UID) programs.

User-supplied input must always be validated using a whitelist approach to prevent:

```text
Shell Injection Attacks
```

---

### Safe Child Termination

If `exec()` fails inside the child process, terminate using:

```c
_exit(127);
```

instead of:

```c
exit(127);
```

This prevents inherited stdio buffers from being flushed twice.

---

# II. FIFOs (Named Pipes)

## 1. Differences from Anonymous Pipes

### Filesystem-Based Identification

A FIFO has a pathname in the filesystem.

Example:

```text
/tmp/myfifo
```

The actual data still resides in kernel RAM.

The filesystem entry merely serves as an access point.

---

### Communication Between Unrelated Processes

Unlike anonymous pipes, FIFOs allow communication between completely independent processes.

Examples:

- Client ↔ Server
- Two unrelated applications
- Processes started at different times

As long as both processes open the same FIFO pathname, communication is possible.

---

## 2. Creation and Blocking Open Behavior

### Shell Command

```bash
mkfifo -m 0666 /path/to/fifo
```

A FIFO appears with:

```text
p
```

as the first character in:

```bash
ls -l
```

Example:

```text
prw-rw-rw-
```

---

### C API

```c
int mkfifo(const char *pathname, mode_t mode);
```

---

### Default Blocking Semantics

#### Open for Reading

```c
open(path, O_RDONLY);
```

blocks until another process opens the FIFO for writing.

#### Open for Writing

```c
open(path, O_WRONLY);
```

blocks until another process opens the FIFO for reading.

---

### Important Warning

Avoid using:

```c
O_RDWR
```

as a workaround for blocking.

Reasons:

- Not portable according to POSIX.
- Prevents readers from ever receiving EOF.

---

# III. Advanced Client-Server Architecture and Non-Blocking I/O

## 1. FIFO-Based Client-Server Design

### Server FIFO (Well-Known FIFO)

The server owns a fixed FIFO path.

Example:

```text
/tmp/server_fifo
```

All clients send requests through this FIFO.

---

### Client FIFO (Private FIFO)

Each client creates a unique FIFO.

Example:

```text
/tmp/seqnum_cl.<PID>
```

The client includes its PID in the request.

The server uses this PID to locate the correct FIFO and send the response.

Benefits:

- No contention among clients.
- Responses are delivered directly to the requesting client.

---

### Preventing EOF on the Server

An iterative server usually opens an additional write descriptor to its own FIFO:

```c
open(server_fifo, O_WRONLY);
```

This guarantees that:

```text
Number of writers > 0
```

at all times.

As a result:

- The server waits for future clients.
- EOF is never generated when existing clients disconnect.

---

## 2. Non-Blocking I/O Fundamentals (`O_NONBLOCK`)

The behavior of system calls changes significantly when:

```c
O_NONBLOCK
```

is enabled.

---

### Effects on `open()`

#### Read Open

```c
open(path, O_RDONLY | O_NONBLOCK);
```

Succeeds immediately, even if no writer exists.

#### Write Open

```c
open(path, O_WRONLY | O_NONBLOCK);
```

Fails immediately if no reader exists.

Error:

```text
ENXIO
```

---

### Effects on `read()`

If:

- the pipe is empty
- at least one writer still exists

then:

```c
read()
```

returns:

```text
-1
```

with:

```text
EAGAIN
```

or

```text
EWOULDBLOCK
```

instead of blocking.

---

### Effects on `write()`

#### Atomic Writes (`PIPE_BUF = 4096 bytes`)

For writes:

```text
size <= PIPE_BUF
```

the kernel guarantees atomicity.

If sufficient space exists:

```text
Entire write succeeds.
```

If even one byte is missing:

```text
Entire write fails with EAGAIN.
```

No partial write occurs.

---

#### Large Writes (`size > PIPE_BUF`)

Atomicity is not guaranteed.

The kernel may:

```text
Write only part of the data.
```

and return the actual number of bytes written.

If the pipe is completely full:

```text
EAGAIN
```

is returned.

---

## 3. Industrial-Grade Design Practices

High-performance systems do not create thousands of processes or threads solely for blocking I/O operations.

Problems include:

- Excessive RAM usage
- High context-switch overhead
- Poor scalability

---

### Modern Architecture

All FIFOs and sockets are configured as:

```c
O_NONBLOCK
```

and registered with an I/O multiplexing mechanism such as:

```text
epoll (Linux)
```

The architecture becomes:

```text
Event-Driven
```

instead of:

```text
Thread-Per-Connection
```

---

### Typical Workflow

```text
Kernel detects data availability
            ↓
Kernel wakes event loop
            ↓
epoll reports ready descriptor
            ↓
Application calls read()
            ↓
Process data
```

This design minimizes:

- CPU usage
- Memory consumption
- Context-switch overhead

while maximizing scalability and throughput.