# Threads and Concurrency

This document provides a detailed overview of threads and concurrency models in modern operating systems, primarily based on the concepts from Operating System Concepts (Chapter 4).

## 1. Overview
- **Thread:** A basic unit of CPU utilization consisting of a thread ID, program counter (PC), a register set, and a stack. Threads within the same process share its code section, data section, and other operating-system resources (such as open files and signals).
- **Benefits of Multithreading:**
  - **Responsiveness:** Allows a program to continue running even if part of it is blocked or performing a lengthy operation (e.g., UI threads).
  - **Resource Sharing:** Threads naturally share the memory and resources of the process they belong to, making inter-thread communication much easier than inter-process communication (IPC).
  - **Economy:** Allocating memory and resources for process creation is costly. Because threads share resources, it is more economical to create and context-switch threads.
  - **Scalability:** The benefits of multithreading can be greatly increased in a multiprocessor architecture, where threads may be running in parallel on different processing cores.

## 2. Multicore Programming
- **Concurrency vs. Parallelism:**
  - **Concurrency** supports more than one task making progress. A single-core system can run concurrent tasks by rapidly interleaving their execution.
  - **Parallelism** implies a system can perform more than one task simultaneously (requires multiple cores).
- **Types of Parallelism:**
  - **Data Parallelism:** Distributes subsets of the same data across multiple cores and performs the same operation on each.
  - **Task Parallelism:** Distributes independent tasks (threads) across multiple cores. Each task may perform a unique operation.
- **Amdahl's Law:** A formula to evaluate the maximum potential performance gain from adding additional computing cores. It highlights that the speedup is fundamentally limited by the serial (non-parallelizable) portion of the application.

## 3. Multithreading Models
Threads can be supported at the user level (**User Threads**) or at the kernel level (**Kernel Threads**). 
- **Many-to-One Model:** Maps many user-level threads to a single kernel thread. Thread management is efficient in user space, but if one thread makes a blocking system call, the entire process will block. It also cannot take advantage of multiple cores.
- **One-to-One Model:** Maps each user thread to a corresponding kernel thread. Provides higher concurrency but has the drawback of higher overhead, as creating a user thread requires creating the corresponding kernel thread. Supported by Linux and Windows.
- **Many-to-Many Model (Two-level Model):** Multiplexes many user-level threads to a smaller or equal number of kernel threads. Extremely flexible but complex to implement.

## 4. Thread Libraries
Thread libraries provide programmers with an API for creating and managing threads. The three primary libraries are **Pthreads** (POSIX standard, used on UNIX, Linux, macOS), **Windows Threads**, and **Java Threads**.
- **Execution Strategies:**
  - **Asynchronous threading:** The parent thread creates a child thread and then resumes its execution, so both run concurrently and independently.
  - **Synchronous threading:** The parent thread creates one or more children and must wait (`join`) for all of them to terminate before it resumes.

## 5. Implicit Threading
As systems scale to thousands of threads, developers increasingly transfer the creation and management of threading from application developers to compilers and run-time libraries.
- **Thread Pools:** Creates a number of threads at startup that sit in a pool and wait for work. This limits the maximum number of threads (saving resources) and avoids the overhead of creating new threads for every task.
- **Fork-Join:** A synchronous model where the main thread forks (spawns) tasks and then joins them when they are completed.
- **OpenMP:** A set of compiler directives and APIs for C/C++ that support parallel programming in shared-memory environments.
- **Grand Central Dispatch (GCD):** Apple's technology that manages threads behind the scenes using dispatch queues.
- **Intel Threading Building Blocks (TBB):** A C++ template library for parallel programming.

## 6. Threading Issues
- **`fork()` and `exec()`:** Semantics vary. On some UNIX systems, `fork()` duplicates all threads, while on others it duplicates only the calling thread. If `exec()` is called immediately after `fork()`, duplicating all threads is unnecessary.
- **Signal Handling:** Signals are used in UNIX to notify a process of an event. Synchronous signals are delivered to the thread causing the signal. Asynchronous signals are more complex and can be delivered to every thread, a specific thread, or a dedicated signal-handling thread.
- **Thread Cancellation:** Terminating a thread before it has finished.
  - **Asynchronous cancellation:** Terminates the target thread immediately (can lead to resource leaks).
  - **Deferred cancellation:** The target thread periodically checks a flag at safe points (cancellation points) to determine if it should terminate itself.
- **Thread-Local Storage (TLS):** Allows each thread to have its own copy of data. Unlike local variables, TLS data is visible across function invocations.
- **Scheduler Activations:** A communication mechanism between the user-thread library and the kernel using Lightweight Processes (LWP). The kernel informs the library of events via **upcalls**, allowing the library to adjust thread scheduling.

## 7. Operating System Examples
- **Windows:** Implements the One-to-One model. Each thread has an Executive Thread Block (ETHREAD), a Kernel Thread Block (KTHREAD), and a Thread Environment Block (TEB) in user space.
- **Linux:** Linux does not distinguish between processes and threads, referring to both as **tasks**. Threads are created using the `clone()` system call, which accepts flags specifying the degree of sharing (memory space, open files, signal handlers) between the parent and child tasks.
