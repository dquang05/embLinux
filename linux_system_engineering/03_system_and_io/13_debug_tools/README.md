# Chapter 13: Linux Debug Tools & Profiling

## Objectives
- Master **System Call & Dynamic Library Tracing** using `strace` and `ltrace` to inspect application behavior, error returns (`errno`), file/socket activity, and perform latency profiling (`strace -c`).
- Gain proficiency in **GDB (GNU Debugger)** for interactive live debugging, setting breakpoints, stepping through execution, and diagnosing crash signatures: Null Pointer Dereferences (`SIGSEGV`), Buffer Overflows & GCC Stack Canary violations (`SIGABRT`), and Stack Exhaustion.
- Detect and eliminate critical heap memory defects with **Valgrind Memcheck**: Definitely Lost vs. Indirectly Lost leaks, Invalid Read/Write bounds overruns, Use-After-Free, and Double Free.
- Analyze POSIX thread synchronization hazards with **Valgrind Helgrind**: pinpointing Data Races on unprotected shared variables and detecting Lock-Order Inversion Deadlocks.
- Perform high-efficiency, production-grade memory profiling on embedded targets using **Heaptrack**, distinguishing transient Peak Memory consumption spikes from accumulated Memory Leaks with minimal runtime overhead (~1.2x).

## References
- **TLPI (The Linux Programming Interface)**:
  - Appendix A: Tracing System Calls (`strace`, `ltrace`, `ptrace` mechanics, `-c`, `-e trace=`, `-p`, `-f`).
- **GDB Documentation**:
  - GDB User Manual: Call stack inspection, frame manipulation, breakpoints, memory examine commands (`x/`).
- **Valgrind Core & Tool Documentation**:
  - Memcheck: Dynamic Binary Instrumentation, A-bits and V-bits tracking, memory leak classification.
  - Helgrind: Eraser and Happens-Before algorithms, Lock Graph analysis.
- **Heaptrack**:
  - `heaptrack` and `heaptrack_print` allocation tree analysis via `LD_PRELOAD`.

## Documentation Guide
- [Core Concepts & Taxonomy](docs/concepts.md)
- [GDB Crash Analysis Labs](docs/gdb_crash_labs.md)
- [Valgrind Memcheck Memory Debugging](docs/valgrind_tutorial.md)
- [Valgrind Helgrind Thread & Concurrency Debugging](docs/valgrind_thread_tutorial.md)
- [Heaptrack Memory & Peak Consumption Profiling](docs/heaptrack_labs.md)

## Tool Quick Reference

### 1. System Call Tracing (`strace`)
```bash
# Basic trace
strace ./my_program

# Filter specific syscalls
strace -e trace=open,read,write,close ./my_program

# Statistical summary of execution time and call counts
strace -c ./my_program

# Attach to a running daemon process
strace -p <PID> -f
```

### 2. Interactive Debugger (`gdb`)
```bash
# Compile with debug symbols and no optimization
gcc -g -O0 -o my_program my_program.c

# Run in GDB
gdb ./my_program
(gdb) run
(gdb) backtrace
(gdb) frame 1
(gdb) print my_var
```

### 3. Memory Checking (`valgrind`)
```bash
# Detect memory leaks and invalid accesses
valgrind --leak-check=full --show-leak-kinds=all ./my_program

# Detect data races and thread deadlocks
valgrind --tool=helgrind ./my_thread_program
```

### 4. Low-Overhead Heap Profiling (`heaptrack`)
```bash
# Profile heap allocations
heaptrack ./my_program

# Analyze output file
heaptrack_print heaptrack.my_program.<PID>.zst | head -n 40
```
