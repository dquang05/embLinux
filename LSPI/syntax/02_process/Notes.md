# Linux Process Management and Memory Architecture

## 1. Process Identification

A **process** is a dynamic instance of a program in execution. The operating system manages each process using unique identifiers.

### `getpid()`

```c
pid_t getpid(void);
```

Returns the **Process ID (PID)** of the calling process.

**Common Uses**

* Generating unique temporary filenames

```text
/tmp/app_1234.log
```

* Providing a target identifier for inter-process signaling

**Notes**

* Typical default PID limits are often around `32767`, although modern Linux systems may support much larger ranges.

---

### `getppid()`

```c
pid_t getppid(void);
```

Returns the **Parent Process ID (PPID)** of the calling process.

#### Orphan Process Mechanism

If a parent process terminates before its child, the child becomes an **orphan process**.

The Linux kernel automatically reassigns the orphan to the `init` process (PID = 1) or an equivalent system manager.

In this situation:

```c
getppid(); /* returns 1 */
```

---

# 2. Virtual Memory Layout

The kernel divides a process's virtual address space into several memory segments arranged from low addresses to high addresses.

```text
Low Address
+-------------------+
| Text Segment      |
+-------------------+
| Initialized Data  |
+-------------------+
| BSS Segment       |
+-------------------+
| Heap              |
|      ↑ grows up   |
+-------------------+
|                   |
| Free Space        |
|                   |
+-------------------+
| Stack             |
|    grows down ↓   |
+-------------------+
High Address
```

---

## Memory Segments

### Text Segment

Contains the executable machine code of the program.

**Characteristics**

* Read-only
* Prevents accidental modification of program instructions

---

### Initialized Data Segment

Contains global or static variables that have explicit initial values.

Example:

```c
int x = 5;
```

**Characteristics**

* Stored in the executable file
* Loaded into memory during program startup

---

### Uninitialized Data Segment (BSS)

Contains global or static variables without explicit initialization.

Example:

```c
int counter;
```

**Characteristics**

* Automatically initialized to zero
* Does not occupy space inside the executable file

---

### Heap

Used for dynamic memory allocation.

Functions:

```c
malloc()
calloc()
realloc()
free()
```

**Characteristics**

* Grows toward higher addresses
* Upper boundary is known as the **program break**

---

### Stack

Stores stack frames created during function calls.

Contains:

* Local variables
* Function arguments
* Return addresses
* Saved CPU registers

**Characteristics**

* Grows toward lower addresses
* Automatically managed by the CPU and compiler

---

## Memory Boundary Symbols

The linker provides several symbols representing memory boundaries between segments.

```c
extern char etext, edata, end;
```

| Symbol   | Meaning                         |
| -------- | ------------------------------- |
| `&etext` | End of Text Segment             |
| `&edata` | End of Initialized Data Segment |
| `&end`   | End of BSS / Beginning of Heap  |

---

### ⚠️ Critical Warning

Never modify these symbols.

Incorrect:

```c
etext = 'A';
```

These symbols are not ordinary allocated variables.

Writing to them can cause:

```text
Segmentation Fault
```

or other undefined behavior.

---

# 3. Command-Line Arguments

The command line is the simplest mechanism for passing data into a program.

```c
int main(int argc, char *argv[])
```

---

## `argc` (Argument Count)

Contains the total number of command-line arguments.

Example:

```bash
./my_program hello world
```

Results in:

```c
argc == 3
```

---

## `argv` (Argument Vector)

Array of pointers to null-terminated strings.

Example:

```text
argv[0] = "./my_program"
argv[1] = "hello"
argv[2] = "world"
argv[3] = NULL
```

---

### Special Entries

#### `argv[0]`

Always contains the executable name or path.

Example:

```text
./my_program
```

---

#### `argv[argc]`

Always equals:

```c
NULL
```

This allows iteration without using `argc`.

Example:

```c
for (char **p = argv; *p != NULL; ++p)
{
    printf("%s\n", *p);
}
```

---

# 4. Environment Variables

Child processes inherit a copy of the parent process's environment.

Environment variables are stored as strings in the format:

```text
NAME=value
```

The environment list is maintained through:

```c
extern char **environ;
```

---

## `getenv()`

```c
char *getenv(const char *name);
```

Searches for an environment variable.

Returns:

* Pointer to value if found
* `NULL` otherwise

### Warning

Do not modify the returned string directly.

---

## `putenv()`

```c
int putenv(char *string);
```

Adds or modifies an environment variable.

Example:

```c
putenv("PATH=/usr/bin");
```

---

### ⚠️ Memory Trap

`putenv()` does **not** copy the string.

It stores the pointer directly.

Dangerous example:

```c
void foo()
{
    char buffer[64];
    sprintf(buffer, "PATH=%s", value);

    putenv(buffer);
}
```

After `foo()` returns, `buffer` no longer exists, leaving the environment variable pointing to invalid memory.

---

## `setenv()`

```c
int setenv(
    const char *name,
    const char *value,
    int overwrite
);
```

Safely creates or updates an environment variable.

Unlike `putenv()`, it allocates memory internally and copies the data.

### Recommendation

Prefer `setenv()` over `putenv()` whenever possible.

---

## `unsetenv()`

```c
int unsetenv(const char *name);
```

Removes an environment variable.

---

## `clearenv()`

```c
int clearenv(void);
```

Removes all environment variables.

Effectively:

```c
environ = NULL;
```

### Warning

Repeated use of `setenv()` and `clearenv()` may accumulate unused memory allocations and potentially contribute to memory leaks.

---

# 5. Process Lifecycle: Creation, Monitoring, and Termination

---

## Process Creation (`fork()` and `execve()`)

### `fork()`

Creates a child process by duplicating the calling process.

Linux optimizes this operation using:

### Copy-On-Write (COW)

Initially:

* Parent and child share the same physical memory pages.
* Pages are marked read-only.

Actual duplication occurs only when one process attempts to modify a page.

---

### `execve()`

```c
execve(path, argv, envp);
```

Replaces the current process image with a new program.

The following are reconstructed:

* Text Segment
* Data Segment
* Heap
* Stack

The old program image is completely discarded.

---

## Process Termination

### `_exit()`

```c
_exit(status);
```

Direct kernel system call.

Characteristics:

* Terminates immediately
* Releases kernel resources
* Does not flush user-space buffers

---

### `exit()`

```c
exit(status);
```

C library wrapper around process termination.

Before exiting, it:

1. Executes handlers registered through:

```c
atexit()
on_exit()
```

2. Flushes all stdio buffers.

Example:

```c
printf("Hello");
exit(0);
```

The text is guaranteed to be written before termination.

---

## Stdio Buffer Duplication Trap

Consider:

```c
printf("Hello");
fork();
```

If stdout is block-buffered and the buffer has not been flushed, the buffer is duplicated into the child process.

Both processes later call:

```c
exit();
```

Result:

```text
HelloHello
```

---

### Design Rule

Typically:

* Parent process uses `exit()`
* Child process uses `_exit()`

This avoids:

* Duplicate output
* Duplicate cleanup execution

---

## Zombie Processes

A zombie process is a terminated child whose parent has not yet collected its exit status.

Displayed as:

```text
<defunct>
```

The zombie still occupies:

* PID
* Process table entry

Until the parent performs a wait operation.

---

### Important Property

A zombie process cannot be killed by:

```text
SIGKILL
```

because it is already dead.

---

## `waitpid()`

```c
waitpid(pid, &status, options);
```

Provides flexible child-process monitoring.

Common option:

```c
WNOHANG
```

Allows non-blocking checks.

---

### Status Macros

```c
WIFEXITED(status)
WEXITSTATUS(status)

WIFSIGNALED(status)
WTERMSIG(status)
```

These macros safely interpret the returned status.

---

## Synchronization Using `SIGCHLD`

Whenever a child changes state, the kernel sends:

```text
SIGCHLD
```

to the parent process.

### Important Limitation

Standard UNIX signals are **not queued**.

If multiple children terminate simultaneously:

```text
SIGCHLD
```

may be delivered only once.

---

### Correct Zombie Reaping Pattern

Inside the signal handler:

```c
while (waitpid(-1, NULL, WNOHANG) > 0)
    continue;
```

This guarantees that all zombie children are collected.

---

# 6. Advanced Error Handling: Nonlocal Goto

A **Nonlocal Goto** allows execution to jump directly from a deeply nested function back to a predefined recovery point.

This eliminates the need to propagate errors through multiple return statements.

---

## Checkpoint Creation: `setjmp()`

```c
#include <setjmp.h>

jmp_buf env;

int setjmp(jmp_buf env);
```

Saves the current execution context:

* Program Counter
* Stack Pointer
* CPU Registers

Returns:

```c
0
```

when called directly.

---

## Context Restoration: `longjmp()`

```c
void longjmp(jmp_buf env, int value);
```

Restores the saved context and immediately transfers control back to the corresponding `setjmp()`.

After the jump:

```c
setjmp()
```

returns:

```c
value
```

instead of `0`.

All intermediate stack frames are discarded.

---

## Deadly Pitfall #1: Jumping into a Destroyed Stack Frame

Never jump back into a function that has already returned.

Incorrect:

```c
foo()
{
    setjmp(env);
    return;
}

longjmp(env, 1);
```

The stack frame no longer exists.

Consequences:

* Undefined behavior
* Program crash
* Segmentation fault

---

## Deadly Pitfall #2: Variables "Traveling Back in Time"

Compiler optimizations may store local variables in CPU registers.

Example:

```c
int x = 0;

if (setjmp(env) == 0)
{
    x = 10;
    longjmp(env, 1);
}

printf("%d\n", x);
```

With optimization (`-O2`), the output may unexpectedly be:

```text
0
```

instead of:

```text
10
```

because `longjmp()` restores the old register state saved by `setjmp()`.

---

## Solution: Use `volatile`

```c
volatile int x = 0;
```

The `volatile` keyword forces the compiler to store the variable in memory rather than CPU registers.

Since RAM is not restored by `longjmp()`, the updated value remains valid.

Result:

```text
10
```

is printed consistently and correctly.

# 7. Daemon Processes

A **daemon process** is a background process that runs independently of any controlling terminal. Daemons typically provide system services, monitor resources, handle network requests, or perform scheduled tasks.

Common examples include:

* `sshd` – Secure Shell server
* `cron` – Task scheduler
* `syslogd` / `rsyslogd` – System logging services
* `systemd` – System and service manager

---

## Characteristics of a Daemon

A daemon process typically has the following properties:

* Runs in the background
* Has no controlling terminal
* Usually starts during system boot
* Continues running until explicitly stopped or the system shuts down
* Often operates with elevated privileges

A daemon generally spends most of its time:

```text
Sleeping → Waiting for Event → Processing Request → Sleeping
```

This design minimizes CPU usage while remaining responsive to external events.

---

## Why Detach from the Terminal?

A normal process inherits its controlling terminal from its parent shell.

Example:

```bash
./my_program
```

If the terminal is closed, the process may receive:

```text
SIGHUP (Hangup Signal)
```

and terminate unexpectedly.

A daemon detaches itself from the terminal to remain running independently of user sessions.



---

## Common Pitfalls

### Zombie Children

If a daemon creates child processes, it must properly reap them.

Example:

```c
while (waitpid(-1, NULL, WNOHANG) > 0)
    continue;
```

Otherwise zombie processes may accumulate.

---

### Log Output

Since stdout and stderr are usually disconnected from the terminal, diagnostic messages should be written to:

* Log files
* Syslog
* Journald (`systemd`)

Instead of:

```c
printf("Error!\n");
```

use:

```c
syslog(LOG_ERR, "Error!");
```

for production-quality daemons.

---

## Rule of Thumb

Use a daemon process when:

* A service must run continuously in the background.
* The process must survive user logouts.
* The application listens for requests or events over long periods.

Examples include:

* Web servers
* Database servers
* MQTT brokers
* SSH servers
* Monitoring agents
* Embedded Linux background services
* Industrial IoT gateways
