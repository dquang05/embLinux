# Core Concepts: Sockets and Network Programming

This section focuses on core concepts, the operational mechanisms of APIs, and how to design servers using Sockets in C/C++ programming on Linux.

## 1. Overview of Networks and Distributed Systems
Before diving deep into Socket programming, it is crucial to understand their operating environment (LAN/WAN architecture, TCP/UDP protocols, concepts of DNS, ARP, and distributed system design models).

> [!NOTE]
> This is foundational knowledge on network system design, helping you create overall architectural solutions when building distributed software.
> Please refer to the detailed documentation: **[Network and Distributed Systems Overview](file:///home/quangtran/Projects/embLinux/linux_system_engineering/02_advanced_ipc/08_socket/docs/distributed_systems_overview.md)**.

## 2. Socket Architecture and API Functions (From the TLPI Book)

### 2.1 Communication Domains (UNIX vs IPv4/IPv6)
- **`AF_UNIX` (UNIX Domain):** Used for local communication between processes on the same machine (IPC). The address is a file on the system (using the `struct sockaddr_un` structure).
  - **Characteristics:** Data transmission, even using Datagrams (`SOCK_DGRAM`), remains **reliable** and lossless because it is handled entirely within the kernel's memory.
  - **Abstract Namespace:** (Linux only) If the first character of `sun_path` is `\0`, the socket is created in memory without generating a physical file.
- **`AF_INET` / `AF_INET6` (Internet Domain):** Communication over IPv4/IPv6 networks. Uses an IP address and Port pair (using `struct sockaddr_in` or `sockaddr_in6` structures).

### 2.2 Socket Types (Stream vs Datagram)
- **Stream Sockets (`SOCK_STREAM` - TCP):** 
  - Transmits data as a byte-stream, with no message boundaries.
  - Connection-oriented: Requires establishing a connection prior to transmission.
  - Reliable: Guarantees data will not be lost and arrives in the correct sequence.
- **Datagram Sockets (`SOCK_DGRAM` - UDP):**
  - Transmits via independent packets (datagrams), preserving message boundaries.
  - Connectionless.
  - Unreliable: Packets can be lost, duplicated, or arrive out of order. (Note: in TCP/IP networks, UDP packets should ideally be kept under 512 bytes to avoid IP Fragmentation).

### 2.3 Address Structures and Core System Calls
- **`struct sockaddr`:** A generic structure used for casting when calling APIs, allowing functions to support multiple domains uniformly.
- **Main System Calls:**
  - `socket()`: Creates a new file descriptor for the socket.
  - `bind()`: Assigns a fixed address (name) to the socket (e.g., a network port or a file system path). For `AF_UNIX`, it is recommended to `remove()`/`unlink()` the old file before calling `bind()`.
  - `listen()`: Enables passive listening mode, indicating to the kernel that this socket is dedicated to accepting connections.
  - `accept()`: (TCP Server) Accepts a connection from a client. It **creates a new file descriptor** dedicated to communicating with that specific client, while the original socket continues listening.
  - `connect()`: (TCP Client) Initiates a connection to the server. For UDP, `connect()` can be used to set a default destination address, allowing the use of `write()`/`read()` instead of `sendto()`/`recvfrom()`.
  - `socketpair()`: Allocates a pair of connected UNIX domain sockets (often used as a convenient two-way alternative to `pipe` for IPC).

### 2.4 Endianness and Address Conversion APIs
- **Network Byte Order (Endianness):** CPUs store data differently (x86 uses Little-endian). When sending over a network, data must be cast to the common standard of **Big-endian**. Use functions: `htons()`, `htonl()` (Host to Network) and `ntohs()`, `ntohl()` (Network to Host).
- **String IP Conversion:**
  - `inet_pton()`: Converts an IP from string format (`192.168.1.1`) to binary format.
  - `inet_ntop()`: Converts an IP from binary format back to string format.
- **Domain Name Resolution (Modern DNS API):** (Recommended over legacy functions like `gethostbyname`).
  - **`getaddrinfo()`:** Takes a hostname (`www.google.com`) and a service (`http` or `80`) as input, returning a list of `struct addrinfo`. Supports protocol-independent processing (both IPv4 and IPv6) transparently.
  - **`getnameinfo()`:** Reverse translates from a binary IP/Port to a hostname/service.

### 2.5 Advanced I/O Functions for Sockets
- **`recv()` and `send()`:** Similar to `read()` and `write()` but include a flags parameter for flow control, such as the `MSG_DONTWAIT` flag (for non-blocking mode) or `MSG_PEEK` (reads data without removing it from the buffer).
- **`shutdown()`:** Closes one direction of the connection (Half-close). For example, `shutdown(fd, SHUT_WR)` signals the server that the Client has finished sending data (sending a FIN packet), but the Client can still continue to receive (`read`) responses from the server. This is very common when writing Clients.
- **`sendfile()`:** (Zero-copy feature) Allows copying directly from a disk file descriptor to a socket descriptor entirely in Kernel space, bypassing the application's buffer (User space). Offers extremely high performance for Web Servers.
- **`getsockname()` & `getpeername()`:** Retrieves the local IP and Port (especially after automatic connection to see which ephemeral port the OS assigned) or fetches the remote Client's address.

## 3. Server Design and TCP Lifecycle

### 3.1 TCP Connection Lifecycle (State Machine)
TCP manages state very strictly:
- Starts in `LISTEN` (waiting for connections). The 3-way handshake process transitions through `SYN_SENT`, `SYN_RECV`, and then reaches `ESTABLISHED`.
- When closing a connection, one side sends a `FIN` flag and moves through `FIN_WAIT1`, `FIN_WAIT2`, and then `TIME_WAIT` to ensure the final ACK packet is received and does not interfere with a new stream.

### 3.2 Server Models
1. **Iterative Server:** Handles clients sequentially, one by one. Suitable for UDP or very lightweight, immediate-response TCP services.
2. **Concurrent Server (Using `fork()`):**
   - The server calls `accept()` to receive a new TCP connection, then calls `fork()` to spawn a child process.
   - The child process exclusively handles that connection's request and then exits (`exit`). The parent process closes the file descriptor of the newly accepted socket and continues the `accept()` loop for the next client.
   - This is a traditional model that is easy to program but incurs high process creation costs if the access volume is large (in which case Pre-forking, Multithreading, or I/O Multiplexing should be considered).
3. **`inetd` Daemon (Superserver):** Manages many network ports simultaneously. When a connection arrives, `inetd` automatically `fork()`s and `exec()`s directly into the service handling program (the program reads via `STDIN/STDOUT`). Useful for rarely used network services, helping conserve system RAM.

### 3.3 TIME_WAIT State and SO_REUSEADDR Option
- **Purpose of TIME_WAIT:** Persists for an interval of `2MSL` (Maximum Segment Lifetime). It protects TCP from mistakenly receiving delayed, old packets from the previous connection (with the same IP and Port) and guarantees the peer receives the final ACK packet.
- **`EADDRINUSE` Error:** Occurs when a Server crashes/restarts and attempts to `bind()` to a port where the previous connection is still stuck in the `TIME_WAIT` state.
- **Solution - `SO_REUSEADDR`:** TCP Servers should **always** enable this socket option using `setsockopt` before calling `bind()` to allow immediate port reuse.

### 3.4 Advanced Socket Features
- **Reasons to use UDP:** Although lacking the reliability of TCP, UDP is extremely useful for applications requiring speed and rapid response (no connection setup/teardown latency, e.g., DNS), or when **Multicast / Broadcast** is mandatory (which TCP does not support).
- **File Descriptor Passing (`sendmsg` / `recvmsg`):** Through the "Ancillary Data" structure of UNIX domain sockets, a process can send a file descriptor reference (e.g., a newly `accept()`ed socket) to another process to handle. This is highly powerful when designing Server Pools.
