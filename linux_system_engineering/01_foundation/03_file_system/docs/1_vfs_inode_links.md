# Core Concepts: Operating System File Systems

This document covers the fundamental theory of file systems from the perspective of Operating System Design (based on *Operating System Concepts*, Chapters 13, 14, and 15). Understanding these concepts is essential before diving into the Linux File System API.

---

## 1. File-System Interface (Chapter 13)

### The File Concept
A file is a contiguous logical address space provided by the operating system, mapping to physical storage. The OS abstracts the physical properties of storage devices (like HDDs or SSDs) to define a logical storage unit, the **file**.
Files contain data (the actual content) and metadata (attributes like name, identifier, type, location, size, protection, time, and user identification).

### Directory Structure
To manage millions of files, operating systems use directories (also called folders). A directory can be viewed as a symbol table that translates file names into their corresponding directory entries.
- **Tree-Structured Directories:** The standard approach in modern OS (including Linux). It allows users to create their own subdirectories and organize files hierarchically. Every file in the system has a unique absolute path starting from the root directory `/`.

### Protection
When information is stored in a computer system, it must be protected from physical damage (reliability) and improper access (protection). 
The most common approach to protection is to make access dependent on the identity of the user. In UNIX/Linux, permissions are divided into three domains:
- **Owner (User):** The creator of the file.
- **Group:** A set of users who share access.
- **Universe (Others):** All other users in the system.

Each domain can be granted `Read (r)`, `Write (w)`, or `Execute (x)` permissions.

---

## 2. File-System Implementation (Chapter 14)

### File Control Block (FCB) / Inode
To implement a file system, the OS needs a data structure to keep track of a file's metadata (everything about a file except its actual data). 
In POSIX/Linux systems, this structure is called an **inode** (Index Node). In general OS theory, it is known as a File Control Block (FCB).

An inode typically contains:
- File permissions and ownership (UID, GID).
- File timestamps (Creation, Last Access, Last Modification).
- File size in bytes.
- Pointers to the physical data blocks on the disk (Direct pointers, Single/Double/Triple Indirect pointers).

> [!IMPORTANT]
> The inode **does not** contain the file's name. The file name is stored in the Directory Entry, which points to the inode number. This design allows multiple names to point to the exact same file data (Hard Links).

### The Virtual File System (VFS)
Modern operating systems must support multiple file system types concurrently (e.g., `ext4`, `FAT32`, `NTFS`, `NFS`). To do this cleanly, the OS introduces a **Virtual File System (VFS)** layer.
- VFS provides a unified, abstract API (like `open()`, `read()`, `write()`) to user programs.
- It defines an object-oriented interface (using structures like `file`, `inode`, `dentry`, `superblock`) that all concrete file systems must implement.
- When a user program calls `read()`, the VFS routes the call to the specific implementation of the underlying file system (e.g., ext4's read function).

### Allocation Methods
How are physical disk blocks allocated to files?
1. **Contiguous Allocation:** Each file occupies a set of contiguous blocks. It is extremely fast for sequential access but suffers heavily from external fragmentation.
2. **Linked Allocation:** Each file is a linked list of disk blocks. There is no external fragmentation, but random access performance is terrible.
3. **Indexed Allocation:** Brings all pointers together into one location: the index block (inode). This is the approach used by standard Linux/UNIX file systems, offering a good balance between sequential and random access.

---

## 3. File-System Internals & Links (Chapter 15)

### Hard Links vs Soft (Symbolic) Links
Because of the separation of directory entries (which hold the file name) and inodes (which hold the metadata and data pointers), UNIX systems support two types of linking:

- **Hard Link:** A new directory entry that points directly to an existing inode. 
  - The inode maintains a `link count` (the number of hard links pointing to it). 
  - The actual file data is only deleted from the disk when the link count reaches zero. 
  - *Limitations:* You cannot create hard links across different file systems (because inode numbers are only unique within a single file system), nor can you hard-link directories (to prevent circular loops).
- **Soft Link (Symbolic Link):** A special type of file whose data block contains the *path name* of another file. 
  - If the original file is deleted, the soft link remains but points nowhere (it becomes a "dangling link"). 
  - *Advantages:* Soft links can cross file-system boundaries and can point to directories.

### Caching and Performance
Because disk I/O is orders of magnitude slower than RAM, the OS uses main memory to aggressively cache file data.
- **Page Cache / Buffer Cache:** Linux integrates virtual memory and the file system to cache file blocks in RAM. When a program calls `read()`, the kernel first checks the Page Cache. If the data is there, it's copied directly to user space without touching the disk (a huge performance boost).
- **Journaling:** To prevent file system corruption during a power failure or system crash, modern file systems (like `ext3`, `ext4`) use a journal. Metadata changes are written sequentially to a log (journal) *before* being applied to the actual file system structures. If a crash occurs, the OS simply replays the journal on the next boot to restore consistency.
