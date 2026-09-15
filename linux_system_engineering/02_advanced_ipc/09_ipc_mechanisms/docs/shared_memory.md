# Shared Memory & Memory Mapping

Shared memory allows cooperating processes to establish a common region of memory to exchange data without incurring kernel data-copying overhead.

---

## 1. Core Concepts & Taxonomy
- **Mechanism**: A region of physical memory pages is mapped into the virtual address spaces of two or more processes.
- **Performance**: The fastest IPC mechanism available on Linux. System calls are only needed to set up and tear down the mapping; subsequent reads and writes are standard memory dereferences without kernel mediation.
- **Responsibility**: Because the kernel provides no implicit mutual exclusion or data-boundary serialization (unlike pipes or message queues), communicating processes **must explicitly synchronize** concurrent access using mutexes, semaphores, or atomic operations.

---

## 2. Memory Mapping (`mmap`) & The Four Mapping Types

The Linux kernel provides `mmap()` to map files or devices into memory. Every memory mapping is categorized along two orthogonal axes:
1. **Sharing**: `MAP_SHARED` vs `MAP_PRIVATE`
2. **Backing Source**: File-backed vs Anonymous (`MAP_ANONYMOUS`)

This creates **Four Core Quadrants**:

```text
               |  File-backed Mapping            |  Anonymous Mapping (MAP_ANONYMOUS)
---------------+---------------------------------+--------------------------------------
MAP_SHARED     |  1. Shared File-backed          |  3. Shared Anonymous
               |  - Backed by regular disk file  |  - No disk file (RAM/swap only)
               |  - Changes visible to others    |  - Shared across fork()
               |  - Updates flushed to disk      |  - Fast IPC between related procs
---------------+---------------------------------+--------------------------------------
MAP_PRIVATE    |  2. Private File-backed         |  4. Private Anonymous
               |  - Backed by disk file          |  - No disk file (zero-initialized)
               |  - Copy-On-Write (COW) on write |  - Completely isolated to process
               |  - Used for binaries & .so libs |  - Backend for malloc() (large chunks)
```

### 2.1 Shared File-backed Mapping (`MAP_SHARED`)
- **How it works**: Maps a file on the file system into the process virtual address space.
- **Behavior**: Modifications by any process are immediately visible to all other processes mapping the same region and are eventually written back to the underlying disk file.
- **Flushing to Disk**: Programmers can use `msync(void *addr, size_t length, int flags)` with `MS_SYNC` (blocking) or `MS_ASYNC` (asynchronous) to control when modified (dirty) pages are synced to disk.

### 2.2 Private File-backed Mapping (`MAP_PRIVATE` & Copy-On-Write)
- **How it works**: Maps a file into memory for reading.
- **Copy-On-Write (COW)**:
  1. Initially, all processes share the same physical pages in read-only mode in their page tables.
  2. When any process attempts to **write** to a page, the CPU Memory Management Unit (MMU) raises a page fault exception.
  3. The Linux kernel intercepts the fault, allocates a **new physical page frame**, copies the original content into it, marks the new page writable in that process's page table, and resumes execution.
  4. Changes are completely private to the writing process and are **never** written back to the disk file.
- **Primary Use Case**: Loading executable programs (ELF `.text` and initialized data) and shared libraries (`.so`). All running instances of a program share code pages in RAM until individual processes modify private writable data.

### 2.3 Shared Anonymous Mapping (`MAP_SHARED | MAP_ANONYMOUS`)
- **How it works**: Allocates memory not backed by any file. The `fd` parameter to `mmap()` is passed as `-1` (with `offset = 0`).
- **Legacy Alternative**: On older UNIX/BSD systems lacking `MAP_ANONYMOUS`, opening `/dev/zero` read/write and mapping it with `MAP_SHARED` achieves the exact same behavior.
- **IPC Mechanics**:
  - Memory is initialized to zero.
  - The mapping must be created **before** calling `fork()`.
  - Upon `fork()`, the child process inherits the memory mapping, and both parent and child share the same physical pages. Writes by parent or child are immediately visible to both.
  - Ceases to exist when all processes unmap it (`munmap()`) or terminate. No files are left on disk.

### 2.4 Private Anonymous Mapping (`MAP_PRIVATE | MAP_ANONYMOUS`)
- **How it works**: Allocates a private, zero-filled memory block isolated to the calling process.
- **Primary Use Case**: Memory allocators like `glibc` `malloc()` use this for allocations exceeding `MMAP_THRESHOLD` (typically $\ge 128$ KB) to avoid fragmentation of the process data segment (heap / `brk()`).

---

## 3. POSIX Shared Memory

POSIX Shared Memory provides a file-descriptor-based interface conforming to POSIX standards, creating objects in a memory-backed file system (`tmpfs`).

- **API Lifecycle**:
  - `shm_open()`: Creates or opens a named shared memory object (names conventionally begin with `/`). Returns a file descriptor.
  - `ftruncate()`: Sets the initial size of the shared memory object (newly created objects have size 0).
  - `mmap()`: Maps the object into virtual memory using `MAP_SHARED`.
  - `munmap()`: Detaches the mapping from the process address space.
  - `close()`: Closes the file descriptor.
  - `shm_unlink()`: Removes the object name from the system. The memory persists until all processes close and unmap it.
- **Linux Implementation Details**:
  - Objects are stored under the `/dev/shm` virtual filesystem (`tmpfs`).
  - You can inspect active POSIX shared memory segments with `ls -la /dev/shm` and remove orphaned segments with `rm /dev/shm/<name>`.
  - Standard file operations like `fstat()`, `fchmod()`, and `fchown()` apply directly.

---

## 4. System V Shared Memory & `IPC_PRIVATE`

System V Shared Memory uses keys (`key_t`) and integer IPC identifiers (`shmid`).

- **API Lifecycle**:
  - `shmget()`: Allocates or locates a segment.
  - `shmat()`: Attaches the segment into the calling process address space.
  - `shmdt()`: Detaches the segment.
  - `shmctl()`: Control commands (`IPC_RMID` to mark for deletion, `IPC_STAT`, `SHM_LOCK`).
- **The True Meaning of `IPC_PRIVATE`**:
  - A common misconception is that `IPC_PRIVATE` allocates "private unshared memory".
  - In reality, `IPC_PRIVATE` (defined as `(key_t) 0`) instructs `shmget()` to **always create a brand-new shared memory segment** with a unique key guaranteed not to conflict with any existing segment.
  - It is specifically intended for **cooperating related processes** (parent-child). The parent creates the segment using `IPC_PRIVATE`, calls `fork()`, and the child inherits the `shmid` (or attaches to it), sharing the memory segment completely.
- **Addressing and Relocation**:
  - Since different processes may attach the same shared memory segment at different virtual memory addresses, **storing absolute pointers inside shared memory is unsafe**.
  - Always store **relative byte offsets** or array indexes when constructing linked lists, trees, or queue structures in shared memory.

---

## 5. Synchronization Inside Shared Memory

Because shared memory accesses bypass the OS kernel, processes must coordinate their access to shared structures:

1. **POSIX Unnamed Semaphores (`sem_t`)**:
   - Resides directly inside the shared memory structure.
   - Initialized via `sem_init(sem_t *sem, int pshared, unsigned int value)`.
   - **Crucial Parameter**: Setting `pshared = 1` tells the kernel that this semaphore is shared between processes (rather than threads within the same process).
2. **Process-Shared Pthread Mutexes**:
   - `pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED)`.
   - `pthread_mutex_init(&shm->mutex, &attr)`.
3. **Atomic Operations & Lock-free Rings**:
   - Using C11 atomics (`stdatomic.h`) or GCC built-ins for single-producer single-consumer (SPSC) circular ring buffers.
