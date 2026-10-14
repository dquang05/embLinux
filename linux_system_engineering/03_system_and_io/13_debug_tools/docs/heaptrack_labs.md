# Heaptrack Labs — Heap Memory Profiling & Analysis

> **Target Audience:** Developers familiar with basic `malloc`/`free` dynamic memory allocation who are new to Heaptrack.  
> **Objective:** Through 2 hands-on labs, master using Heaptrack (both CLI and GUI) to diagnose memory leaks and eliminate dangerous peak memory consumption spikes.

---

## What is Heaptrack?

**Heaptrack** is a fast heap memory profiler for Linux applications. It intercepts every `malloc`, `realloc`, and `free` invocation, producing detailed reports that pinpoint:

| Problem Domain | How Heaptrack Detects It |
| :--- | :--- |
| **Memory Leaks** | Tracks allocations that are never reclaimed by a matching `free()`. |
| **Peak Memory Consumption** | Identifies the exact instant and call stack responsible for the highest RAM usage spike. |
| **Allocation Churn / Hotspots** | Uncovers functions that excessively allocate and deallocate temporary buffers, wasting CPU cycles. |

### Quick Comparison: Valgrind Memcheck vs. Heaptrack

| Metric / Capability | Valgrind Memcheck | Heaptrack |
| :--- | :--- | :--- |
| **Leak Detection** | Comprehensive (exact bytes and addresses) | Comprehensive (call stacks and metrics) |
| **Runtime Slowdown** | **10x – 50x** (heavy binary translation) | **~1.2x – 3x** (lightweight `LD_PRELOAD`) |
| **RAM Overhead** | Massive (often runs out of memory on SBCs) | Negligible (suitable for embedded targets) |
| **Memory Over Time Timeline** | ❌ (only snapshots at exit) | ✅ (detailed time-series chart) |
| **Graphical Interface (GUI)** | ❌ (CLI text only) | ✅ (`heaptrack_gui` with interactive charts) |

---

## Environment Setup

### Installation

```bash
# Ubuntu / Debian
sudo apt-get install heaptrack heaptrack-gui

# Verify installation
heaptrack --version
```

### Compilation — Always Include `-g -O0`

```bash
gcc -g -O0 -o my_program my_program.c
#    │   │
#    │   └── Disables compiler optimizations — ensures call stacks match actual source lines
#    └────── Preserves DWARF debug symbols — allows Heaptrack to resolve function names and line numbers
```

> [!TIP]
> **Use Absolute Paths:** Always supply the absolute binary path when invoking Heaptrack so the GUI can automatically locate source files for inline viewing:
> ```bash
> heaptrack $(pwd)/my_program
> ```

---

## Lab 1 — Memory Leak (Cumulative Leak in Error Paths)

### Learning Objectives
- Interpret the `leaked` metric and call tree in `heaptrack_print`.
- Read call stacks to pinpoint the exact line calling `malloc()` without a matching `free()`.
- Identify a classic real-world bug: **failing to free buffers before returning early from error-handling branches**.

### Problem Scenario
Function `process()` executes 20 rounds of data processing. Each round allocates a 256-byte scratch buffer. When an simulated error occurs (`round % 5 == 0`), the function returns early and **forgets to call `free(buf)`**, leaking 256 bytes per error event.

### Buggy Code — `bai1_memleak.c`

```c
#include <stdio.h>
#include <stdlib.h>

#define NUM_ROUNDS 20

void process(int round) {
    char *buf = malloc(256);  // Allocate scratch buffer
    if (!buf) return;

    if (round % 5 == 0) {
        printf("  round %2d: error -> skipping\n", round);
        // BUG: Early return without calling free(buf)!
        return;
    }

    printf("  round %2d: completed successfully\n", round);
    free(buf);
}

int main(void) {
    printf("=== Start ===\n");
    for (int r = 1; r <= NUM_ROUNDS; r++) {
        process(r);
    }
    printf("=== Completed ===\n");
    return 0;
}
```

### Debugging Strategy
1. **Detect the Leak**: Inspect the `PEAK MEMORY CONSUMERS` and `MOST TEMPORARY ALLOCATIONS` sections in the CLI report.
2. **Calculate Leaked Allocations**: If `total allocations = 20` but `temporary allocations = 16`, exactly $20 - 16 = 4$ allocations were leaked.
3. **Trace the Bug in Code**: Inspect the early `return` condition preceding `free()`.

### Compilation and Execution

```bash
gcc -g -O0 -o bai1_memleak bai1_memleak.c
./bai1_memleak
```

**Terminal Output:**
```text
=== Start ===
  round  1: completed successfully
  round  2: completed successfully
  round  3: completed successfully
  round  4: completed successfully
  round  5: error -> skipping        <- Leaked!
  round  6: completed successfully
  ...
  round 20: error -> skipping        <- Leaked!
=== Completed ===
```

### Analyzing with Heaptrack CLI (`heaptrack_print`)

```bash
heaptrack $(pwd)/bai1_memleak
heaptrack_print heaptrack.bai1_memleak.*.zst
```

**Heaptrack CLI Report (Excerpts):**
```text
MOST CALLS TO ALLOCATION FUNCTIONS
20 calls to allocation functions with 1.02K peak consumption from
process
  at /path/to/bai1_memleak.c:7
  in /path/to/bai1_memleak
20 calls with 1.02K peak consumption from:
    main
      at /path/to/bai1_memleak.c:21

PEAK MEMORY CONSUMERS
1.02K peak memory consumed over 4 calls from    <- 4 allocations remained live!
process
  at /path/to/bai1_memleak.c:7

MOST TEMPORARY ALLOCATIONS
16 temporary allocations of 20 allocations in total (80.00%) from   <- 16/20 freed
process
  at /path/to/bai1_memleak.c:7
```

### Decoding the Report

| Section | Output Value | Interpretation |
| :--- | :--- | :--- |
| `MOST CALLS` | `20 calls at bai1_memleak.c:7` | Line 7 (`malloc(256)`) was called 20 times. |
| `MOST TEMPORARY` | `16 temporary allocations of 20 (80.00%)` | 16 calls were freed. **Leaked instances = 20 - 16 = 4**. |
| `PEAK CONSUMERS`| `1.02K peak memory over 4 calls` | At peak / exit, 4 blocks ($4 \times 256\text{ bytes} \approx 1\text{ KB}$) were never freed. |

### Analyzing with Heaptrack GUI

```bash
heaptrack_gui heaptrack.bai1_memleak.*.zst
```
- **"Summary" Tab**: Total leaked: 1.00 KB across 4 leaked allocations.
- **"Memory Usage over Time" Tab**: Healthy rounds show allocation rising and immediately dropping back to zero. Error rounds (5, 10, 15, 20) produce non-receding stair steps on the graph.
- **"Allocations" Tab $\rightarrow$ "Leaked"**: Points directly to `process` with 1.00 KB un-freed.

### The Fix — `bai1_memleak_fixed.c`

```c
if (round % 5 == 0) {
    printf("  round %2d: error -> skipping\n", round);
    free(buf);  // FIX: Free buffer before returning
    return;
}
```

**Verification:**
```bash
gcc -g -O0 -o bai1_memleak_fixed bai1_memleak_fixed.c
heaptrack $(pwd)/bai1_memleak_fixed
heaptrack_print heaptrack.bai1_memleak_fixed.*.zst | head -n 40
```
```text
MOST TEMPORARY ALLOCATIONS
20 temporary allocations of 20 allocations in total (100.00%) from
process
  at /path/to/bai1_memleak_fixed.c:7
# Result: 20/20 allocations freed (100.00%) -- ZERO LEAKS!
```

---

## Lab 2 — Peak Memory Spikes (Zero-Leak OOM Hazards)

### Learning Objectives
- Understand why **zero memory leaks does not mean your application is safe from OOM crashes**.
- Inspect the `PEAK MEMORY CONSUMERS` section to discover temporary allocation spikes.
- Use the "Memory Usage over Time" timeline to visualize batch vs. stream processing.
- Refactor batch processing into streaming pipelines to drastically reduce RAM requirements.

### Problem Scenario: The Out-Of-Memory (OOM) Killer Threat
On memory-constrained Embedded Linux devices (e.g., 64MB–256MB RAM) or Docker containers with strict memory limits, memory exhaustion triggers the kernel's **OOM Killer**.

```
System Available RAM drops below threshold
                   ↓
Linux Kernel triggers OOM Killer (oom-killer.c)
                   ↓
Selects victim process with highest badness score (highest RAM consumption)
                   ↓
Sends uncatchable SIGKILL (Process terminated immediately with exit code 137)
```

### Buggy Code — Batch Allocation — `bai2_peakmem.c`
This application processes 10 data chunks of 1 MB each. It allocates all 10 chunks into RAM simultaneously, processes them, and then frees them.

```c
#include <stdio.h>
#include <stdlib.h>

#define CHUNK_SIZE  (1024 * 1024)   // 1 MB per chunk
#define NUM_CHUNKS  10

void process_chunk(char *data, int idx) {
    printf("  processing chunk %d: byte[0] = %d\n", idx, (int)data[0]);
}

// BUG: Allocates all chunks concurrently -> Peak Memory = 10 MB!
void load_all_then_process(void) {
    char *chunks[NUM_CHUNKS];

    // Allocate all buffers up front
    for (int i = 0; i < NUM_CHUNKS; i++) {
        chunks[i] = malloc(CHUNK_SIZE);
        chunks[i][0] = (char)(i + 1);
    }

    // Process all chunks
    for (int i = 0; i < NUM_CHUNKS; i++) {
        process_chunk(chunks[i], i);
    }

    // Free all buffers at the very end — ZERO leaks, but peak reached 10 MB!
    for (int i = 0; i < NUM_CHUNKS; i++) {
        free(chunks[i]);
    }
}

int main(void) {
    printf("=== Start ===\n");
    load_all_then_process();
    printf("=== Completed ===\n");
    return 0;
}
```

### Debugging with Heaptrack
Valgrind reports 0 leaks because all memory is eventually freed. However, Heaptrack reveals the dangerous peak memory spike:

```bash
gcc -g -O0 -o bai2_peakmem bai2_peakmem.c
heaptrack $(pwd)/bai2_peakmem
heaptrack_print heaptrack.bai2_peakmem.*.zst | head -n 40
```

**Heaptrack Report:**
```text
PEAK MEMORY CONSUMERS
10.00M peak memory consumed over 10 calls from   <- 10 MB live simultaneously!
load_all_then_process
  at /path/to/bai2_peakmem.c:16

MOST TEMPORARY ALLOCATIONS
10 temporary allocations of 10 allocations in total (100.00%) from
load_all_then_process
  at /path/to/bai2_peakmem.c:16
```

### The Fix — Streaming / Chunk-by-Chunk Processing

```c
// FIX: Allocate -> Process -> Free each chunk sequentially
void process_one_by_one(void) {
    for (int i = 0; i < NUM_CHUNKS; i++) {
        char *chunk = malloc(CHUNK_SIZE);
        if (!chunk) return;
        chunk[0] = (char)(i + 1);

        process_chunk(chunk, i);

        free(chunk);  // FIX: Free immediately after processing!
    }
}
```

### Comparing Peak Memory Results

```bash
gcc -g -O0 -o bai2_peakmem_fixed bai2_peakmem_fixed.c
heaptrack $(pwd)/bai2_peakmem_fixed
heaptrack_print heaptrack.bai2_peakmem_fixed.*.zst | head -n 40
```

```text
PEAK MEMORY CONSUMERS
1.00M peak memory consumed over 1 calls from    <- Peak reduced from 10 MB to 1 MB!
process_one_by_one
  at /path/to/bai2_peakmem_fixed.c:16
```

| Metric | Buggy Batch Version | Fixed Streaming Version | Impact |
| :--- | :--- | :--- | :--- |
| **Total Allocations** | 10 | 10 | Unchanged |
| **Leaked Bytes** | 0 B | 0 B | Clean in both |
| **Peak Heap RAM** | **10.00 MB** ❌ | **1.00 MB** ✅ | **10x RAM reduction!** |

---

## In-Depth Guide: Reading `heaptrack_print` Reports

When debugging headless embedded devices or SSH remote sessions where X11/GUI is unavailable, `heaptrack_print` provides complete textual diagnostics:

### 1. `MOST CALLS TO ALLOCATION FUNCTIONS`
Shows which code paths trigger `malloc()` most frequently:
```text
MOST CALLS TO ALLOCATION FUNCTIONS
1000 calls to allocation functions with 40.00K peak consumption from
record_telemetry_event
  at /src/telemetry.c:45
```
*Use this section to detect allocation churn and excessive temporary allocations.*

### 2. `PEAK MEMORY CONSUMERS`
Sorted in descending order; the topmost entry identifies the code path holding the maximum heap bytes during the run:
```text
PEAK MEMORY CONSUMERS
10.49M peak memory consumed over 10 calls from
load_all_then_process
  at /src/image_pipeline.c:27
```
*Use this section to locate OOM Killer risks.*

### 3. `MOST TEMPORARY ALLOCATIONS`
Reports the percentage of allocations that were subsequently deallocated:
```text
MOST TEMPORARY ALLOCATIONS
16 temporary allocations of 20 allocations in total (80.00%) from
process_sensor_data
  at /src/sensor.c:12
```
*Formula: $\text{Leaked Allocations} = \text{Total Allocations} - \text{Temporary Allocations}$.*

---

## Summary & Embedded Engineering Best Practices

| Diagnostic Metric in Heaptrack | Root Cause | Engineering Solution |
| :--- | :--- | :--- |
| `Temporary < 100%`, stair-step graph | Memory leak in error or exit branches | Ensure `free()` is invoked along all code paths. |
| `100% temporary`, but `Peak >> expected` | Batch loading or buffering entire files | Stream data in small chunks (e.g., 4KB–64KB). |
| High allocation count, small sizes | Excessive allocation churn | Use memory pools, object caches, or stack buffers. |

### Standard Profiling Workflow
```bash
# 1. Compile with debug symbols and no optimization
gcc -g -O0 -o my_app my_app.c

# 2. Record allocation profile with Heaptrack
heaptrack $(pwd)/my_app

# 3. Analyze textual report via CLI
heaptrack_print heaptrack.my_app.*.zst | head -n 60

# 4. Optional: Inspect interactive visual timeline via GUI
heaptrack_gui heaptrack.my_app.*.zst
```
