# Chapter 10: Memory Management

## Objectives
- Master the underlying mechanisms of Linux memory management: Virtual Memory, Memory Management Unit (MMU), and Paging.
- Understand Demand Paging, Page Fault handling, and the Copy-On-Write (COW) mechanism during process creation.
- Deeply understand how heap allocation functions (`malloc`, `free`, `realloc`, `calloc`) operate under the hood using `brk()` / `sbrk()` and anonymous `mmap()`.
- Investigate heap metadata layout and witness how buffer overruns/underruns corrupt allocator control headers leading to crashes.
- Master Memory Mappings (`mmap`): Anonymous vs. File-backed, Shared (`MAP_SHARED`) vs. Private (`MAP_PRIVATE`), and zero-copy Memory-Mapped I/O.
- Apply specialized virtual memory operations: Memory Pinning (`mlock`/`mlockall`), Kernel Access Hints (`madvise`), Page Residency Inspection (`mincore`), and Synchronous Flushing (`msync`).

## References
- **OSC (Operating System Concepts)**:
  - Chapter 9: Main Memory (Address Binding, Logical vs. Physical Address Space, Swapping, Paging, Page Table Structure)
  - Chapter 10: Virtual Memory (Demand Paging, Copy-on-Write, Page Replacement Algorithms, Allocation of Frames, Thrashing, Memory Compression, Kernel Allocators)
- **TLPI (The Linux Programming Interface)**:
  - Chapter 7: Memory Allocation (`brk`, `sbrk`, `malloc`, `free`, `realloc`, `alloca`)
  - Chapter 49: Memory Mappings (`mmap`, `munmap`, `msync`, `mprotect`, Anonymous mappings)
  - Chapter 50: Virtual Memory Operations (`mlock`, `mlockall`, `mincore`, `madvise`, `posix_madvise`)
- **Cross-References**:
  - Chapter 04: Process Creation & Memory Layout ([04_process](../../01_foundation/04_process))
  - Chapter 09: Interprocess Communication & Shared Memory ([09_ipc_mechanisms](../09_ipc_mechanisms))

## Documentation Guide
- [Core Concepts & Taxonomy](docs/concepts.md)
- [Core Foundations: Virtual Memory, Allocation APIs & mmap](docs/core_knowledge.md)
- [Advanced Optimizations: Kernel Internals & Virtual Memory Operations](docs/advanced_optimizations.md)

## Source Code Demonstrations (`src/`)

| File | Description | Key APIs Used |
| :--- | :--- | :--- |
| `malloc_metadata_corruption.c` | Inspects heap program break (`sbrk`), examines pre-pointer allocator metadata, and demonstrates how buffer corruptions trigger aborts/crashes upon `free()`. | `sbrk()`, `malloc()`, `free()` |
| `mlock_mincore_demo.c` | Verifies demand paging lazy allocation, provides kernel read-ahead hints with `madvise()`, pins pages to physical RAM using `mlock()`, and queries page residency via `mincore()`. | `mmap()`, `mincore()`, `madvise()`, `mlock()`, `munlock()`, `munmap()`, `sysconf()` |
| `mmap_ipc_demo.c` | Implements high-performance zero-copy shared memory IPC between parent and child processes using shared anonymous memory mappings and commits pages with `msync()`. | `mmap(MAP_SHARED \| MAP_ANONYMOUS)`, `fork()`, `msync()`, `munmap()` |

## Build and Run Instructions

### 1. Compile All Demos
From this directory, run:
```bash
make clean
make
```

All compiled binaries will be output into the `build/` directory:
- `build/malloc_metadata_corruption`
- `build/mlock_mincore_demo`
- `build/mmap_ipc_demo`

### 2. Run Demonstrations

#### Demo 1: Shared Anonymous Memory Mapping IPC
```bash
./build/mmap_ipc_demo
```

#### Demo 2: Virtual Memory Operations (`mlock`, `madvise`, `mincore`)
```bash
./build/mlock_mincore_demo
```
*(Note: Pinning large memory regions with `mlock()` may require elevated permissions depending on `ulimit -l` limits).*

#### Demo 3: Malloc Metadata Corruption
```bash
./build/malloc_metadata_corruption
```
*(Note: Intentionally corrupts heap headers to demonstrate standard glibc integrity checks and crashes).*
