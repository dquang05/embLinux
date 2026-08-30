# Shared Memory

Shared memory allows cooperating processes to establish a common region of memory to exchange data.

## 1. Core Concepts
- **Mechanism**: A region of memory resides in the creator's address space. Other processes attach it to their own address space.
- **Performance**: Extremely fast. Only requires system calls to establish the region; subsequent accesses are routine memory accesses (no kernel intervention).
- **Responsibility**: Processes must explicitly manage synchronization (avoiding simultaneous writes) to prevent race conditions.

## 2. POSIX Shared Memory
POSIX Shared Memory provides an interface that is more consistent with the traditional UNIX I/O model compared to System V, as it uses file descriptors.

- **API Overview**:
  - `shm_open()`: Creates or opens a shared-memory object and returns a standard file descriptor.
  - `ftruncate()`: Newly created shared memory objects have a size of 0. This must be used to set the initial size before mapping.
  - `mmap()`: Maps the shared memory object into the process's virtual address space (using `MAP_SHARED`).
  - `munmap()`: Unmaps the object from the process's address space.
  - `shm_unlink()`: Removes the shared-memory object name. The object itself is destroyed when all processes have unmapped it.
- **Linux Specifics**:
  - Implemented via a `tmpfs` virtual file system mounted at `/dev/shm`.
  - Because it returns a file descriptor, standard file system calls like `fstat()`, `fchmod()`, and `fchown()` can be applied directly to the shared memory object.
- **Advantages over System V**:
  - Uses standard file descriptors instead of custom identifiers (keys).
  - Can be easily resized using `ftruncate()`, whereas System V shared memory segments have a fixed size upon creation.

## 3. System V Shared Memory
System V Shared Memory is an older alternative to POSIX Shared Memory, but widely used. Like POSIX shared memory, it does not require kernel intervention after setup.

- **API Overview**:
  - `shmget()`: Creates a new shared memory segment or obtains the identifier of an existing one. Returns a shared memory identifier.
  - `shmat()`: Attaches the shared memory segment to the calling process's virtual address space (returns a pointer `void *`). The kernel normally decides the attach address.
  - `shmdt()`: Detaches the shared memory segment when the process no longer needs it.
  - `shmctl()`: Control operations (`IPC_RMID` to delete, `IPC_STAT`, `SHM_LOCK` to lock into RAM to prevent swapping).
- **System Limits** (Configurable in `/proc/sys/kernel/` on Linux):
  - `SHMMNI`: System-wide limit on the number of shared memory identifiers.
  - `SHMMAX`: Maximum size of a single shared memory segment.
  - `SHMALL`: System-wide limit on the total number of pages of shared memory.
- **Data Structures in Shared Memory**:
  - Because a shared memory segment might be attached at different virtual addresses in different processes, **absolute pointers inside the shared memory segment are unsafe**.
  - Instead of absolute pointers, programs should store **relative offsets** (from the base address of the shared segment) or use array indexes to build data structures like linked lists or trees inside shared memory.
