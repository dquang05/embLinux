# Core Foundations: Linux Memory Management

This document details the fundamental memory management concepts and APIs required for Embedded Linux and Systems Engineering.

---

## 1. Memory Architecture & Paging Mechanisms

### 1.1 Memory Management Unit (MMU)
- The **MMU** is a hardware component residing between the CPU core and physical memory.
- The CPU program execution operates exclusively on **Virtual Addresses** (Logical Addresses). The MMU translates each virtual address into a **Physical Address** on RAM at runtime using hardware page tables.
- If a process attempts to access a virtual address outside its mapped address space or violates permissions (e.g., writing to read-only code segment), the MMU raises a hardware exception, translated by the Linux kernel into a `SIGSEGV` (Segmentation Fault).

### 1.2 Paging (Fixed-Size Allocation)
- **Frames and Pages**:
  - Physical memory is partitioned into fixed-size blocks called **Frames** (Page Frames).
  - Virtual memory is divided into blocks of identical size called **Pages** (commonly 4KB on x86/ARM, though configurable on some architectures to 16KB or 64KB).
- **Eliminating External Fragmentation**:
  - Virtual pages do not require contiguous physical frames. Any free frame can be assigned to any virtual page, completely avoiding external fragmentation.
  - Minor **Internal Fragmentation** remains, limited to unused space within the final allocated page of a region.

### 1.3 Demand Paging
- Linux implements **Demand Paging** (lazy loading): pages are allocated in virtual space, but physical RAM frames are not committed until the process actually reads from or writes to that address.
- **Page Fault Handling Lifecycle**:
  1. CPU references a virtual address whose Page Table Entry (PTE) has its "Present" bit cleared (0).
  2. MMU raises a hardware **Page Fault** trap to the OS kernel.
  3. The kernel validates the address against the process's Virtual Memory Areas (VMAs, represented by `vm_area_struct`).
  4. If invalid, the kernel delivers `SIGSEGV` to terminate the process.
  5. If valid, the kernel locates a free physical frame, loads page content from storage (executable file, backing file, or swap), updates the PTE with the frame number and flags, and sets the Present bit.
  6. The CPU restarts the faulting instruction transparently.

### 1.4 Copy-On-Write (COW)
- When a process calls `fork()`, the kernel duplicates the parent's page tables rather than copying the entire physical RAM footprint.
- All shared pages are marked **Read-Only** in both parent and child page tables.
- As long as both processes only read from memory, they share the exact same physical frames.
- When either process writes to a page:
  1. The write triggers a Page Fault due to the read-only permission.
  2. The kernel detects a write violation on a COW page, allocates a brand new physical frame, duplicates the 4KB contents into it, updates the faulting process's page table to point to the new frame with write permissions, and leaves the other process intact.
  3. This makes `fork()` instantaneous even for processes consuming gigabytes of virtual memory.

---

## 2. Dynamic Memory Allocation APIs

### 2.1 Heap Management via `brk()` and `sbrk()`
- The heap is a contiguous virtual memory region growing upwards from the uninitialized data segment (`.bss`).
- The upper boundary of the heap is designated as the **Program Break**.
- System calls:
  - `int brk(void *end_data_segment);`: Sets the program break to an absolute virtual address.
  - `void *sbrk(intptr_t increment);`: Adjusts the program break by an offset (increment). Passing `0` returns the current program break.
- Virtual memory boundary expansion does not immediately consume physical RAM; physical frames are backed lazily on first access.

### 2.2 Heap Allocator: `malloc()` and `free()`
- The C standard library (`glibc`) implements a user-space memory manager on top of `brk()` and `mmap()` (commonly known as *ptmalloc*).
- **Chunk Metadata**:
  - Every allocated block is preceded by an internal header (chunk metadata) storing the size of the chunk and state flags (such as whether the previous chunk is free or mapped).
  - The pointer returned by `malloc(size)` points immediately *after* this metadata header.
- **Vulnerability to Metadata Corruption**:
  - Buffer overflows (writing past the allocated block) or buffer underflows (writing before the block) overwrite the chunk size and flags.
  - Calling `free()` on a corrupted chunk causes glibc to detect an inconsistent allocator state, aborting the process immediately with error messages like `corrupted size vs. prev_size` or raising `SIGABRT` / `SIGSEGV`.
- **Reclaiming Behavior**:
  - `free()` does not automatically return physical RAM to the OS. Instead, it places freed chunks into internal linked lists (*bins*, *fastbins*, *tcache*) for fast reuse in subsequent `malloc()` requests.
  - Memory is returned to the OS via `brk()` shrinking only when a large contiguous block at the top of the heap is freed and exceeds `M_TRIM_THRESHOLD`.

### 2.3 Additional Standard Allocation APIs
- **`calloc(size_t nmemb, size_t size)`**:
  - Allocates memory for an array of `nmemb` elements and initializes all bytes to zero.
- **`realloc(void *ptr, size_t size)`**:
  - Resizes an allocated block. If adjacent memory cannot accommodate the expansion, it allocates a new block elsewhere, copies previous data, frees the old block, and returns the new pointer.
- **`alloca(size_t size)`**:
  - Allocates memory directly on the **Stack frame** by decrementing the stack pointer (`esp`/`rsp`).
  - Allocation is instantaneous (single instruction) and automatically reclaimed when the calling function returns.
  - **Rules**:
    - **Never** call `free()` on pointers returned by `alloca()`.
    - Excessive allocation easily causes **Stack Overflow**, resulting in silent stack corruption or immediate process crashes.

---

## 3. Memory Mappings (`mmap`) & IPC

### 3.1 The `mmap()` System Call
- `mmap()` creates a new mapping in the virtual address space of the calling process:
  ```c
  void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
  ```
- **Protection Flags (`prot`)**:
  - `PROT_READ`: Region can be read.
  - `PROT_WRITE`: Region can be written.
  - `PROT_EXEC`: Region can execute code.
  - `PROT_NONE`: Region cannot be accessed (useful for guard pages).

### 3.2 Classification of Mappings
1. **File-Backed Mapping**:
   - Maps a region of an opened file (`fd`) directly into virtual memory. Accessing memory corresponds to reading/writing the file.
2. **Anonymous Mapping (`MAP_ANONYMOUS`)**:
   - Has no backing file (`fd` is set to `-1`, `offset` to `0`). The kernel provides zero-initialized memory pages.
   - `malloc()` uses anonymous `mmap()` under the hood for large allocations exceeding `MMAP_THRESHOLD` (typically >= 128KB) to allow immediate deallocation back to the OS via `munmap()`.

### 3.3 Sharing Attributes (`flags`)
- **`MAP_PRIVATE`**:
  - Uses Copy-On-Write. Modifications are local to the process and are never written back to the underlying file or shared with other processes.
  - Primary use case: Loading shared libraries (`.so`) and executable binaries into process memory.
- **`MAP_SHARED`**:
  - Modifications to the mapped region are visible to all processes mapping the same region.
  - For file-backed mappings, modifications are automatically written through to the underlying storage file.
  - Primary use case: High-performance shared memory IPC between related or unrelated processes.

### 3.4 Advantages of Memory-Mapped I/O
- **Zero-Copy**: Eliminates the intermediate buffer copy between kernel page cache and user-space buffers that occurs in standard `read()` / `write()` system calls.
- **Random Access Efficiency**: Avoids repeated `lseek()` calls; developers manipulate files directly via array indexing and pointer dereferences.
