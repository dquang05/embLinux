# Advanced Optimizations & High-Performance I/O Patterns

This document covers specialized I/O optimization techniques in the Linux Kernel, tailored for high-concurrency architectures (C10K / C1000K), real-time streaming services, and memory-constrained embedded systems.

---

## 1. Direct I/O (`O_DIRECT`): Bypassing the Kernel Page Cache

### 1.1 When to Use Direct I/O
Linux Page Cache accelerates most standard applications by caching file data in RAM. However, Direct I/O becomes advantageous in two specific scenarios:
1. **Custom Database / Storage Engines (MySQL InnoDB, RocksDB, PostgreSQL)**: These engines maintain specialized user-space buffer pools with customized caching and eviction algorithms (e.g., adaptive LRU). Routing through Page Cache results in **Double Caching**, wasting physical RAM.
2. **Bulk Streaming Transfers**: Streaming multi-gigabyte media files or performing raw disk imaging. Pouring these one-pass workloads through Page Cache evicts active, latency-critical pages of other applications (Page Thrashing).

### 1.2 Strict Memory Alignment Restrictions
Opening a file with `O_DIRECT` imposes three mandatory alignment constraints:
- **Buffer Address**: The user-space buffer pointer must be aligned to a multiple of the underlying device's physical/logical block size (commonly 512 or 4096 bytes; allocated via `posix_memalign()`).
- **File Offset**: Seek positions must align to block boundaries.
- **Data Transfer Length**: Read/write byte counts must be integer multiples of the block size.
- *Any violation causes `read()` / `write()` to immediately fail with `-1 (EINVAL)`.*

```c
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define BLOCK_ALIGNMENT 4096
#define TRANSFER_SIZE   4096

int main() {
    int fd = open("/tmp/direct_sample.dat", O_RDWR | O_CREAT | O_DIRECT, 0644);
    if (fd == -1) { perror("open O_DIRECT"); return 1; }

    void *aligned_buf = NULL;
    if (posix_memalign(&aligned_buf, BLOCK_ALIGNMENT, TRANSFER_SIZE) != 0) {
        perror("posix_memalign");
        close(fd);
        return 1;
    }

    ssize_t bytes_written = write(fd, aligned_buf, TRANSFER_SIZE);
    if (bytes_written == -1) perror("write");

    free(aligned_buf);
    close(fd);
    return 0;
}
```

---

## 2. Fine-Grained Asynchronous Flushing: `sync_file_range()`

Standard `fsync()` blocks the calling thread while synchronously writing all dirty data and metadata to disk, frequently triggering **I/O Stalls** (100ms-500ms latency spikes on flash NAND or SD cards).

Linux provides `sync_file_range()` for fine-grained flushing of specific byte intervals:
```c
#define _GNU_SOURCE
#include <fcntl.h>

int sync_file_range(int fd, off64_t offset, off64_t nbytes, unsigned int flags);
```

### Control Flags
- `SYNC_FILE_RANGE_WRITE`: Initiates writeout of dirty pages in `[offset, offset + nbytes]` in the background and returns immediately without blocking.
- `SYNC_FILE_RANGE_WAIT_BEFORE`: Waits for previously queued write operations on this byte range to complete before initiating a new write.
- `SYNC_FILE_RANGE_WAIT_AFTER`: Waits for newly scheduled write operations to fully complete.

> [!TIP]
> **Sliding Flush for Embedded Dashcams & Video Recorders:**
> As video chunks are written (e.g., every 2MB), call `sync_file_range(..., SYNC_FILE_RANGE_WRITE)` to stream dirty pages smoothly to flash storage, avoiding bursty `fsync()` stalls that drop video frames.

---

## 3. Active Kernel Cache Management: `posix_fadvise()`

Embedded targets often operate with constrained RAM (128MB to 512MB). Large file downloads or media playback can flood the Page Cache, prompting the kernel to invoke the Out-Of-Memory (OOM) Killer.

Applications use `posix_fadvise()` to guide kernel caching behavior:
```c
#include <fcntl.h>

int posix_fadvise(int fd, off_t offset, off_t len, int advice);
```
- **`POSIX_FADV_DONTNEED`**: Informs the kernel that pages in this range will **not be accessed again**. The kernel immediately frees them from Page Cache, reclaiming clean RAM.
- **`POSIX_FADV_SEQUENTIAL`**: Instructs the kernel to double its read-ahead window for efficient streaming throughput.
- **`POSIX_FADV_WILLNEED`**: Asynchronously prefetches specified disk ranges into Page Cache before user reads commence.

---

## 4. Vectored I/O (Scatter-Gather: `readv` & `writev`)

Network protocols and binary formats frequently separate protocol headers from payload bodies.
```
Traditional Approach:
write(fd, &header, sizeof(header));  // Context switch 1 + risk of small TCP packet segmentation
write(fd, payload, payload_len);     // Context switch 2

Vectored I/O (Scatter-Gather):
struct iovec iov[2];
iov[0].iov_base = &header;
iov[0].iov_len  = sizeof(header);
iov[1].iov_base = payload;
iov[1].iov_len  = payload_len;

writev(fd, iov, 2);                  // 1 Atomic system call, no intermediate memory copies
```
- **Key Benefits**:
  1. **Fewer Context Switches**: Consolidates multiple buffers into a single system call.
  2. **Atomicity**: Packet header and payload are emitted as a contiguous stream without interleaved data from concurrent threads.
  3. **Zero Intermediate Memory Copies**: Eliminates the need to allocate and `memcpy` data into a staging buffer.

---

## 5. Zero-Copy Transfers: `sendfile()` and `splice()`

When streaming static files over network sockets:
- **Traditional Model**: `read(disk_fd, buf) -> write(socket_fd, buf)` copies data 4 times across user/kernel boundaries.
- **Zero-Copy Model (`sendfile`)**:
  ```c
  #include <sys/sendfile.h>
  ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);
  ```
  The kernel transfers pages directly from the disk Page Cache to the network interface buffer using DMA, eliminating user-space memory copies and reducing CPU load.

---

## 6. Starvation Prevention in Edge-Triggered Mode (`EPOLLET`)

In Edge-Triggered mode, the event loop must exhaust incoming data until `EAGAIN` is reached.

### The Starvation Vulnerability:
If a client sends an unending data stream (e.g., continuous bulk upload), the thread remains stuck in the `while (read)` loop, starving thousands of other connected clients.

### Architectural Solution: User-Space Ready Queue & Round-Robin Budget
1. Impose a maximum byte quota per iteration (e.g., 64KB per socket).
2. If more data remains after consuming 64KB (i.e., not yet `EAGAIN`), do not continue reading immediately; append the descriptor to a user-space Ready Queue.
3. Service other ready sockets in a round-robin schedule.
4. On the next event loop iteration, call `epoll_wait(timeout = 0)` to poll new events, then continue draining backlog descriptors.

---

## 7. Multi-Threaded Event Loop with `EPOLLONESHOT`

When multiple worker threads call `epoll_wait()` on a shared `epfd`:
- Without protection, incoming packets can wake multiple threads simultaneously (**Thundering Herd**) or trigger race conditions where two threads read interleaved frames from the same connection.

### The `EPOLLONESHOT` Primitive
- When set, epoll delivers an event to **exactly one worker thread** and automatically deactivates monitoring for that descriptor.
- Once the worker finishes processing, it safely re-arms event delivery:
  ```c
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
  ev.data.fd = client_fd;
  epoll_ctl(epfd, EPOLL_CTL_MOD, client_fd, &ev);
  ```

---

## 8. Open File Description Semantics in `epoll`

A critical architectural distinction:
- `epoll` monitors the underlying kernel **Open File Description**, not the process-specific **File Descriptor**.
- When `dup()` or `fork()` is called, cloned descriptors reference the identical Open File Description in the kernel.
- **Closing a descriptor only removes it from `epoll` when ALL duplicates across ALL processes are closed.** If a child process retains an open copy, `epoll_wait()` in the parent continues to trigger events even after the parent closed its own descriptor.
