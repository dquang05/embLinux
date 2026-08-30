# Core Concepts: Network & Distributed Systems

*(Summary from Operating System Concepts - Chapter 19)*

## 1. Overview of Distributed Systems
A distributed system is a collection of loosely coupled nodes interconnected by a communication network.
- **Main Advantages:**
  - **Resource Sharing:** Remote file access, using databases on other nodes, sharing peripheral devices, or utilizing processing hardware (like GPUs/Supercomputers).
  - **Computation Speedup:** Breaking down jobs to run in parallel across multiple nodes (highly useful for Big Data analysis). *Load balancing* features help distribute the workload evenly.
  - **Reliability:** If one node fails, the system continues to operate due to data and hardware redundancy.

## 2. Network Structure
- **LAN (Local-Area Networks):** Small geographic scope (offices, buildings), high speed, low latency. Common examples include Ethernet (wired network, switches/routers) and WiFi (wireless network via Access Points).
- **WAN (Wide-Area Networks):** Broad geographic scope (like the global Internet). Uses Routers to interconnect multiple networks. Traditional WAN link speeds are slower than LANs but are advancing rapidly thanks to fiber optics.

## 3. Communication Structure
### 3.1 Naming & Name Resolution
In a network, processes communicate via a `<host name, identifier>` pair.
- **DNS (Domain Name System):** The protocol and server system that translates human-readable domain names (e.g., `eric.cs.yale.edu`) into IP addresses (e.g., `128.148.31.100`) so network devices can route them.
- **ARP (Address Resolution Protocol):** Resolves an IP address into a MAC (Medium Access Control) physical address so hardware devices can transmit packets to each other within the same LAN (Ethernet).

### 3.2 Transport Protocols: TCP and UDP
Transport layer protocols use a **Port Number** to identify the process (application) receiving/sending data on a computer (e.g., HTTP typically uses port 80, SSH uses port 22).

- **UDP (User Datagram Protocol):**
  - A connectionless protocol, designed to be very simple (only contains port, length, and checksum).
  - Fast transmission speed but **unreliable**.
  - Does not guarantee successful data delivery; packets (datagrams) can be lost or arrive out of order. Applications must handle error management themselves if they use UDP.
- **TCP (Transmission Control Protocol):**
  - A connection-oriented protocol, **reliable**.
  - Requires connection establishment before data transmission (often called a 3-way handshake).
  - Ensures data is transmitted as a continuous, ordered byte stream using **Sequence numbers** and acknowledgment mechanisms (**ACK**). If packets are lost, TCP automatically retransmits them.
  - Integrates **Flow control** (prevents overwhelming the receiver's buffer) and **Congestion control** (prevents overloading routers and the network).

## 4. Network and Distributed Operating Systems
- **Network Operating Systems (NOS):** Provide utilities to access remote resources, but users still need to know specific commands/software to operate them. (e.g., Logging into remote computers via `ssh`, transferring files via `ftp`/`sftp`, file sharing via Cloud Storage).
- **Distributed Operating Systems (DOS):** A distributed system that provides a transparent environment, hiding the differences between local and remote resources. It provides features like:
  - **Data Migration:** Automatically pulls data (entirely or partially) to the running process.
  - **Computation Migration:** Runs computations directly where the data resides (usually via RPC - Remote Procedure Call) to save massive network bandwidth.
  - **Process Migration:** Moves running processes to other nodes with more CPU resources (load balancing) or better hardware.

## 5. Distributed File Systems (DFS)
The goal is to allow users to access network files exactly as if they were on local hard drives.
- **Client-Server Architecture (e.g., NFS, OpenAFS):**
  - **NFS (Network File System):** The original version was designed statelessly, making server recovery extremely easy upon a crash. Provides the ability to "mount" remote directories into the local directory tree. However, it generates many network messages (chatty).
  - **OpenAFS:** Focuses on serving a large number of clients (scalability). The client downloads a large chunk of files to its machine and only writes back to the server when the file is closed.
- **Cluster-Based Architecture (GFS, Hadoop/HDFS):**
  - Developed to handle massive Big Data.
  - An extremely large file is split into multiple "chunks" and replicated across many **Data Servers**.
  - A single **Metadata Server** (or master) stores the location map of these chunks. When a client needs to read a file, it contacts the Metadata Server first, then communicates directly with the respective Data Servers. Allows parallel and infinitely scalable data read/write operations.

- **DFS Caching and Consistency Issues:**
  - To optimize access, data is often cached in the RAM or disk of the client.
  - **Write-through:** Immediately updates the server whenever the client makes a change (Safe, no data loss on crash, but slow).
  - **Delayed-write (Write-back / Write-on-close):** Modifies in the cache first, and after some time (or upon file close), bulk pushes to the server (High speed but risks data loss).
