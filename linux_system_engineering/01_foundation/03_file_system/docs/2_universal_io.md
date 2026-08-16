# Universal I/O Model

This document covers the Linux System Programming API for File I/O, based on *The Linux Programming Interface (TLPI)* Chapters 4 and 5.

---

## 1. The Universal I/O Concept

One of the distinguishing features of the UNIX/Linux I/O model is **Universality**. The exact same four system calls (`open`, `read`, `write`, `close`) are used to perform I/O on *all types of files*:
- Regular files
- Terminals (`/dev/tty`)
- Devices
- Pipes and Sockets

### File Descriptors
All system calls for I/O refer to open files using a **File Descriptor (fd)**, which is simply a non-negative integer. 
By convention, every process starts with three standard file descriptors:
- `0`: Standard Input (`STDIN_FILENO`)
- `1`: Standard Output (`STDOUT_FILENO`)
- `2`: Standard Error (`STDERR_FILENO`)

---

## 2. The Core API

### `open()`
```c
#include <fcntl.h>
int open(const char *pathname, int flags, ... /* mode_t mode */);
```
Opens or creates a file.
- **`flags`**: Defines access mode (`O_RDONLY`, `O_WRONLY`, `O_RDWR`). Can be bitwise OR'd with creation flags like `O_CREAT` (create if doesn't exist), `O_TRUNC` (truncate to 0 bytes), or `O_APPEND` (always write to the end).
- **`mode`**: Required only if `O_CREAT` is used. Specifies the file permissions (e.g., `S_IRUSR | S_IWUSR`).

### `read()`
```c
#include <unistd.h>
ssize_t read(int fd, void *buffer, size_t count);
```
Reads up to `count` bytes from the file referred to by `fd` into the `buffer`.
- Returns the number of bytes read, `0` on End-of-File (EOF), or `-1` on error.

### `write()`
```c
#include <unistd.h>
ssize_t write(int fd, const void *buffer, size_t count);
```
Writes up to `count` bytes from the `buffer` to the file referred to by `fd`.
- Always check the return value, as a partial write (returning less than `count`) is possible.

### `close()`
```c
#include <unistd.h>
int close(int fd);
```
Releases the file descriptor. It is a critical embedded standard to always `close()` file descriptors to prevent resource leaks.

---

## 3. Advanced I/O Features

### Changing File Offset: `lseek()`
Every open file has an associated "file offset" (the position where the next `read` or `write` will occur).
```c
#include <unistd.h>
off_t lseek(int fd, off_t offset, int whence);
```
- `whence` can be:
  - `SEEK_SET`: Offset is set to `offset` bytes from the beginning.
  - `SEEK_CUR`: Offset is set to its current location plus `offset`.
  - `SEEK_END`: Offset is set to the size of the file plus `offset`.

> [!TIP]
> If you `lseek` past the end of a file and write data, a "hole" is created. These holes do not consume actual disk space until data is written to them. Such files are called **Sparse Files**.

### Atomicity and Race Conditions
A race condition occurs when two processes try to write to the same file simultaneously. 
For example, if two processes want to append data, reading the end of the file with `lseek()` and then calling `write()` is **not** atomic. Another process might write data in between those two calls.
- **Solution:** Use the `O_APPEND` flag when calling `open()`. The kernel guarantees that appending data is an atomic operation.
- **Alternative:** Use `pread()` and `pwrite()`, which perform I/O at a specific offset without modifying the global file offset, ensuring thread-safe atomic I/O.

### File Descriptor Manipulation: `fcntl()`
The `fcntl()` system call performs various control operations on an open file descriptor.
```c
#include <fcntl.h>
int fcntl(int fd, int cmd, ...);
```
Common uses:
- `F_GETFL` / `F_SETFL`: Get or set the file status flags (e.g., turning on `O_NONBLOCK` for non-blocking I/O dynamically).
- `F_DUPFD`: Duplicate a file descriptor.

### Duplicating File Descriptors: `dup()` and `dup2()`
Sometimes we want to redirect standard input or output to a file. We do this by duplicating file descriptors.
```c
#include <unistd.h>
int dup(int oldfd);
int dup2(int oldfd, int newfd);
```
`dup2()` makes `newfd` point to the exact same open file description as `oldfd`. If `newfd` was previously open, it is silently closed first. This is the mechanism used by the shell to implement I/O redirection (e.g., `command > file.txt`).
