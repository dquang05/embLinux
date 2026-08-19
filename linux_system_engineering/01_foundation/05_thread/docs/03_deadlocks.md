# Deadlocks

This document covers the theoretical aspects of deadlocks in concurrent systems, based on Operating System Concepts (Chapter 8).

## 1. Deadlock Characterization
- **Deadlock:** A situation where a set of threads (or processes) is permanently blocked because each thread is holding a resource and waiting for an event (typically the release of a resource) that can only be triggered by another thread in the same set.
- **Livelock:** Similar to a deadlock, but instead of being blocked, the threads continuously attempt an operation that always fails, resulting in no progress.
- **The Four Necessary Conditions:** A deadlock can arise if and only if all four of the following conditions hold simultaneously in a system:
  1. **Mutual Exclusion:** At least one resource must be held in a non-shareable mode.
  2. **Hold and Wait:** A thread must be holding at least one resource and waiting to acquire additional resources that are currently held by other threads.
  3. **No Preemption:** Resources cannot be forcibly removed (preempted) from a thread holding them; they can only be released voluntarily by the thread after it has completed its task.
  4. **Circular Wait:** There must exist a circular chain of waiting threads (e.g., $T_0$ is waiting for $T_1$, $T_1$ for $T_2$, ..., $T_n$ for $T_0$).

## 2. Resource-Allocation Graph
- Deadlocks can be modeled using a directed graph consisting of vertices (Threads $T_i$ and Resources $R_j$).
- **Request Edge:** A directed edge $T_i \rightarrow R_j$ indicates that thread $T_i$ has requested resource $R_j$.
- **Assignment Edge:** A directed edge $R_j \rightarrow T_i$ indicates that resource $R_j$ has been allocated to thread $T_i$.
- **Graph Analysis:**
  - If the graph contains **no cycles**, then **no deadlock** exists.
  - If the graph contains a **cycle**:
    - If each resource type has exactly one instance, then a deadlock **has occurred**.
    - If each resource type has multiple instances, a deadlock **may** exist (a cycle is necessary but not sufficient in this case).

## 3. Methods for Handling Deadlocks
Operating systems typically use one of three approaches to deal with deadlocks:

### A. Ignorance (The Ostrich Algorithm)
- Pretend that deadlocks never occur in the system. 
- This is the approach used by most mainstream operating systems, including Linux and Windows, because deadlocks are rare and the cost of preventing them is very high. It is left to application developers to write deadlock-free code.

### B. Prevention
- Provides a set of methods to ensure that at least one of the four necessary conditions cannot hold.
- The most practical prevention method is to invalidate the **Circular Wait** condition. This is usually done by imposing a strict total ordering of all resource types and requiring that each thread requests resources in an increasing order of enumeration.

### C. Avoidance
- Requires the system to have some additional *a priori* information about how resources will be requested. For example, a thread must declare the maximum number of resources it will ever need.
- The OS dynamically examines the resource-allocation state to ensure that a circular-wait condition can never exist.
- **Safe State:** The system is in a safe state if there exists a safe sequence of execution for all threads such that each can finish its execution.
- **Banker's Algorithm:** A classic deadlock avoidance algorithm used when there are multiple instances of each resource type. It only grants requests if the resulting system state is "safe."

### D. Detection and Recovery
- If a system does not employ prevention or avoidance, it may allow deadlocks to occur, run an algorithm to detect them, and then recover.
- **Detection:** Algorithms are run periodically to check for cycles in the resource-allocation graph.
- **Recovery Options:**
  - **Process Termination (Abort):** Abort all deadlocked threads or abort them one by one until the deadlock cycle is broken (costly because partial computation is lost).
  - **Resource Preemption:** Successively preempt some resources from threads and give them to others until the deadlock is broken. This requires dealing with issues like rollback and starvation.
