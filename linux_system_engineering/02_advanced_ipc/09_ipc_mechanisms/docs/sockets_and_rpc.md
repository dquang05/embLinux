# Sockets and RPC

Used for communication in client-server systems.

## 1. Sockets
- **Nature**: An endpoint for communication. Identified by an IP address concatenated with a port number.
- **Types**: 
  - Connection-oriented (TCP)
  - Connectionless (UDP)
- **Pros/Cons**: Considered low-level IPC. Efficient but only transfers unstructured streams of bytes. Application must structure the data.

## 2. Remote Procedure Calls (RPC)
- **Nature**: Abstracts the procedure-call mechanism between systems with network connections. Messages are well-structured.
- **Mechanism**:
  - Uses a **stub** on the client side to marshal parameters (e.g., converting to External Data Representation - XDR) and send a message.
  - A server-side stub unmarshals parameters and invokes the actual procedure.
- **Semantics**: Handles network failures using "at most once" or "exactly once" execution semantics (using timestamps and ACKs).
- **Binding**: Uses predetermined ports or dynamic binding via a matchmaker (rendezvous) daemon.
- **Android RPC**: Uses the Binder framework and Android Interface Definition Language (AIDL) for interprocess communication on the same system.
