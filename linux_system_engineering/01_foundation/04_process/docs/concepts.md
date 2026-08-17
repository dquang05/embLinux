# Core Concepts: Process

*Core knowledge about Processes on Linux.*

## 1. Process & Program
- **Program:** An executable file on disk (e.g., in ELF format), containing machine instructions and passive data.
- **Process:** A program in execution within memory.
- **Process Control Block (PCB):** A data structure used by the OS (called `task_struct` in Linux) to store the state of the process (Registers, Program Counter, Open files, etc.).
- **Identifiers (PID & PPID):** Each process has a unique ID (`PID`). It also stores the ID of its parent process (`PPID`). The system forms a process tree rooted at `systemd` (PID 1).
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
- **Memory Layout:**
  
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
- **Context Switch:** The process of the OS saving the state of the old process and loading the state of the new process to share the CPU. Overhead purely.
- **Special Processes:**
  - **Orphan:** The parent process dies first. The orphan is automatically adopted by `init` (PID 1).
  - **Zombie (`<defunct>`):** The child process has terminated, but the parent hasn't called `wait()`. It consumes no RAM but occupies a slot in the OS process table. Cannot be killed by `SIGKILL`.

## 3. Process Management (Creation, Execution, Termination)
Linux manages the process lifecycle via 4 core system calls:
1. **Creation (`fork()`):** Clones the process to create a child that is an almost identical memory copy of the parent.
2. **Execution (`execve()`):** The process discards its old memory and loads a new program to run under the same PID.
3. **Termination (`exit()`):** The process exits, flushes buffers, and returns an Exit Status.
4. **Cleanup (`wait()` / `waitpid()`):** The parent process waits to receive the child's Exit Status to completely clean up the Zombie.

> [!NOTE]
> 📚 **Deep Dive:** 
> - Memory sharing mechanism (Copy-On-Write) of `fork`.
> - Other calls like `vfork()`, `clone()` (for Threads).
> - Passing arguments to `exec()` family functions.
> - Security considerations (`FD_CLOEXEC` flag and `system()` function).
> 👉 See details at: [**process_creation_execution.md**](file:///home/quangtran/Projects/embLinux/linux_system_engineering/01_foundation/04_process/docs/process_creation_execution.md)

## 4. Interprocess Communication (IPC)
- **Purpose:** To share data and operate concurrently (multi-core processing).
- **Main Mechanisms:**
  - **Shared Memory:** Mapping into the same physical RAM region. Extremely fast but requires manual synchronization to avoid data races.
  - **Message Passing:** Safe message transmission via OS (e.g., Message Queues, UNIX sockets).
  - **Pipes:** Data conduits. *Ordinary pipes* (unnamed) are unidirectional for parent-child. *Named pipes* (FIFOs) are bidirectional for any process.
  - **Signals:** Sending interrupt signals (e.g., `SIGKILL`, `SIGINT`) to control process flow remotely.

## 5. Groups, Sessions & Job Control
- **Structure:** A **Session** contains multiple **Process Groups**. A Group contains multiple Processes.
- **Foreground vs Background:**
  - In a Session (e.g., your Terminal), only **one** Foreground Group is allowed to read keyboard input and receive keyboard signals (Ctrl+C, Ctrl+Z) at a time. Background Groups run silently; attempting to read/write to the terminal will cause the OS to suspend them.
- **Hangup Signal (SIGHUP):**
  - When you close a Terminal, the OS sends a disconnect signal (`SIGHUP`) to the Session Leader (usually the bash shell). Bash forwards `SIGHUP` to kill all Jobs (including background ones) running under it.

## 6. Scheduling & Resource Management
Modern operating systems automatically and fairly distribute resources.
- **CPU Scheduling:** Linux uses the CFS (Completely Fair Scheduler) to share computation time. You can influence priority via "Nice values" or enforce Realtime algorithms.
  - 👉 Details: [**cpu_scheduling.md**](file:///home/quangtran/Projects/embLinux/linux_system_engineering/01_foundation/04_process/docs/cpu_scheduling.md)
- **Resource Limits:** You can measure resource consumption or set hard/soft limits (like max RAM, max number of processes to prevent Fork Bombs).
  - 👉 Details: [**process_resources.md**](file:///home/quangtran/Projects/embLinux/linux_system_engineering/01_foundation/04_process/docs/process_resources.md)

---
## 7. Notable Practical Exercises (From OS Exercises)
- Use the `/proc/<pid>` file to retrieve PCB information and monitor process state via a Kernel Module.
- Program a Kernel Module to iterate through the Process Tree via the `task_struct` using a linked list (`list_head`) and the `for_each_process()` macro.
- Use pipes/shared memory to compute a sequence (like the Collatz sequence) between parent and child processes.