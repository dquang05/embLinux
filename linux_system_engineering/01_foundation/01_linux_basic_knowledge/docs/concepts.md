# Foundation: Linux Basic Knowledge & System Architecture

This document summarizes the core concepts of Operating Systems and Linux System Programming, drawn from two classic resources: *Operating System Concepts* and *The Linux Programming Interface* (TLPI). These are the mandatory foundational topics for any Embedded Linux Developer.

---

## 1. Introduction & Standards

### The Role of an Operating System
An Operating System (OS) acts as the intermediary between users (or applications) and computer hardware. Its two primary roles are:
- **Resource Manager:** Allocates CPU, memory, and I/O resources fairly, safely, and efficiently.
- **Control Program:** Manages the execution of user programs to prevent errors and improper use of the computer.

### History & POSIX Standards
- **UNIX & Linux:** Linux was designed inheriting UNIX philosophies, such as *"Everything is a file"* and writing small, modular programs that do one thing well.
- **POSIX (Portable Operating System Interface):** A core standard (IEEE/ISO) that allows C/C++ source code to be compiled and run across various operating systems (Linux, macOS, UNIX variants). Standard APIs like `open()`, `read()`, `fork()`, and `pthread` comply with POSIX.1.

---

## 2. System Architecture & Protection

### Protection Rings (User Mode vs. Kernel Mode)
To protect the system from buggy or malicious applications, modern CPUs support **Dual-mode operation**:
- **Kernel Mode (Ring 0):** The highest privilege level. Code executing here (the Linux Kernel) has unrestricted access to all CPU instructions and hardware memory space.
- **User Mode (Ring 3):** A restricted privilege mode. User processes running here cannot execute hardware instructions directly (e.g., I/O commands). To interact with hardware, they must request the OS to perform the task via a System Call.

### Interrupts and Timers
- **Interrupts:** A hardware or software mechanism that signals the CPU to suspend its current execution thread to handle an urgent event (e.g., a key press, incoming network packet, or disk I/O completion). In Embedded Linux, the **Interrupt Handler** is the backbone of all Device Drivers.
- **Hardware Timer:** The kernel uses a hardware timer to generate periodic interrupts (Timer Interrupts). Thanks to the timer, the OS can preempt a process that has been running too long, ensuring no single process monopolizes the CPU (the principle of Multitasking).

---

## 3. Kernel Architectures

### Monolithic Kernel
- **Characteristics:** Linux is a Monolithic Kernel. All core components (Memory Manager, Process Scheduler, File System, Device Drivers) reside in a single large binary image and execute entirely in Kernel Mode.
- **Pros:** Extremely high performance, as kernel modules can invoke each other's functions directly without Context Switch overhead.
- **Cons:** A bug in any module (e.g., a faulty third-party driver accessing invalid memory) can crash the entire system (Kernel Panic).

### Microkernel & Loadable Kernel Modules (LKM)
- **Microkernel:** In contrast to a Monolithic design, a Microkernel moves as many services as possible (like File Systems and Drivers) into User Mode as separate processes. The kernel only retains IPC (Inter-Process Communication), basic Scheduling, and Memory Management. It is highly secure but suffers from performance overhead due to frequent Context Switches.
- **Linux LKM (The Perfect Hybrid):** Despite being Monolithic, Linux allows code fragments (modules) to be loaded or unloaded into Kernel space dynamically while the system is running (using `insmod` / `rmmod`). This is a crucial feature for developing Device Drivers in Embedded Linux, eliminating the need to recompile the entire kernel or reboot the machine.

---

## 4. System Calls & APIs

### What is a System Call?
A System Call is the gateway and the sole programming interface for a User Mode process to request the Kernel to perform privileged operations (reading/writing files, allocating memory, managing networks).

When a System Call is invoked:
1. The process triggers a **Software Interrupt** (often called a Trap).
2. A **Context Switch** occurs, transitioning the CPU from User Mode to Kernel Mode.
3. The kernel verifies the validity of the parameters and safely executes the task.
4. Execution returns to User Mode, allowing the process to continue.

### System Call vs. Standard C Library (glibc)
C programmers rarely write raw Assembly code to invoke System Calls. Instead, they use APIs provided by the standard C library (`glibc`, or `uClibc`/`musl` in embedded systems).

> **Example:** The `printf()` function in the C library is a Library Function. It formats the string within User space memory and then implicitly invokes a System Call (usually `write()`) underneath to request the Kernel to print that string to the screen.

---

## 5. Compilation, Linkers & Loaders

The process of translating C source code into an Executable program involves several steps:
1. **Pre-processing:** Resolves directives like `#include` and `#define`.
2. **Compilation:** Translates C code into Assembly code.
3. **Assembly:** Translates Assembly code into binary Object files (`.o`).
4. **Linking:** The Linker combines `.o` files with libraries to create the final Executable file (typically in ELF format on Linux).
    - **Static Library (`.a`):** Extracts and copies the machine code of the library directly into the executable. The resulting file is larger but runs independently without requiring external library installations (Portable).
    - **Shared/Dynamic Library (`.so`):** Contains only references. The OS loads the library into RAM dynamically when the program is executed. This saves RAM when multiple applications share the same library.
5. **Loading:** When you run an application (e.g., `./app`), the OS **Loader** reads the ELF file, allocates virtual memory space, loads the machine code and any required Shared Libraries into RAM, and begins execution at the `main()` function.

---

## 6. System Boot Process

In an Embedded Linux environment, the boot process follows a strict sequence, divided into multiple stages:
1. **Boot ROM / ROM Code:** The processor runs internal code etched on the silicon to perform initial, basic hardware configuration (like clock and pin muxing).
2. **Bootloader (e.g., U-Boot):** Loaded into internal SRAM, it initializes the main memory (DDR) and prepares network interfaces. It then reads the Linux Kernel image (zImage/uImage) and the Device Tree Blob (DTB) from storage (SD card/eMMC/Flash) and loads them into RAM.
3. **Kernel Initialization:** The kernel decompresses itself, bootstraps, initializes memory management and the CPU scheduler, loads fundamental drivers, and finally mounts the Root File System (rootfs).
4. **Init Process:** Once the kernel has booted, it spawns the first process in User space (always Process ID = 1). Modern embedded systems typically use `systemd`, `sysvinit`, or `busybox init`. This process executes scripts to mount drives, bring up network services, configure IP addresses, and launch the main application or interface.

---

## 7. Linux-Specific Interfaces

### "Everything is a file" Philosophy
In Linux, all resources are abstracted as files and accessed using a common set of System Calls (`open`, `read`, `write`, `close`).
- **Regular Files:** Standard text or binary files.
- **Devices:** Interfaces to hardware components located in the `/dev` directory (e.g., `/dev/ttyS0` for a Serial Port, `/dev/sda` for a Hard Drive).

### The `/proc` Virtual File System
The `/proc` directory is a Virtual File System that does not exist on disk but resides entirely in RAM. It serves as a "window" for User Mode to peek into and interact directly with the kernel's internal data structures:
- `/proc/cpuinfo`: Retrieves CPU architecture information.
- `/proc/meminfo`: Retrieves RAM usage information.
- `/proc/[PID]`: Contains all information (state, memory, open files) about a currently running Process.

> **Note:** `/proc` (along with `/sys`) is the most powerful and immediate debugging tool in Linux. Instead of writing C code to call complex APIs, you can simply use standard shell commands like `cat` or `echo` to read/write parameters or interact with kernel modules!
