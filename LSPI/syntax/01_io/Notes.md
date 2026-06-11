# Linux File I/O: Raw System Calls vs Standard C Library

## 🎯 Introduction: Which Approach Should You Choose?

Before diving into the technical details, here is a practical rule of thumb for selecting the appropriate I/O method based on real-world use cases.

### Method 1: Raw System Calls (Unbuffered I/O)

**Purpose:**  
Communicate directly with the kernel. Data is transferred immediately without being held in an application-level buffer.

**Typical Use Cases:**

- Network communication (Sockets)
- Inter-process communication (Pipes, FIFOs)
- Hardware and device control (`/dev/tty`, GPIO, serial ports, etc.)
- Applications that implement their own buffering mechanisms (e.g., database engines such as MySQL and PostgreSQL)

**Characteristics:**

- Extremely low latency
- Higher CPU overhead when processing many small read/write operations due to frequent context switches
- POSIX/UNIX/Linux specific (not inherently portable across platforms)

---

### Method 2: Standard C Library (Buffered I/O)

**Purpose:**  
Provides a user-space buffer on top of system calls. Small read/write operations are automatically grouped into larger blocks before interacting with the kernel.

**Typical Use Cases:**

- Reading and writing regular disk files
- Formatted text processing and logging
- Cross-platform applications (Windows, Linux, macOS)

**Characteristics:**

- High throughput for large file operations
- Easier to use
- Reduces CPU overhead by minimizing system calls

---

# 1. Raw System Calls (The Universal I/O Model)

One of the core UNIX philosophies is:

> **"Everything is a file."**

The same set of system calls can be used to interact with files, devices, sockets, and many other resources.

---

## File Descriptors (fd)

The operating system identifies open files and I/O resources using non-negative integers called **File Descriptors (fd)**.

Every process starts with three predefined descriptors:

| File Descriptor | Name | Purpose |
|---------------|------|---------|
| 0 | `STDIN_FILENO` | Standard Input |
| 1 | `STDOUT_FILENO` | Standard Output |
| 2 | `STDERR_FILENO` | Standard Error |

---

## The Core Four System Calls

### A. Open a File: `open()`

```c
#include <fcntl.h>

int open(const char *pathname, int flags, mode_t mode);
```

**Common Flags**

| Flag | Description |
|--------|-------------|
| `O_RDONLY` | Read only |
| `O_WRONLY` | Write only |
| `O_RDWR` | Read and write |
| `O_CREAT` | Create file if it does not exist |
| `O_APPEND` | Append data at end of file |
| `O_TRUNC` | Truncate existing file |

**Return Value**

- File descriptor (`fd`) on success
- `-1` on failure

---

### B. Read Data: `read()`

```c
#include <unistd.h>

ssize_t read(int fd, void *buffer, size_t count);
```

Reads up to `count` bytes from the file descriptor into `buffer`.

**Return Value**

- Number of bytes actually read
- `0` if End-of-File (EOF) is reached
- `-1` on error

---

### C. Write Data: `write()`

```c
#include <unistd.h>

ssize_t write(int fd, const void *buffer, size_t count);
```

Requests the kernel to copy `count` bytes from the user buffer into the kernel's cache.

---

### D. Close a File: `close()`

```c
#include <unistd.h>

int close(int fd);
```

Releases the file descriptor and associated kernel resources.

---

## Atomic Operations and Advanced I/O

### `pread()` and `pwrite()`

These functions perform seeking and reading/writing as a single atomic operation.

They are especially important in multithreaded environments to avoid race conditions.

```c
pread(fd, buffer, length, offset);
pwrite(fd, buffer, length, offset);
```

---

### `O_SYNC`

Forces the kernel to flush data to physical storage before returning.

```c
open("data.bin", O_WRONLY | O_SYNC);
```

**Advantages**

- Maximum data integrity

**Disadvantages**

- Significantly slower performance

---

### `O_DIRECT`

Bypasses the kernel page cache and performs direct disk I/O.

```c
open("data.bin", O_WRONLY | O_DIRECT);
```

Typically used by databases and high-performance storage systems.

---

# 2. Standard C Library (Buffered I/O)

The `<stdio.h>` library uses **streams** (`FILE *`) instead of file descriptors.

Each `FILE *` internally contains:

- A file descriptor
- A user-space buffer
- State information

---

## Standard I/O Equivalents

### A. Open a Stream: `fopen()`

```c
#include <stdio.h>

FILE *fopen(const char *pathname, const char *mode);
```

**Common Modes**

| Mode | Description |
|--------|-------------|
| `"r"` | Read |
| `"w"` | Write (truncate file) |
| `"a"` | Append |
| `"r+"` | Read and write |
| `"w+"` | Read and write (truncate file) |
| `"a+"` | Read and append |

---

### B. Block-Based I/O: `fread()` and `fwrite()`

Useful when reading or writing binary arrays and structures.

```c
size_t fread(void *ptr,
             size_t size,
             size_t nmemb,
             FILE *stream);

size_t fwrite(const void *ptr,
              size_t size,
              size_t nmemb,
              FILE *stream);
```

---

### C. Formatted I/O: `fprintf()` and `fscanf()`

A capability that raw system calls do not provide.

```c
fprintf(stream, "Temperature: %d C\n", temp);
```

Useful for text processing and structured logging.

---

### D. Close a Stream: `fclose()`

```c
int fclose(FILE *stream);
```

Automatically flushes buffered data before closing the underlying file descriptor.

---

## Buffering Modes

The power of `stdio` comes from its buffering mechanism.

Buffering behavior can be configured using `setvbuf()`.

---

### `_IONBF` — Unbuffered

No buffering.

```c
setvbuf(stream, NULL, _IONBF, 0);
```

Commonly used for:

- `stderr`
- Real-time output

---

### `_IOLBF` — Line Buffered

Flushes automatically whenever a newline (`\n`) is encountered.

```c
setvbuf(stream, NULL, _IOLBF, 0);
```

Default behavior for terminal output (`stdout` connected to a terminal).

---

### `_IOFBF` — Fully Buffered

Flushes only when the buffer becomes full.

```c
setvbuf(stream, NULL, _IOFBF, BUFSIZ);
```

Default behavior for disk files.

Typical buffer sizes:

- 4 KB
- 8 KB

---

## Manual Buffer Flushing: `fflush()`

Forces buffered data in user space to be written to the kernel immediately.

```c
fflush(stdout);
```

Commonly used when output must appear immediately on screen.

---

# 3. Mixing System Calls and Standard I/O

In some situations, you may obtain a file descriptor from a system call (such as a socket), but still want to use convenient functions like `fprintf()`.

POSIX provides conversion functions for this purpose.

---

## Extracting a File Descriptor from a Stream

Use `fileno()`.

```c
int fd = fileno(stream);
```

This allows you to modify file descriptor properties using functions such as `fcntl()`.

---

## Creating a Stream from a File Descriptor

Use `fdopen()`.

```c
FILE *stream = fdopen(fd, "w");
```

Commonly used with:

- Sockets
- Pipes
- Existing file descriptors

---

# ⚠️ Danger Zone: Mixing Buffered and Unbuffered I/O

Mixing raw system calls (`read()`, `write()`) with standard I/O (`fread()`, `fprintf()`, etc.) on the same file can lead to unexpected behavior.

Because `stdio` uses an internal buffer, the order of data written may become inconsistent.

### Incorrect

```c
fprintf(stream, "Hello");
write(fd, "World", 5);
```

Possible output:

```text
WorldHello
```

instead of:

```text
HelloWorld
```

### Correct

Always flush the stream before switching to raw system calls.

```c
fprintf(stream, "Hello");
fflush(stream);

write(fd, "World", 5);
```

This guarantees the expected ordering of data.