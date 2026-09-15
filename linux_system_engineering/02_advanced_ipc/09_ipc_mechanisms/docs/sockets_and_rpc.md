# Sockets and Comprehensive IPC Comparison

> [!NOTE]
> For in-depth networking socket architecture, TCP/UDP state machines, address structures, and client-server socket programming, refer to:
> **[Chapter 08: Sockets and Network Programming](../../08_socket/docs/concepts.md)**.
> This document focuses specifically on **Sockets as a Local IPC Mechanism (`socketpair` and `AF_UNIX`)** and provides an overall architectural comparison across all Linux IPC facilities.

---

## 1. Local Socket IPC: `socketpair()`

While traditional `pipe()` is strictly unidirectional (half-duplex), Linux provides `socketpair()` to create a connected pair of anonymous UNIX domain sockets providing **full-duplex (bidirectional) communication**.

### 1.1 System Call Signature
```c
#include <sys/types.h>
#include <sys/socket.h>

int socketpair(int domain, int type, int protocol, int sv[2]);
```
- **`domain`**: Must be `AF_UNIX` (or `AF_LOCAL`) on Linux.
- **`type`**: `SOCK_STREAM` (bidirectional reliable byte-stream) or `SOCK_DGRAM` (bidirectional reliable datagrams).
- **`protocol`**: Must be `0`.
- **`sv[2]`**: Array of two integers returning the connected file descriptors (`sv[0]` and `sv[1]`).

### 1.2 `socketpair()` vs `pipe()`

| Characteristic | `pipe()` | `socketpair()` |
| :--- | :--- | :--- |
| **Directionality** | Unidirectional (Half-duplex) | **Bidirectional (Full-duplex)** |
| **Descriptor Roles** | `fd[0]` = Read-only; `fd[1]` = Write-only | **Both `sv[0]` and `sv[1]` are read/write** |
| **Two-way IPC Setup** | Requires 2 separate pipes (4 descriptors) | Requires only 1 socketpair (2 descriptors) |
| **Socket Options** | None (standard file descriptor) | Supports socket options (`SO_RCVBUF`, `SO_SNDBUF`) |
| **Ancillary Data** | Not supported | Supports passing File Descriptors via `SCM_RIGHTS` |

---

## 2. UNIX Domain Sockets (`AF_UNIX`) as an IPC Mechanism

UNIX Domain Sockets are designed specifically for IPC on the same machine:
- **Zero Network Stack Overhead**: All communication occurs directly in kernel space buffers. No routing, IP packet encapsulation, ARP, or checksum calculations are performed.
- **Reliable Datagrams**: Unlike UDP over network sockets, `SOCK_DGRAM` over `AF_UNIX` is **100% reliable**—packets are never dropped, corrupted, or delivered out of order.
- **Abstract Socket Namespace** (Linux-specific): If the first byte of `sun_path` is `\0`, the socket resides purely in kernel memory and does not create an inode on the filesystem. It is automatically cleaned up when the process closes it.
- **Capability to Pass File Descriptors**: A process can pass an open file descriptor (such as an open file, another socket, or pipe) to another process using `sendmsg()` with `SCM_RIGHTS`.

---

## 3. Comprehensive Master IPC Comparison Matrix

The table below summarizes all major IPC mechanisms in Linux:

| IPC Mechanism | Data Abstraction | Duplex | Persistence | Host Scope | Epoll Compatible? | Sync Required? | Throughput / Latency |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Anonymous Pipe** | Byte stream | Half-duplex | Process | Related procs | Yes | Kernel buffers | Medium / Low latency |
| **FIFO (Named Pipe)** | Byte stream | Half-duplex | Filesystem | Same host | Yes | Kernel buffers | Medium / Low latency |
| **`socketpair()`** | Byte stream / Dgram | **Full-duplex** | Process | Related procs | Yes | Kernel buffers | High / Low latency |
| **UNIX Domain Socket** | Stream / Dgram | **Full-duplex** | Filesystem / Abstract | Same host | Yes | Kernel buffers | High / Low latency |
| **Internet Socket** | Stream / Dgram | **Full-duplex** | Process | Network / Local | Yes | Network stack | Lower throughput (protocol overhead) |
| **POSIX Message Queue** | Discrete messages (priority) | Bidirectional | Kernel | Same host | **Yes** (on Linux) | Kernel-managed | High / Very low latency |
| **System V Message Queue** | Discrete messages (type-tagged) | Bidirectional | Kernel | Same host | **No** (no fd) | Kernel-managed | High / Very low latency |
| **POSIX / SysV Shared Memory** | Raw memory address space | Direct R/W | Kernel / tmpfs | Same host | **No** (memory addr) | **Yes** (Mutex / Sem) | **Fastest (Zero-copy memory speeds)** |
| **Anonymous Shared Memory (`mmap`)** | Raw memory address space | Direct R/W | Process | Related procs | **No** (memory addr) | **Yes** (Mutex / Sem) | **Fastest (Zero-copy memory speeds)** |

---

## 4. Remote Procedure Calls (RPC)
- **Concept**: Abstracts network communication so that calling a function on a remote server has the syntactic appearance of a local function call.
- **Architecture**:
  - **Client Stub**: Marshals arguments into a standardized wire format (e.g. XDR, Protocol Buffers) and transmits them via sockets.
  - **Server Stub**: Unmarshals arguments, invokes the local implementation, and marshals return values back to the client.
- **Embedded & Modern Linux Implementations**:
  - **Android Binder**: Low-level IPC/RPC driver (`/dev/binder`) optimized for zero-copy IPC between Android services.
  - **gRPC**: Modern cross-platform RPC framework utilizing HTTP/2 transport and Protocol Buffers serialization.
  - **D-Bus**: Standard system bus on desktop and embedded Linux (e.g. systemd, NetworkManager) for IPC and remote object method calls.
