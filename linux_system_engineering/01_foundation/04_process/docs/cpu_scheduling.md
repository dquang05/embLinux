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
This is the current default scheduler in Linux.
- CFS does not use fixed *time quanta* but allocates a "percentage" of CPU time based on priority (nice value).
- It uses a Red-Black tree to track virtual runtime (`vruntime`). The process with the smallest `vruntime` (leftmost node on the tree) is allocated the CPU next.

### Priorities and "Nice" values
The `nice` attribute allows adjusting process priority in CFS:
- Ranges from **-20 (highest priority)** to **+19 (lowest priority)**. Default is 0.
- Normal users can only *decrease* their priority (increase `nice` > 0). Only privileged users (root) can *increase* priority (negative nice).
- Use `getpriority()` and `setpriority()` functions to adjust.

### Realtime Scheduling
Designed for processes requiring extremely low latency. Linux supports POSIX Realtime APIs:
- **SCHED_FIFO (First-In First-Out):** The process runs until it terminates, voluntarily blocks, or is preempted by a higher-priority realtime process. Does not share time slices.
- **SCHED_RR (Round-Robin):** Similar to FIFO, but processes with the same priority share time slices.
- > [!IMPORTANT]
  > Any realtime process (priority 1 to 99) will always preempt all normal processes (SCHED_OTHER). Developers must be extremely careful to avoid hanging the system if a realtime process gets stuck in an infinite loop.

## 4. Multicore Scheduling
- **SMP (Symmetric Multiprocessing):** Cores share the load. Utilizes Load Balancing (Push and Pull migration) to distribute processes from busy cores to idle ones.
- **Processor Affinity:** Moving a process from one core to another causes data loss in the cache (Cache miss), degrading performance. Developers can use the `sched_setaffinity()` function to "pin" a process to a specific CPU core to optimize computational speed.
