# Valgrind Labs — Memory Debugging with Memcheck

> **Target Audience:** Developers familiar with basic `malloc`/`free` dynamic memory allocation who are new to Valgrind.  
> **Objective:** Through 4 hands-on labs, learn how to use Valgrind Memcheck to pinpoint and fix common C memory bugs.

---

## What is Valgrind?

**Valgrind** (specifically its default tool, **Memcheck**) executes your binary inside a synthetic CPU environment, tracking every memory read, write, allocation, and deallocation. It pinpoints errors with exact source file names and line numbers. The program runs normally, albeit with a 10x–50x performance slowdown.

---

## Environment Setup

### Installation

```bash
# Ubuntu / Debian
sudo apt-get install valgrind

# Verify installation
valgrind --version
```

### Compilation — Always Include `-g`

```bash
gcc -g -o my_program my_program.c
#    │
#    └── Preserves DWARF debug symbols — allows Valgrind to display function names and line numbers
#        Without -g: Valgrind displays only raw hexadecimal addresses, making debugging difficult
```

### Execution Commands

```bash
# Basic run
valgrind ./my_program

# Comprehensive run — recommended for all labs
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./my_program
```

### Key Command-Line Flags

| Flag | Description |
| :--- | :--- |
| `--leak-check=full` | Prints detailed stack traces for each individual leaked memory block. |
| `--show-leak-kinds=all` | Shows all leak classifications (`definitely lost`, `indirectly lost`, `possibly lost`, `still reachable`). |
| `--track-origins=yes` | Tracks the origin of uninitialized memory values. |
| `--error-exitcode=1` | Returns an exit status of 1 if any memory errors are detected (ideal for CI/CD pipelines). |

### Reading Valgrind Output

Every output line begins with `==PID==` (the process ID), for example `==1036==`:

```text
==1036== Invalid write of size 4
            ↑ Error type and byte count (4 bytes = 1 int)

==1036==    at 0x1091E1: main (02_invalid_rw.c:12)
              ↑ Address  ↑ Function  ↑ file:line   ← The line causing the fault

==1036==  Address 0x4a7c054 is 0 bytes after a block of size 20 alloc'd
            ↑ Offending address    ↑ Offset relative to the valid allocated block
```

---

## Lab 1 — Heap Memory Leak

### Learning Objectives
- Identify memory leaks via the `definitely lost` keyword in the `LEAK SUMMARY`.
- Inspect call stacks to find which `malloc()` call lacks a matching `free()`.
- Understand the difference between `definitely lost` and `still reachable`.

### Problem Scenario
`malloc()` allocates heap memory, but the program never invokes `free()`. Memory consumption grows over time—a critical defect in long-running processes (daemons, network servers, embedded services).

### Buggy Code — `buggy/01_heap_leak.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void create_string(void) {
    char *str = malloc(50 * sizeof(char));  // Allocate 50 bytes
    if (!str) return;
    strcpy(str, "Hello, Valgrind!");
    printf("String: %s\n", str);
    // BUG: Missing free(str) — memory leaks!
}

int main(void) {
    create_string();
    create_string();  // Leaked twice
    printf("Program terminated.\n");
    return 0;
}
```

### Debugging Strategy
1. **Confirm the Leak**: Inspect the `LEAK SUMMARY` at the end of the Valgrind report.
2. **Locate the Origin**: Examine the stack trace under `definitely lost` pointing directly to line 6 in `create_string()`.

### Compilation and Execution

```bash
gcc -g -o 01_heap_leak 01_heap_leak.c
./01_heap_leak
# Output looks completely normal to the naked eye
```

### Running Valgrind

```bash
valgrind --leak-check=full --show-leak-kinds=all ./01_heap_leak
```

**Valgrind Report:**
```text
==1036== Memcheck, a memory error detector
==1036== Command: ./01_heap_leak
==1036==
String: Hello, Valgrind!
String: Hello, Valgrind!
Program terminated.
==1036==
==1036== HEAP SUMMARY:
==1036==     in use at exit: 100 bytes in 2 blocks
              ↑ At exit, 100 bytes remained on heap without being freed
==1036==   total heap usage: 3 allocs, 1 frees, 4,196 bytes allocated
              ↑ 3 allocations vs 1 free → 2 un-freed blocks
==1036==
==1036== 50 bytes in 1 blocks are definitely lost in loss record 1 of 2
              ↑ "definitely lost" = genuine leak, no pointer references remain
==1036==    at 0x4846828: malloc (...)
==1036==    by 0x10919E: create_string (01_heap_leak.c:6)
                         ↑ malloc() was called at line 6
==1036==    by 0x1091F5: main (01_heap_leak.c:14)
                         ↑ Called from main() line 14
==1036==
==1036== 50 bytes in 1 blocks are definitely lost in loss record 2 of 2
==1036==    at 0x4846828: malloc (...)
==1036==    by 0x10919E: create_string (01_heap_leak.c:6)
==1036==    by 0x1091FF: main (01_heap_leak.c:15)
==1036==
==1036== LEAK SUMMARY:
==1036==    definitely lost: 100 bytes in 2 blocks
==1036==
==1036== ERROR SUMMARY: 2 errors from 2 contexts
```

### Understanding Leak Classifications

| Classification | Meaning |
| :--- | :--- |
| `definitely lost` | Pointers to the allocated block no longer exist. Cannot be freed anymore. Must fix. |
| `indirectly lost` | Leaked structure blocks referenced only by other leaked structures (e.g., nodes in a leaked linked list). |
| `possibly lost` | An interior pointer still points into the block, but no pointer addresses the start. |
| `still reachable` | Valid pointers to the block still exist at exit, but `free()` was never called (common for global variables). |

### The Fix — `fixed/01_heap_leak.c`

```c
void create_string(void) {
    char *str = malloc(50 * sizeof(char));
    if (!str) return;
    strcpy(str, "Hello, Valgrind!");
    printf("String: %s\n", str);
    free(str);  // FIX: Free memory when finished
}
```

**Verification:**
```bash
gcc -g -o 01_heap_leak_fixed 01_heap_leak_fixed.c
valgrind --leak-check=full ./01_heap_leak_fixed
# Output: All heap blocks were freed -- no leaks are possible
# Output: ERROR SUMMARY: 0 errors from 0 contexts
```

---

## Lab 2 — Invalid Read / Invalid Write

### Learning Objectives
- Identify buffer overruns and out-of-bounds accesses via `Invalid read/write of size N`.
- Understand why programs may appear to "work" despite corrupting adjacent heap memory.
- Decode `Address ... is N bytes after a block` to pinpoint the exact overflow distance.

### Problem Scenario
An array `arr` has 5 integer elements (valid indices 0 to 4). Code writes to and reads from `arr[5]`. The program may not crash immediately, but it silently corrupts heap memory belonging to other objects.

### Buggy Code — `buggy/02_invalid_rw.c`

```c
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *arr = malloc(5 * sizeof(int));  // 5 elements (indices 0..4)
    if (!arr) return 1;

    for (int i = 0; i < 5; i++) {
        arr[i] = i * 10;
    }

    // BUG: Out-of-bounds WRITE — index 5 is outside the allocated block!
    arr[5] = 999;

    // BUG: Out-of-bounds READ
    printf("arr[5] = %d\n", arr[5]);

    free(arr);
    return 0;
}
```

### Analyzing with Valgrind

```bash
gcc -g -o 02_invalid_rw 02_invalid_rw.c
valgrind --leak-check=full ./02_invalid_rw
```

**Valgrind Report:**
```text
==1042== Invalid write of size 4
            ↑ Wrote 4 bytes (1 int) to an illegal memory address
==1042==    at 0x1091E1: main (02_invalid_rw.c:13)
                         ↑ Line 13: arr[5] = 999
==1042==  Address 0x4a7c054 is 0 bytes after a block of size 20 alloc'd
            ↑ "0 bytes after" = immediately following the valid boundary
            ↑ "size 20" = 5 * sizeof(int) = 20 bytes
==1042==    at 0x4846828: malloc (...)
==1042==    by 0x10919E: main (02_invalid_rw.c:5)
==1042==
==1042== Invalid read of size 4
==1042==    at 0x1091EF: main (02_invalid_rw.c:16)
                         ↑ Line 16: printf("arr[5] = %d\n", arr[5]);
==1042==  Address 0x4a7c054 is 0 bytes after a block of size 20 alloc'd
==1042==
arr[5] = 999
==1042== ERROR SUMMARY: 2 errors from 2 contexts
```

### The Fix — `fixed/02_invalid_rw.c`
Constrain iterations and indices strictly within `0 .. 4`.

```c
for (int i = 0; i < 5; i++) {  // FIX: Access only valid indices 0 to 4
    arr[i] = i * 10;
    printf("arr[%d] = %d\n", i, arr[i]);
}
```

**Verification:**
```bash
gcc -g -o 02_invalid_rw_fixed 02_invalid_rw_fixed.c
valgrind --leak-check=full ./02_invalid_rw_fixed
# Output: ERROR SUMMARY: 0 errors from 0 contexts
```

---

## Lab 3 — Use After Free

### Learning Objectives
- Identify use-after-free conditions via `inside a block ... free'd`.
- Understand dangling pointers and why accessing freed memory yields undefined behavior.
- Learn the defensive pattern of setting pointers to `NULL` immediately after `free()`.

### Problem Scenario
After calling `free(p)`, the block is returned to the heap allocator. The pointer variable `p` still holds the old address (a **dangling pointer**). Reading or writing through `p` corrupts the allocator's free list metadata.

### Buggy Code — `buggy/03_use_after_free.c`

```c
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *p = malloc(sizeof(int));
    if (!p) return 1;

    *p = 42;
    printf("Before free: *p = %d\n", *p);

    free(p);  // Memory deallocated

    // BUG: Accessing memory that has already been freed!
    printf("After free: *p = %d\n", *p);  // Read through dangling pointer
    *p = 100;                             // Write to freed block

    return 0;
}
```

### Analyzing with Valgrind

```bash
gcc -g -o 03_use_after_free 03_use_after_free.c
valgrind --leak-check=full ./03_use_after_free
```

**Valgrind Report:**
```text
==1045== Invalid read of size 4
==1045==    at 0x1091DF: main (03_use_after_free.c:13)
                         ↑ Line 13: printf("After free: *p = %d\n", *p);
==1045==  Address 0x4a7c040 is 0 bytes inside a block of size 4 free'd
            ↑ Indicates USE-AFTER-FREE! Block was already freed!
==1045==    at 0x484988F: free (...)
==1045==    by 0x1091D4: main (03_use_after_free.c:10)
                         ↑ Line 10: Where free() occurred
==1045==  Block was alloc'd at
==1045==    at 0x4846828: malloc (...)
==1045==    by 0x10919E: main (03_use_after_free.c:5)
==1045==
==1045== Invalid write of size 4
==1045==    at 0x1091FB: main (03_use_after_free.c:14)
                         ↑ Line 14: *p = 100;
==1045==  Address 0x4a7c040 is 0 bytes inside a block of size 4 free'd
==1045==
==1045== ERROR SUMMARY: 2 errors from 2 contexts
```

### The Fix — `fixed/03_use_after_free.c`

```c
free(p);
p = NULL;  // FIX: Reset pointer to NULL immediately to prevent dangling pointer access

if (p != NULL) {
    printf("*p = %d\n", *p);
} else {
    printf("Pointer has been freed, access skipped.\n");
}
```

---

## Lab 4 — Double Free

### Learning Objectives
- Detect double-free errors via `Invalid free() / delete / delete[] / realloc()`.
- Understand the danger of double frees: corrupting heap metadata structures and introducing critical security exploits.
- Reiterate that `free(NULL)` is well-defined and completely safe in standard C.

### Problem Scenario
`free()` is mistakenly executed twice on the same pointer. The second deallocation corrupts the allocator's free-chunk bins.

### Buggy Code — `buggy/04_double_free.c`

```c
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *p = malloc(sizeof(int));
    if (!p) return 1;

    *p = 99;
    printf("Value: %d\n", *p);

    free(p);  // First free — OK
    printf("First free executed.\n");

    free(p);  // BUG: Second free on identical address — CRASH / Heap corruption!
    printf("Second free executed.\n");

    return 0;
}
```

### Analyzing with Valgrind

```bash
gcc -g -o 04_double_free 04_double_free.c
valgrind --leak-check=full ./04_double_free
```

**Valgrind Report:**
```text
==1051== Invalid free() / delete / delete[] / realloc()
==1051==    at 0x484988F: free (...)
==1051==    by 0x10920F: main (04_double_free.c:13)
                         ↑ Invalid second free() on line 13
==1051==  Address 0x4a7c040 is 0 bytes inside a block of size 4 free'd
==1051==    at 0x484988F: free (...)
==1051==    by 0x1091F4: main (04_double_free.c:10)
                         ↑ First valid free() on line 10
==1051==  Block was alloc'd at
==1051==    at 0x4846828: malloc (...)
==1051==    by 0x1091BE: main (04_double_free.c:5)
==1051==
==1051== ERROR SUMMARY: 1 errors from 1 contexts
```

### The Fix — `fixed/04_double_free.c`

```c
free(p);
p = NULL;  // FIX: Reset pointer to NULL

free(p);   // Safe: In ISO C, calling free(NULL) is a guaranteed no-op
printf("free(NULL) executed safely.\n");
```

---

## Summary & Quick Reference

| Error Type | Valgrind Signature | Root Cause | Remediation |
| :--- | :--- | :--- | :--- |
| **Heap Leak** | `definitely lost` | `malloc()` without matching `free()` | Add `free()` along all exit code paths |
| **Invalid R/W** | `Invalid read/write of size N` | Out-of-bounds array access | Validate buffer boundaries and array indices |
| **Use After Free**| `inside a block ... free'd` | Accessing dangling pointer | Assign `p = NULL` immediately following `free()` |
| **Double Free** | `Invalid free() / delete` | Freeing the same pointer twice | Assign `p = NULL` after first `free()` |
| **Uninitialized** | `Conditional jump ... uninitialised`| Branching on uninitialized variable | Zero-initialize variables or use `calloc()` |

### Recommended Valgrind Workflow
```bash
# 1. Compile with debug symbols
gcc -g -o my_program my_program.c

# 2. Run with comprehensive diagnostics
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./my_program

# 3. Analyze output top-down:
#    a. Fix runtime memory violations (Invalid read/write, Use-after-free) first.
#    b. Fix leaks listed in LEAK SUMMARY (definitely lost).
#    c. Verify ERROR SUMMARY reaches 0 errors.
```
