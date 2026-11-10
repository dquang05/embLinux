# Core Concepts: Process

*Core knowledge about Processes on Linux.*

## 1. Process & Program
- **Program:** An executable file on disk (e.g., in ELF format), containing machine instructions and passive data.
- **Process:** A program in execution within memory.
- **Process Control Block (PCB):** A conceptual OS structure for process state. Linux represents each task using `task_struct` and related structures; the PCB is not a literal one-to-one name for all of them.
- **Identifiers (PID & PPID):** A PID identifies a process within a PID namespace; PPID identifies its parent as visible there. Each PID namespace has an init process with PID 1; it may be `systemd`, but need not be.
- **Virtual Memory Management:**
  - Relies on the **Locality of reference** principle; only part of the program needs to be in RAM at a time.
  - Virtual memory is divided into **pages**, RAM into **page frames**. The Kernel maintains a **Page table** mapping virtual memory to physical memory with the help of the hardware PMMU (Paged Memory Management Unit).
  - *Illustration (Virtual to Physical Memory Mapping):*
    | Virtual Memory (Process) | Page Table | Physical Memory (RAM) |
    |--------------------------|------------|-----------------------|
    | Page 0 (Code)            | → Frame X → | Frame X               |
    | Page 1 (Data)            | → Frame Y → | Frame Y               |
    | Page 2 (Unused)          | → (Invalid) | Frame Z (Free)        |
    | Page 3 (Stack)           | → Frame W → | Frame W               |
- **Memory Layout:** A simplified illustration typical of architectures such as x86-64; stack direction depends on the architecture, and allocators may use separate mappings rather than one growing heap.
  
  | Segment | Description | Direction |
  |---------|-------------|-----------|
  | **Stack** | Function parameters, return addresses, local vars | Grows downwards ↓ |
  | **...** | *Free Space* | |
  | **Heap** | Dynamically allocated memory (`malloc`, `new`) | Grows upwards ↑ |
  | **BSS** | Uninitialized global variables | Fixed |
  | **Data** | Initialized global variables | Fixed |
  | **Text** | Executable machine code (read-only) | Fixed |

## 2. Process Lifecycle
- **Basic States:** New, Ready (waiting for CPU), Running (executing on CPU), Waiting (blocked for I/O), Terminated (exited).
- **Context Switch:** The OS saves and restores execution state to schedule tasks. Switching introduces overhead but is necessary for sharing a CPU among tasks.
- **Special Processes:**
  - **Orphan:** When the parent dies, Linux reparents the child to an eligible surviving thread of the parent, a nearest ancestor subreaper, or the relevant namespace init process. Namespace relationships can affect which init process adopts it.
  - **Zombie (`<defunct>`):** A terminated child awaiting reaping has released its user address space, but retains kernel bookkeeping including its PID and exit status. It cannot be killed by `SIGKILL`; explicit `SIGCHLD` ignore or `SA_NOCLDWAIT` can prevent zombies.

## 3. Process Management (Creation, Execution, Termination)
Linux manages the process lifecycle via these core APIs:
1. **Creation (`fork()`):** Clones the process to create a child that is an almost identical memory copy of the parent.
2. **Execution (`execve()`):** The process discards its old memory and loads a new program to run under the same PID.
3. **Termination (`exit()`):** This C library function runs exit handlers and flushes stdio before termination. `_exit()` is a lower-level interface that does not perform that stdio cleanup.
4. **Cleanup (`wait()` / `waitpid()`):** The parent process waits to receive the child's Exit Status to completely clean up the Zombie.

> [!NOTE]
> 📚 **Deep Dive:** 
> - Memory sharing mechanism (Copy-On-Write) of `fork`.
> - Other calls like `vfork()`, `clone()` (for Threads).
> - Passing arguments to `exec()` family functions.
> - Security considerations (`FD_CLOEXEC` flag and `system()` function).
> 👉 See details at: [**process_creation_execution.md**](process_creation_execution.md)

## 4. Interprocess Communication (IPC)
- **Purpose:** To share data and operate concurrently (multi-core processing).
- **Main Mechanisms:**
  - **Shared Memory:** Mapping into the same physical RAM region. Extremely fast but requires manual synchronization to avoid data races.
  - **Message Passing:** Safe message transmission via OS (e.g., Message Queues, UNIX sockets).
  - **Pipes:** On Linux, unnamed pipes and named pipes (FIFOs) provide unidirectional byte streams. An unnamed pipe is commonly inherited by related processes; unrelated processes can open a FIFO if permissions allow. Two channels or a socket can provide two-way communication.
  - **Signals:** Asynchronous notifications and control mechanisms, including `SIGINT` and `SIGKILL`, can affect a process or thread depending on the signal and how it is sent.

## 5. Groups, Sessions & Job Control
- **Structure:** A **Session** contains multiple **Process Groups**. A Group contains multiple Processes.
- **Foreground vs Background:**
  - A controlling terminal has one foreground process group; terminal-generated signals such as Ctrl+C and Ctrl+Z target that group. A background read normally generates `SIGTTIN`; a background write generates `SIGTTOU` when `TOSTOP` is enabled. Signal disposition and orphaned-group conditions can alter the outcome.
- **Hangup Signal (SIGHUP):**
  - A terminal hangup can send `SIGHUP` to its session leader. An interactive Bash receiving `SIGHUP` resends it to its jobs, except those excluded with `disown` or `disown -h`. A job can handle or ignore the signal; receiving it does not guarantee termination. Bash's `huponexit` setting also affects interactive login shell exit.

## 6. Scheduling & Resource Management
Modern operating systems automatically and fairly distribute resources.
- **CPU Scheduling:** Linux historically used CFS for fair scheduling and began transitioning to EEVDF in Linux 6.6. Nice values influence fair scheduling; realtime policies have different rules and constraints.
  - 👉 Details: [**cpu_scheduling.md**](cpu_scheduling.md)
- **Resource Limits:** You can measure resource consumption or set hard/soft limits (like max RAM, max number of processes to prevent Fork Bombs).
  - 👉 Details: [**process_resources.md**](process_resources.md)

---
## 7. Notable Practical Exercises (From OS Exercises)
- Use the `/proc/<pid>` file to retrieve PCB information and monitor process state via a Kernel Module.
- Program a Kernel Module to iterate through the Process Tree via the `task_struct` using a linked list (`list_head`) and the `for_each_process()` macro.
- Use pipes/shared memory to compute a sequence (like the Collatz sequence) between parent and child processes.
