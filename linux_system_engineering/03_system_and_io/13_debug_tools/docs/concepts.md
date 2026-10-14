# Core Concepts: Linux Debug Tools & Profiling

## Reading Limits & References
- **TLPI**: Appendix A (Tracing System Calls: `strace` and `ltrace`) from line **74158** to **74253** in `Syntax.txt`.
- **GDB Crash Labs**: [gdb_crash_labs.md](gdb_crash_labs.md) (Live Debugging, Core Dumps, Stack Canary, Crash Signatures).
- **Valgrind Memcheck Labs**: [valgrind_tutorial.md](valgrind_tutorial.md) (Heap Leaks, Invalid Access, Use-After-Free, Double Free).
- **Valgrind Helgrind Labs**: [valgrind_thread_tutorial.md](valgrind_thread_tutorial.md) (Data Races, Lock Inversion Deadlocks).
- **Heaptrack Labs**: [heaptrack_labs.md](heaptrack_labs.md) (Low-overhead Heap Profiling, Peak Memory vs. Leaks).

---

This document synthesizes standard Linux diagnostics, tracing, and profiling tools. Mastering these tools enables Embedded Linux and Systems engineers to pinpoint and resolve fatal crashes (`SIGSEGV`, `SIGABRT`), memory leaks, I/O bottlenecks, and subtle multithreaded concurrency hazards.

---

## 1. Linux Debug & Profiling Tool Taxonomy

```
+----------------------------------------------------------------------------------------------------+
|                                    LINUX DEBUG & PROFILING TOOLS                                   |
+------------------------------------+-------------------------------+-------------------------------+
| 1. System Call & Library Tracing   | 2. Memory & Thread Analysis   | 3. Crash & Interactive Debug  |
+------------------------------------+-------------------------------+-------------------------------+
| • strace: User/Kernel boundary     | • Valgrind Memcheck: Deep     | • GDB: Interactive live debug,|
| • ltrace: Dynamic library (.so)    |   memory correctness (10-50x) |   breakpoints, call stack,    |
| • ptrace: Underlying kernel        | • Valgrind Helgrind: Data     |   registers, core dump.       |
|   primitive                        |   races & deadlocks           |                               |
|                                    | • Heaptrack: Lightweight fast |                               |
|                                    |   heap profiler (1.2x overhead|                               |
|                                    |   ideal for Embedded Linux).  |                               |
+------------------------------------+-------------------------------+-------------------------------+
```

---

## 2. Tracing System Calls & Library Calls (TLPI Appendix A)

### 2.1 `strace` — Inspecting the User / Kernel Space Boundary
- **Mechanism**: `strace` leverages the `ptrace()` system call to intercept and decode every system call executed by a process before transitioning to Kernel Mode.
- **Output Format**: `syscall(arguments...) = return_value [errno]`. Bitmasks, structs, pointers, and text strings are decoded into human-readable symbolic constants (e.g., `O_RDONLY`, `PROT_READ|PROT_WRITE`).
- **Essential Command-Line Options**:
  - `strace -c <command>`: **Statistical Profiling**: Aggregates call counts, error counts, and cumulative CPU time spent inside each system call. Indispensable for identifying I/O latency bottlenecks.
  - `strace -T <command>`: Measures individual system call execution duration with microsecond precision.
  - `strace -e trace=<set> <command>`: Filters system calls by subsystem: `-e trace=file`, `-e trace=network`, `-e trace=process`, `-e trace=ipc`, or specific calls `-e trace=open,read,write,close`.
  - `strace -p <pid>`: Attaches to a running daemon or process without restarting it.
  - `strace -f -o trace.log <command>`: Automatically traces all child processes spawned via `fork()` / `clone()`.
  - `strace -s <size> <command>`: Increases the string preview length limit (default is 32 characters).

### 2.2 `ltrace` — Dynamic Library Call Tracing
- Analogous to `strace`, but intercepts user-space shared library calls (`glibc`, `libpthread`, third-party `.so` libraries) by hooking into the **Procedure Linkage Table (PLT)**.

---

## 3. Interactive Debugging & Crash Analysis with GDB

👉 **Detailed Guide: [gdb_crash_labs.md](gdb_crash_labs.md)**

### Key Takeaways:
- **Compilation Standard**: Always compile with `-g -O0` (preserves full DWARF debug symbols and disables compiler optimization/inlining so GDB steps line-by-line accurately).
- **Core Navigation Commands**:
  - `run (r)`, `continue (c)`, `next (n)`, `step (s)`, `finish (fin)`.
  - `break (b)`: Sets breakpoints at function entry, file line, or conditional expressions (`condition N expr`).
  - `backtrace (bt)`: Displays the entire active call stack frame hierarchy at break or crash time.
  - `frame <N> (f)`: Switches context to frame $N$ to inspect caller local variables (`print`, `info locals`, `info args`).
  - `x/<count><format><unit> <addr>`: Inspects raw memory bytes on Stack or Heap.
- **Diagnosing the Three Common Crash Signatures**:
  1. **Null Pointer Dereference (`SIGSEGV`)**: Function parameters in frame 0 appear as `0x0`. Navigate up the call stack via `frame 1` to find the unvalidated pointer assignment.
  2. **Buffer Overflow & GCC Stack Canary (`SIGABRT`)**: GCC places a random 8-byte Canary at `[RBP - 8]`. When a function returns, if the canary has been overwritten (e.g., with `0x41414141`), GCC immediately executes `__stack_chk_fail` to terminate the process before a hijacked return address is executed.
  3. **Stack Overflow (`SIGSEGV`)**: Infinite recursion exhausts the default stack limit (typically 8MB on Linux). Backtrace displays thousands of identical recurring stack frames.

---

## 4. Memory Correctness Verification with Valgrind Memcheck

👉 **Detailed Guide: [valgrind_tutorial.md](valgrind_tutorial.md)**

### Key Takeaways:
- **Architecture**: Runs binaries on a synthetic CPU (VEX JIT engine), tracking allocation and validity status of every memory byte via Addressability (A-bits) and Validity (V-bits). Tradeoff: 10x-50x runtime slowdown.
- **Four Memory Leak Categories**:
  - `Definitely lost`: No pointer to the allocated block remains $\rightarrow$ **Definite memory leak requiring immediate fix**.
  - `Indirectly lost`: Leaked memory referenced solely by other leaked blocks (e.g., child nodes of a lost tree or linked list).
  - `Possibly lost`: Only interior pointers reference the block.
  - `Still reachable`: Memory was never `free()`'d, but valid pointers still exist at exit (common with static/global resources).
- **Dangerous Memory Defects Detected**:
  - `Invalid read / Invalid write`: Out-of-bounds heap buffer access.
  - `Use after free`: Accessing dangling pointers after `free()`.
  - `Double free`: Invoking `free()` twice on the identical heap address.
  - `Conditional jump or move depends on uninitialised value(s)`: Branching on uninitialized variables.

---

## 5. Thread Concurrency & Synchronization with Valgrind Helgrind

👉 **Detailed Guide: [valgrind_thread_tutorial.md](valgrind_thread_tutorial.md)**

### Key Takeaways:
- **Data Races**: Occurs when two or more threads concurrently access the same memory address, at least one access is a WRITE, and **no common mutex lock** protects that memory. Helgrind uses the Happens-Before relationship to catch data races even before race-induced corruption manifests.
- **Deadlocks (Lock Order Inversion)**:
  - Thread 1 locks Mutex A $\rightarrow$ Waits for Mutex B.
  - Thread 2 locks Mutex B $\rightarrow$ Waits for Mutex A.
  - Helgrind constructs a dynamic Lock Graph and immediately reports lock order violations on the very first execution pass.

---

## 6. High-Performance Embedded Memory Profiling with Heaptrack

👉 **Detailed Guide: [heaptrack_labs.md](heaptrack_labs.md)**

### Key Takeaways:
- **Why Heaptrack over Valgrind on Embedded Systems?**
  - Valgrind's 10x-50x slowdown and massive memory footprint often exhausts RAM and stalls real-time hardware targets (Raspberry Pi, i.MX6/8, routers).
  - `heaptrack` leverages `LD_PRELOAD` to intercept `malloc`/`free` with negligible overhead (~1.2x slowdown), making it viable for production profiling on real hardware.
- **Peak Memory vs. Memory Leaks**:
  - An application can have **zero memory leaks**, yet still be terminated by the Linux **OOM Killer** if a transient memory spike exceeds device RAM (**Peak Memory Consumption**).
  - `heaptrack_print` pinpoints:
    - The exact call stack responsible for the peak memory threshold.
    - Call stacks exhibiting high allocation churn (excessive temporary allocations wasting CPU cycles).
