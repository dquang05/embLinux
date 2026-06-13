# Advanced Memory Mapping (mmap) Notes

> **Important Note**
>
> Memory mapping (`mmap`) is a specialized mechanism intended for specific scenarios such as very large files, shared memory IPC, database systems, high-performance I/O, and applications that require random access to large datasets.
>
> For most ordinary applications, traditional I/O using `read()` and `write()` is simpler, easier to debug, more portable, and sufficiently efficient. Unless profiling demonstrates that file I/O is a bottleneck, `read()` and `write()` should generally be preferred over `mmap()`.

---

# 1. Synchronizing Memory-Mapped Data: `msync()`

When modifying data in a `MAP_SHARED` mapping, the kernel will eventually write the modified pages back to the underlying file. However, the kernel does not guarantee *when* this write-back will occur.

The `msync()` system call allows an application to explicitly request synchronization.

```c
#include <sys/mman.h>

int msync(void *addr, size_t length, int flags);
```

## Synchronization Modes

### `MS_SYNC`

**Synchronous write-back**

The calling process blocks until all modified memory pages have been physically written to storage.

Characteristics:

* Guarantees data durability.
* Commonly required by database management systems.
* Higher latency because the caller waits for disk I/O completion.

### `MS_ASYNC`

**Asynchronous write-back**

The function returns immediately.

Characteristics:

* Dirty pages are scheduled for later write-back by kernel background processes.
* Data becomes visible through the kernel page cache immediately.
* Other processes using `read()` will observe the updated contents even before the data reaches physical storage.

### `MS_INVALIDATE`

**Invalidate other cached copies**

Marks cached copies of the affected pages as invalid.

Characteristics:

* Forces future accesses to reload the newest data.
* Useful when maintaining consistency among multiple mappings.

## Linux-Specific Behavior

Linux uses a **Unified Virtual Memory System**, meaning that:

* Memory mappings (`mmap`)
* The kernel page cache (buffer cache)

share the same physical memory pages.

As a result:

* `read()` and `mmap()` on the same file generally observe consistent data automatically.
* The primary purpose of `msync()` on Linux is to force modified pages from RAM to persistent storage.

---

# 2. Anonymous Mappings

Anonymous mappings are memory regions that are **not associated with any file on disk**.

All bytes are initialized to zero by the kernel.

## Creation Methods

### Method 1: `MAP_ANONYMOUS`

```c
mmap(NULL,
     size,
     PROT_READ | PROT_WRITE,
     MAP_PRIVATE | MAP_ANONYMOUS,
     -1,
     0);
```

Requirements:

* `MAP_ANONYMOUS` flag
* `fd = -1`

### Method 2: `/dev/zero`

```c
int fd = open("/dev/zero", O_RDWR);

mmap(NULL,
     size,
     PROT_READ | PROT_WRITE,
     MAP_PRIVATE,
     fd,
     0);
```

`/dev/zero` is a special device file that:

* Returns an endless stream of zero bytes when read.
* Discards any written data.

Both methods are effectively equivalent on Linux.

## Types of Anonymous Mappings

| Type                           | Behavior                                                                                                                             | Typical Usage                                                                                                                     |
| ------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------- |
| `MAP_PRIVATE \| MAP_ANONYMOUS` | Private memory initialized to zero. After `fork()`, Copy-On-Write (COW) is used.                                                     | Large allocations performed internally by `glibc malloc()` when the requested size exceeds `MMAP_THRESHOLD` (typically ≥ 128 KB). |
| `MAP_SHARED \| MAP_ANONYMOUS`  | Shared memory initialized to zero. After `fork()`, parent and child continue sharing the same physical pages without triggering COW. | High-performance shared memory IPC between related processes.                                                                     |

### Large Allocations in glibc

For large allocations, `glibc` often bypasses the heap and directly requests memory using:

```c
MAP_PRIVATE | MAP_ANONYMOUS
```

Advantages:

* Fast allocation and deallocation.
* Memory can be returned directly using `munmap()`.
* Reduces heap fragmentation.

---

# 3. Swap Space, Overcommitting, and the OOM Killer

## Memory Overcommit

Some applications allocate extremely large virtual memory regions while actually touching only a small subset of pages.

Example:

* Sparse matrices
* Sparse arrays
* Memory-mapped databases

Reserving swap space for every allocated page immediately would waste resources.

Therefore Linux commonly uses **lazy swap reservation**:

* Memory is granted virtually.
* Swap resources are reserved only when pages are actually used.

This allows:

```text
Allocated Virtual Memory
>
Physical RAM + Swap
```

This behavior is known as **memory overcommitment**.

## Consequences

If many processes suddenly begin using all memory they previously reserved:

* RAM becomes exhausted.
* Swap becomes exhausted.
* The system risks becoming unresponsive.

To prevent a kernel panic, Linux activates the:

**OOM Killer (Out-Of-Memory Killer)**

## OOM Killer Selection Algorithm

Each process has a score:

```text
/proc/<PID>/oom_score
```

Higher scores indicate a greater likelihood of being terminated.

### More Likely to Be Killed

Processes that:

* Consume large amounts of RAM.
* Create many child processes.
* Run with lower scheduling priority (`nice > 0`).

### Less Likely to Be Killed

Processes that:

* Run as root.
* Access hardware devices directly.
* Have been running for a long time.
* Have already consumed significant CPU time.

The kernel attempts to minimize lost work when selecting victims.

## Manual Adjustment

Administrators can influence OOM selection through:

```text
/proc/<PID>/oom_adj
```

Typical range:

```text
-16  ...  +15
```

Where:

* Lower values = more protected.
* Higher values = easier to kill.

Historically:

```text
-17
```

effectively granted complete immunity from OOM termination.

---

# 4. Nonlinear File Mappings: `remap_file_pages()`

## Traditional Mapping

A normal mapping is linear:

```text
File Page 0 -> Memory Page 0
File Page 1 -> Memory Page 1
File Page 2 -> Memory Page 2
...
```

## The Problem

Some applications need arbitrary page arrangements:

```text
File Page 2 -> Memory Page 0
File Page 0 -> Memory Page 2
File Page 7 -> Memory Page 1
```

A traditional solution is multiple `mmap()` calls using `MAP_FIXED`.

However, this creates many:

```text
VMA (Virtual Memory Area)
```

structures inside the kernel.

Consequences:

* Increased management overhead.
* Slower page fault handling.

## `remap_file_pages()`

Linux provides a specialized system call:

```c
#include <sys/mman.h>

int remap_file_pages(
    void *addr,
    size_t size,
    int prot,
    size_t pgoff,
    int flags
);
```

This allows rearranging file pages directly within page tables.

Advantages:

* Fewer VMAs.
* Faster page fault processing.
* Better scalability for very large mappings.

Typical users include:

* Database management systems
* Virtual memory managers
* Garbage collectors
* Large caching systems

> Note: `remap_file_pages()` is Linux-specific and non-portable. Modern Linux kernels have largely deprecated its original optimization purpose because VM management has improved significantly.

---

# 5. Critical Signals When Using `mmap()`

Memory mapping introduces runtime failure modes that differ from ordinary file I/O.

The two most important signals are:

## SIGSEGV (Segmentation Fault)

Occurs when the process violates memory protection rules.

Examples:

### Writing to Read-Only Memory

```c
mmap(..., PROT_READ, ...);
```

Then:

```c
mapped_region[0] = 'A';
```

Result:

```text
SIGSEGV
```

### Accessing Beyond the Mapping

Accessing memory outside the mapped region also triggers:

```text
SIGSEGV
```

---

## SIGBUS (Bus Error)

Typically occurs with file-backed mappings.

The virtual address may belong to a valid mapped page, but the corresponding file data does not exist.

Example:

```text
File size = 1000 bytes
Mapping size = 4096 bytes
```

The mapping occupies an entire page.

Accessing bytes that correspond to nonexistent file blocks may generate:

```text
SIGBUS
```

Reason:

* The kernel cannot translate the access into valid storage blocks.
* No physical file data exists for the requested offset.

### Typical Causes

* File truncation after mapping.
* Accessing holes beyond the actual file size.
* Invalid file-backed pages.

---

# Key Takeaways

* `mmap()` is a powerful but specialized optimization tool.
* Use `read()` and `write()` for ordinary file operations.
* `msync()` controls when modified pages are flushed to storage.
* Anonymous mappings provide zero-initialized memory without files.
* Linux overcommits virtual memory and relies on the OOM Killer during memory exhaustion.
* `remap_file_pages()` enables nonlinear file mappings for advanced workloads.
* Always be prepared to handle `SIGSEGV` and `SIGBUS` when working with memory-mapped files.
