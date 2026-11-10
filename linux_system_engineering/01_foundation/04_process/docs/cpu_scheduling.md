# CPU Scheduling and Priorities

## 1. Basic Scheduling Concepts

- **CPU-I/O Burst Cycle:** Processes constantly alternate between computation (CPU burst) and waiting for peripherals/input (I/O burst). Processes with heavy computation are *CPU-bound*, while those with heavy I/O are *I/O-bound*.
- **Preemptive vs Nonpreemptive:** 
  - *Nonpreemptive:* The process voluntarily yields the CPU (waiting for I/O or terminating).
  - *Preemptive:* The OS has the authority to interrupt a running process to allocate the CPU to a higher-priority process. Most modern OSes use this approach.

## 2. Classic Scheduling Algorithms
1. **FCFS (First-Come, First-Served):** First to arrive, first served. Susceptible to the "Convoy effect" where short tasks must wait for a long one.
2. **SJF (Shortest-Job-First):** Selects the process with the shortest next CPU burst. Optimizes wait time but is difficult to predict execution time.
3. **Round-Robin (RR):** Allocates a small time interval (time quantum) to each process to run alternately. Excellent for interactive systems.
4. **Priority Scheduling:** Prioritized based on a score. Prone to *Starvation*, where low-priority processes are never executed. Mitigated by *Aging* (gradually increasing priority based on wait time).

## 3. Linux Scheduling

### Linux CFS (Completely Fair Scheduler)
Historically, CFS became Linux's default fair scheduler in Linux 2.6.23. Linux began transitioning fair scheduling to EEVDF in Linux 6.6; exact behavior depends on the kernel version and configuration.
- CFS does not use fixed *time quanta* but allocates a "percentage" of CPU time based on priority (nice value).
- Historically, CFS used a Red-Black tree ordered by virtual runtime (`vruntime`) and selected the leftmost runnable task. EEVDF instead selects an eligible task with the earliest virtual deadline.

### Priorities and "Nice" values
The `nice` attribute adjusts the weight of tasks under Linux fair scheduling:
- Ranges from **-20 (highest priority)** to **+19 (lowest priority)**. Default is 0.
- Unprivileged processes normally cannot lower a nice value, but on Linux a suitable `RLIMIT_NICE` can allow it. `CAP_SYS_NICE` can grant additional permission; target ownership and permission checks also apply.
- Use `getpriority()` and `setpriority()` functions to adjust.

### Realtime Scheduling
Designed for processes requiring extremely low latency. Linux supports POSIX Realtime APIs:
- **SCHED_FIFO (First-In First-Out):** The process runs until it terminates, voluntarily blocks, or is preempted by a higher-priority realtime process. Does not share time slices.
- **SCHED_RR (Round-Robin):** Similar to FIFO, but processes with the same priority share time slices.
- > [!IMPORTANT]
  > Runnable `SCHED_FIFO` and `SCHED_RR` tasks normally outrank `SCHED_OTHER` tasks on eligible CPUs, subject to affinity, higher scheduling classes and RT bandwidth constraints such as throttling. A runaway realtime task can still make a system unresponsive.

## 4. Multicore Scheduling
- **SMP (Symmetric Multiprocessing):** Cores share the load. Utilizes Load Balancing (Push and Pull migration) to distribute processes from busy cores to idle ones.
- **Processor Affinity:** Migrating execution between CPU cores may reduce cache locality and cause additional cache misses and memory-access costs, depending on the workload and architecture; it does not lose valid program data or bypass cache coherence. On Linux, `sched_setaffinity()` sets a task/thread's eligible CPUs, so pinning a thread may improve locality but does not guarantee better performance.
