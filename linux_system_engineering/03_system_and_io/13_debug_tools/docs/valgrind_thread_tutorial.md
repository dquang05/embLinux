# Valgrind Labs — Thread Concurrency & Synchronization (Helgrind)

> **Target Audience:** Developers familiar with basic `pthread` programming who are new to Helgrind.  
> **Objective:** Through 2 hands-on labs, master using Helgrind to detect data races and deadlocks in multithreaded C programs.

---

## What is Helgrind?

**Helgrind** is a specialized Valgrind tool designed to detect synchronization errors in POSIX pthreads programs. It monitors all mutex lock/unlock operations and memory accesses to shared variables, reporting issues with exact function names and line numbers.

Helgrind detects:

| Error Category | How Helgrind Detects It |
| :--- | :--- |
| **Data Race** | Two or more threads concurrently access the same memory location, at least one access is a write, without holding a common mutex lock. |
| **Deadlock** | Circular wait condition detected via Lock Order Inversion graph analysis. |

---

## Environment Setup

### Installation

```bash
# Ubuntu / Debian (Helgrind is packaged inside valgrind)
sudo apt-get install valgrind

valgrind --version
```

### Compilation Flags — Mandatory `-g` and `-pthread`

```bash
gcc -g -pthread -o my_program my_program.c
#    │   │
#    │   └── Links the POSIX threads library
#    └────── Preserves DWARF debug symbols — allows Helgrind to show file names and line numbers
```

### Execution Command

```bash
# Run Helgrind to detect data races and deadlocks
valgrind --tool=helgrind ./my_program
```

### Reading Helgrind Output

```text
==619== Possible data race during read of size 4 at 0x10C014 by thread #3
                                  ↑ Error type: 4-byte read (e.g., int) raced by thread #3

==619== Locks held: none
         ↑ Thread #3 held NO locks while accessing this variable!

==619==    at 0x1091BE: increment (01_data_race.c:8)
            ↑ Line 8 inside increment() — location of the conflicting access

==619== This conflicts with a previous write of size 4 by thread #2
                                        ↑ Conflicted with an earlier WRITE by thread #2

==619==  Address 0x10c014 is 0 bytes inside data symbol "counter"
                                          ↑ Pinpoints the exact global variable "counter"
```

---

## Lab 1 — Data Race

### Learning Objectives
- Identify data race signatures via `Possible data race` and `Locks held: none`.
- Understand why `counter++` is not atomic (read-modify-write cycle).
- Accurately determine the contested variable via `data symbol "..."` in the report.

### Problem Scenario
Two concurrent threads increment a shared integer variable `counter` 100,000 times each. Expected total: 200,000.
However, `counter++` compiles into three separate machine instructions: **read**, **add**, and **write**. Context switches between these steps cause lost updates.

### Buggy Code — `buggy/01_data_race.c`

```c
#include <stdio.h>
#include <pthread.h>

int counter = 0;  // Shared global state — UNPROTECTED!

void *increment(void *arg) {
    (void)arg;
    for (int i = 0; i < 100000; i++) {
        counter++;  // BUG: Concurrent non-atomic read-modify-write!
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, increment, NULL);
    pthread_create(&t2, NULL, increment, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("Result: %d (Expected: 200000)\n", counter);
    return 0;
}
```

### Debugging Strategy
1. **Confirm the Data Race**: Run under Helgrind, inspect `Possible data race` and `Locks held: none`.
2. **Identify the Contested Symbol**: Look for `Address ... is inside data symbol "counter"`.
3. **Analyze Conflicting Threads**: Check `by thread #3` conflicting with `write ... by thread #2`.

> **Note on Data Races:** Helgrind uses the **Happens-Before** algorithm. It does not need the race to actively corrupt data during the run; it detects the *potential* for a race based on lock histories even if the program accidentally printed 200000.

### Compilation and Execution

```bash
gcc -g -pthread -o 01_data_race 01_data_race.c
./01_data_race
# Output may intermittently print 200000 or a corrupted smaller value
```

### Analyzing with Helgrind

```bash
valgrind --tool=helgrind ./01_data_race
```

**Helgrind Report:**
```text
==619== Helgrind, a thread error detector
==619== Command: ./01_data_race
==619==
==619== ---Thread-Announcement------------------------------------------
==619== Thread #3 was created
==619==    by 0x109235: main (01_data_race.c:16)
==619== Thread #2 was created
==619==    by 0x109218: main (01_data_race.c:15)
==619==
==619== Possible data race during read of size 4 at 0x10C014 by thread #3
==619== Locks held: none
==619==    at 0x1091BE: increment (01_data_race.c:8)
==619==
==619== This conflicts with a previous write of size 4 by thread #2
==619== Locks held: none
==619==    at 0x1091C7: increment (01_data_race.c:8)
==619==  Address 0x10c014 is 0 bytes inside data symbol "counter"
==619==
==619== ERROR SUMMARY: 2 errors from 2 contexts
```

### The Fix — `fixed/01_data_race.c`
Protect critical sections with a mutual exclusion lock (`pthread_mutex_t`).

```c
#include <stdio.h>
#include <pthread.h>

int counter = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;  // FIX: Mutex lock

void *increment(void *arg) {
    (void)arg;
    for (int i = 0; i < 100000; i++) {
        pthread_mutex_lock(&lock);    // FIX: Acquire lock before access
        counter++;
        pthread_mutex_unlock(&lock);  // FIX: Release lock immediately
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, increment, NULL);
    pthread_create(&t2, NULL, increment, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("Result: %d (Expected: 200000)\n", counter);
    pthread_mutex_destroy(&lock);
    return 0;
}
```

**Verification:**
```bash
gcc -g -pthread -o 01_data_race_fixed 01_data_race_fixed.c
valgrind --tool=helgrind ./01_data_race_fixed
# Output: ERROR SUMMARY: 0 errors from 0 contexts
```

---

## Lab 2 — Deadlock (Lock Order Inversion)

### Learning Objectives
- Identify deadlocks: process hangs indefinitely without crashing.
- Interpret `Exiting thread still holds N lock` in Helgrind crash/termination logs.
- Enforce the golden rule of concurrency: **Lock acquisition must follow a globally consistent hierarchy**.

### Problem Scenario
- Thread 1 acquires `lockA` $\rightarrow$ attempts to acquire `lockB`.
- Thread 2 acquires `lockB` $\rightarrow$ attempts to acquire `lockA`.
- If both threads execute concurrently, Thread 1 holds A and waits for B, while Thread 2 holds B and waits for A (Circular Wait Deadlock).

### Buggy Code — `buggy/02_deadlock.c`

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t lockA = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockB = PTHREAD_MUTEX_INITIALIZER;

void *thread1_fn(void *arg) {
    (void)arg;
    pthread_mutex_lock(&lockA);   // Thread 1: holds lockA
    printf("Thread 1: acquired lockA, waiting for lockB...\n");
    sleep(1);
    pthread_mutex_lock(&lockB);   // Blocks: Thread 2 holds lockB!
    printf("Thread 1: acquired both locks\n");
    pthread_mutex_unlock(&lockB);
    pthread_mutex_unlock(&lockA);
    return NULL;
}

void *thread2_fn(void *arg) {
    (void)arg;
    pthread_mutex_lock(&lockB);   // Thread 2: holds lockB
    printf("Thread 2: acquired lockB, waiting for lockA...\n");
    sleep(1);
    pthread_mutex_lock(&lockA);   // Blocks: Thread 1 holds lockA!
    printf("Thread 2: acquired both locks\n");
    pthread_mutex_unlock(&lockA);
    pthread_mutex_unlock(&lockB);
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread1_fn, NULL);
    pthread_create(&t2, NULL, thread2_fn, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("Completed!\n");
    return 0;
}
```

### Analyzing with Helgrind
When running this program, it deadlocks. Sending `SIGTERM` / `Ctrl+C` generates the diagnostic report:

```bash
valgrind --tool=helgrind ./02_deadlock
# Wait 3-5 seconds, then terminate with Ctrl+C
```

**Helgrind Report:**
```text
==651== Process terminating with default action of signal 15 (SIGTERM)
==651== Thread #2: Exiting thread still holds 1 lock
==651==    at 0x4910100: pthread_mutex_lock (pthread_mutex_lock.c:93)
==651==    by 0x10924F: thread1_fn (02_deadlock.c:12)
==651==
==651== Thread #3: Exiting thread still holds 1 lock
==651==    at 0x4910100: pthread_mutex_lock (pthread_mutex_lock.c:93)
==651==    by 0x1092CA: thread2_fn (02_deadlock.c:23)
==651==
==651== ERROR SUMMARY: 2 errors from 2 contexts
```

### Trace Analysis
```
Thread #2 (thread1_fn):
  Holds:   lockA (line 10)
  Waiting: lockB (blocked at line 12)
            ↓
Thread #3 (thread2_fn):
  Holds:   lockB (line 20)
  Waiting: lockA (blocked at line 23)
            ↓
  → Thread #2 waits for Thread #3 to release lockB
  → Thread #3 waits for Thread #2 to release lockA
  → CIRCULAR DEPENDENCY: Neither thread can proceed!
```

### The Fix — `fixed/02_deadlock.c`
Enforce identical lock ordering (`lockA` $\rightarrow$ `lockB`) across all worker threads.

```c
void *thread1_fn(void *arg) {
    (void)arg;
    pthread_mutex_lock(&lockA);   // Order: A then B
    printf("Thread 1: acquired lockA\n");
    sleep(1);
    pthread_mutex_lock(&lockB);
    printf("Thread 1: acquired both locks\n");
    pthread_mutex_unlock(&lockB);
    pthread_mutex_unlock(&lockA);
    return NULL;
}

void *thread2_fn(void *arg) {
    (void)arg;
    pthread_mutex_lock(&lockA);   // Order: A then B (CONSISTENT!)
    printf("Thread 2: acquired lockA\n");
    sleep(1);
    pthread_mutex_lock(&lockB);
    printf("Thread 2: acquired both locks\n");
    pthread_mutex_unlock(&lockB);
    pthread_mutex_unlock(&lockA);
    return NULL;
}
```

**Verification:**
```bash
gcc -g -pthread -o 02_deadlock_fixed 02_deadlock_fixed.c
valgrind --tool=helgrind ./02_deadlock_fixed
# Output: ERROR SUMMARY: 0 errors from 0 contexts
```

---

## Summary & Quick Reference

| Error Type | Symptoms | Helgrind Indication | Root Cause | Remediation |
| :--- | :--- | :--- | :--- | :--- |
| **Data Race** | Non-deterministic results, corrupted state | `Possible data race` + `Locks held: none` | Unsynchronized concurrent memory access | Guard all accesses with a shared `pthread_mutex_t` |
| **Deadlock** | Application freezes indefinitely | `Exiting thread still holds N lock` | Circular lock dependency across threads | Enforce identical lock acquisition ordering across all threads |

### Golden Rules for Embedded Multithreading
1. **Single Mutex Ownership**: Every shared mutable variable must be protected by exactly one designated mutex.
2. **Global Lock Hierarchy**: Establish and document a strict acquisition order (e.g., Level 1 $\rightarrow$ Level 2) to eliminate deadlocks.
3. **Minimize Critical Section Scope**: Unlock as soon as data processing is complete; avoid calling blocking functions (`sleep()`, disk I/O, network calls) while holding locks.
4. **CI Integration**: Run Helgrind as part of automated unit and integration tests to catch synchronization defects before deployment.
