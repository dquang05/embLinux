# Designing Lock-Free Data Structures

## 1. Design concurrency?

Data structures with concurrency mean that multiple threads can access and modify the data structure simultaneously without causing data corruption or inconsistency.

Goals:
 - Safety
 - Genuine concurrency
 - Minimize lock scope

## 2. Design Lock-Free Data Structures

Look back how to make thread-safe data structures in [threads](../threads):

- Ensure no threads can access the data structure while it is being modified by another thread.
- Package operations to avoid race conditions.
- Safe for exception handling.
- Avoid deadlocks and other synchronization issues.
- Using scope: which functions can be called concurrently, and which cannot.

## 3. How to optimize for concurrency?

- Can the scope of locks be restricted to allow some parts of an operation to be performed outside the lock?
- Can different parts of the data structure be protected with different mutexes?
- Do all operations require the same level of protection?
- Can a simple change to the data structure improve the opportunities for concurrency without affecting the operational semantics?