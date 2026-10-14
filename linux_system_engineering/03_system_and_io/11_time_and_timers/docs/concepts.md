# Core Concepts: Time, Clocks, Timers & Sleeping in Linux

## Reading Limits
- **Syntax.txt (TLPI)**: Chapter 10 (*Time*) from line **11572** to **12932**.
- **Syntax.txt (TLPI)**: Chapter 23 (*Timers and Sleeping*) from line **26736** to **28544**.

---

## 1. Overview & Architectural Scope

In Linux systems programming, time management bifurcates into two fundamental domains:
1. **Time Measurement**: Tracking calendar time (wall-clock time), monotonic intervals (elapsed time without discontinuous jumps), and process execution time (CPU cycles consumed in user vs. kernel mode).
2. **Time Scheduling (Timers & Sleep)**: Suspending threads for precise intervals, enforcing I/O timeouts, and arming periodic or single-shot alarms through signals, worker threads, or file descriptors.

Modern Linux provides multiple layers of timing APIs—from legacy System V/BSD primitives (`time`, `gettimeofday`, `setitimer`, `sleep`) to standard POSIX.1b interfaces (`clock_gettime`, `clock_nanosleep`, `timer_create`), culminating in the Linux-specific `timerfd` subsystem that unifies timers with kernel event loops (`epoll`).

---

## 2. Pillar 1: Time Representation & Conversion

### 2.1 Calendar Time & Epoch
UNIX represents calendar time internally as seconds elapsed since the **Epoch**: `00:00:00, 1 January 1970 UTC`.

- **`time_t`**: Integer type representing seconds since Epoch.
  - On 32-bit Linux systems, `time_t` is a signed 32-bit integer, maxing out on **19 January 2038 03:14:07 UTC** (**Year 2038 Problem / Y2038**).
  - On 64-bit systems, `time_t` is a signed 64-bit integer, safe for billions of years.
- **`struct timeval`**: Microsecond resolution (used by `gettimeofday()` and `setitimer()`):
  ```c
  struct timeval {
      time_t      tv_sec;     /* Seconds since Epoch */
      suseconds_t tv_usec;    /* Microseconds (long int) */
  };
  ```
- **`struct timespec`**: Nanosecond resolution (used by POSIX clocks, `nanosleep()`, and `timerfd`):
  ```c
  struct timespec {
      time_t tv_sec;          /* Seconds */
      long   tv_nsec;         /* Nanoseconds (0 - 999,999,999) */
  };
  ```

### 2.2 Time Conversion Flowchart
Linux distinguishes between Epoch scalar values and **Broken-Down Time** (`struct tm`), which splits time into human calendar units (year, month, day, hour, minute, second).

```
                      +-----------------------------+
                      |   time_t / struct timeval   |
                      |       (Calendar Time)       |
                      +-----------------------------+
                        |        ^              |
             gmtime()   |        |              | ctime()
            localtime() |        | mktime()     |
                        v        |              v
             +----------------------+    +-------------------------+
             |      struct tm       |    | Fixed Format String:    |
             |  (Broken-down time)  |    | "Wed Jun 8 14:22:34\n\0"|
             +----------------------+    +-------------------------+
                  |            ^
        asctime() |            | strptime()
        strftime()|            |
                  v            |
             +----------------------+
             |   Formatted String   |
             | (Custom / Localized) |
             +----------------------+
```

### 2.3 Broken-Down Time Structure (`struct tm`)
```c
struct tm {
    int tm_sec;    /* Seconds (0-60, allowing 1 leap second) */
    int tm_min;    /* Minutes (0-59) */
    int tm_hour;   /* Hours (0-23) */
    int tm_mday;   /* Day of the month (1-31) */
    int tm_mon;    /* Month (0-11, where 0 = January) */
    int tm_year;   /* Years since 1900 (e.g., 2026 -> 126) */
    int tm_wday;   /* Day of week (0-6, where 0 = Sunday) */
    int tm_yday;   /* Day of year (0-365, 0 = 1 Jan) */
    int tm_isdst;  /* Daylight Saving: >0 (in effect), 0 (not), <0 (auto-detect) */
};
```

### 2.4 Critical Conversion APIs & Reentrancy
| Function | Input $\rightarrow$ Output | Reentrant Safe Variant | Notes |
|:---|:---|:---|:---|
| `time(NULL)` | Epoch seconds | *Thread-safe* | Fast fallback for whole-second checks. |
| `gettimeofday(&tv, NULL)` | Sub-second calendar time | *Thread-safe* | Microsecond precision; `tz` argument is obsolete (always pass `NULL`). |
| `ctime(&t)` | `time_t` $\rightarrow$ string | `ctime_r(&t, buf)` | Static buffer! Overwritten by subsequent calls or other time functions. |
| `gmtime(&t)` | `time_t` $\rightarrow$ UTC `struct tm` | `gmtime_r(&t, &tm)` | Static buffer return; non-reentrant. |
| `localtime(&t)` | `time_t` $\rightarrow$ Local `struct tm` | `localtime_r(&t, &tm)` | Applies timezone and DST rules. Static buffer return. |
| `mktime(&tm)` | Local `struct tm` $\rightarrow$ `time_t` | *Thread-safe* | **Normalizes out-of-range fields** (e.g. `sec = 125` increments `min` by 2). Invaluable for date arithmetic. |
| `asctime(&tm)` | `struct tm` $\rightarrow$ string | `asctime_r(&tm, buf)` | Static buffer return. |
| `strftime(buf, sz, fmt, &tm)` | `struct tm` $\rightarrow$ Custom string | *Thread-safe* | Format specifiers: `%F` (`YYYY-MM-DD`), `%T` (`HH:MM:SS`), `%Z` (Timezone). |
| `strptime(str, fmt, &tm)` | String $\rightarrow$ `struct tm` | *Thread-safe* | Parses date/time text; does not initialize unset fields. Always `memset` target `tm` first. |

> [!WARNING]
> The classical functions `ctime()`, `gmtime()`, `localtime()`, and `asctime()` return pointers to **shared static memory** within glibc. Calls to one function will silently overwrite the return values of earlier calls. In multi-threaded applications, **always use the `_r` reentrant variants** (`localtime_r`, `gmtime_r`, `ctime_r`, `asctime_r`).

### 2.5 Timezones & Locales
- **Timezone Storage**: Stored as binary compiled rules in `/usr/share/zoneinfo/` (compiled via `zic(8)`). Local default is determined by symlink `/etc/localtime`.
- **`TZ` Environment Variable**: Overrides process timezone:
  - Relative file path: `TZ=":Asia/Ho_Chi_Minh"` or `TZ=":Europe/Berlin"`.
  - Direct POSIX rule string: `TZ="CET-1:00:00CEST-2:00:00,M3.5.0,M10.5.0"`.
  - Process initializes timezone data via `tzset()`.
- **Locales (`setlocale`)**: Governs language and date formatting conventions (`LC_TIME`).
  - To respect environmental locale in `strftime()`, call:
    ```c
    setlocale(LC_ALL, "");
    ```

### 2.6 Updating the System Clock
- **`settimeofday(const struct timeval *tv, NULL)`**: Sets calendar time abruptly. Requires `CAP_SYS_TIME` capability. Abrupt step changes can break build systems (`make`), log correlation, and database commit logs.
- **`adjtime(struct timeval *delta, struct timeval *olddelta)`**: Smoothly slews the system clock by slightly speeding up or slowing down clock ticks (typically at 1 second per 2000 seconds on Linux) until `delta` is eliminated. Prevents backward time jumps.
- **`adjtimex()`**: Linux-specific system call utilized by NTP daemons (e.g., `chrony`, `ntpd`) for phase-locked loop synchronization.

---

## 3. Pillar 2: Process Time & Kernel Software Clocks

### 3.1 Process Execution Time (CPU Time)
Process time measures the actual CPU cycles consumed by a process rather than wall-clock time:
- **User CPU Time (`utime`)**: Cycles spent running user-space instructions.
- **System CPU Time (`stime`)**: Cycles spent inside kernel space servicing system calls and page faults on behalf of the process.

```c
#include <sys/times.h>

struct tms {
    clock_t tms_utime;  /* User CPU time used by caller */
    clock_t tms_stime;  /* System CPU time used by caller */
    clock_t tms_cutime; /* User CPU time of all waited-for children */
    clock_t tms_cstime; /* System CPU time of all waited-for children */
};

clock_t times(struct tms *buf);
```

- **Clock Ticks**: Values in `struct tms` are returned in **clock ticks**. Convert to seconds by dividing by `sysconf(_SC_CLK_TCK)`:
  $$\text{User Seconds} = \frac{\text{tms\_utime}}{\text{sysconf(\_SC\_CLK\_TCK)}}$$
  On Linux x86/ARM, `sysconf(_SC_CLK_TCK)` is typically fixed at `100` (`USER_HZ`).
- **`clock()`**: Simpler ANSI C helper returning total CPU time (`utime + stime`) measured in units of `CLOCKS_PER_SEC` (standardized by POSIX at `1,000,000`):
  $$\text{Total CPU Seconds} = \frac{\text{clock()}}{\text{(double)CLOCKS\_PER\_SEC}}$$

### 3.2 The Kernel Software Clock (Jiffies) & High-Resolution Timers
- **Jiffy**: The granularity of the kernel's internal software clock, incremented once per timer interrupt at frequency `HZ` (configurable at kernel build: 100, 250, 300, or 1000 Hz; 1ms to 10ms per tick).
- **High-Resolution Timers (`CONFIG_HIGH_RES_TIMERS`)**:
  - In modern Linux (kernels $\ge$ 2.6.21), the kernel implements **hrtimers** driven by hardware timers (APIC, TSC, HPET, ARM Generic Timer).
  - Decouples timer expiration and sleep resolution from the jiffy rate, enabling nanosecond-level accuracy. Check capability and resolution via `clock_getres()`.

---

## 4. Pillar 3: Classical Interval Timers & Sleep APIs

### 4.1 Classical Interval Timers (`setitimer`, `alarm`)
Linux provides three distinct per-process interval timers via `setitimer()`:

```c
#include <sys/time.h>

int setitimer(int which, const struct itimerval *new_value, struct itimerval *old_value);
```

| Type (`which`) | Countdown Mechanism | Signal Delivered | Typical Use Case |
|:---|:---|:---|:---|
| `ITIMER_REAL` | Real (wall-clock) time | `SIGALRM` | Real-time timeouts and periodic ticks. |
| `ITIMER_VIRTUAL` | User-mode CPU time only | `SIGVTALRM` | Profiling user-space computation. |
| `ITIMER_PROF` | User + System CPU time | `SIGPROF` | Statistical CPU profilers (e.g., `gprof`). |

```c
struct itimerval {
    struct timeval it_interval; /* Periodic repeat interval (0 = single-shot) */
    struct timeval it_value;    /* Initial expiration delay (0 = disarm) */
};
```

- **`alarm(unsigned int seconds)`**: Simplified, single-shot `ITIMER_REAL` wrapper.
- **Shared Timer Conflict**: On Linux, `alarm()` and `setitimer(ITIMER_REAL, ...)` manipulate the **same underlying kernel timer**. Setting one overwrites any active timer established by the other.

### 4.2 Pattern: Enforcing Timeouts on Blocking System Calls
A primary systems engineering pattern for `alarm()` or `setitimer()` is preventing indefinite stalls on blocking I/O calls (e.g., `read()` from serial lines, FIFOs, or sockets):

```c
struct sigaction sa;
sa.sa_flags = 0;              /* CRUCIAL: Do NOT specify SA_RESTART */
sa.sa_handler = sigalrm_handler;
sigemptyset(&sa.sa_mask);
sigaction(SIGALRM, &sa, NULL);

alarm(5);                     /* Set 5-second deadline */
ssize_t n = read(fd, buf, sizeof(buf));
int saved_errno = errno;
alarm(0);                     /* Cancel alarm immediately */
errno = saved_errno;

if (n == -1 && errno == EINTR) {
    /* Operation timed out! Handle cancellation gracefully */
}
```

> [!IMPORTANT]
> The signal handler must be installed **without** `SA_RESTART`, ensuring that when `SIGALRM` fires, the blocked kernel system call aborts immediately with `errno = EINTR`.

### 4.3 Sleep APIs: Low vs. High Resolution
- **`sleep(unsigned int seconds)`**:
  - Suspends execution for whole seconds. Returns `0` on completion, or remaining unslept seconds if interrupted by an unmasked signal.
  - *Portability Pitfall*: On older UNIX platforms, `sleep()` was implemented via `alarm()` and `SIGALRM`. Never mix `sleep()` with `alarm()` or `setitimer()`.
- **`nanosleep(const struct timespec *req, struct timespec *rem)`**:
  - Nanosecond resolution.
  - Standard specifies it **must not** be implemented via signals; completely safe to combine with any timer.
  - If interrupted by a signal, returns `-1` with `errno = EINTR`. The remaining duration is written into `rem`, allowing restart loops:
    ```c
    while (nanosleep(&req, &rem) == -1 && errno == EINTR) {
        req = rem; /* Resume sleep for remaining duration */
    }
    ```

---

## 5. Pillar 4: POSIX Clocks & High-Resolution Timers (POSIX.1b)

POSIX.1b introduced nanosecond-precision clocks and dynamic interval timers, overcoming all limitations of legacy BSD timers. Compile with `-lrt` on older glibc.

### 5.1 POSIX Clock Types (`clockid_t`)

```c
#include <time.h>

int clock_gettime(clockid_t clk_id, struct timespec *tp);
int clock_getres(clockid_t clk_id, struct timespec *res);
int clock_settime(clockid_t clk_id, const struct timespec *tp);
```

| Clock ID | Characteristics | Settable? | Best Practice Use Case |
|:---|:---|:---|:---|
| **`CLOCK_REALTIME`** | System-wide wall-clock time since Epoch | Yes (`CAP_SYS_TIME`) | User timestamps, calendar logs. **Never use for interval measurements** (subject to NTP step corrections and manual adjustments). |
| **`CLOCK_MONOTONIC`** | Strictly increasing time since boot | No | **Default clock for benchmarking, timeouts, and state machines.** Monotonic and immune to wall-clock resets. |
| **`CLOCK_MONOTONIC_RAW`** | Hardware oscillator time unaffected by NTP frequency slewing | No | Real-time hardware synchronization, low-level DSP/CAN bus controllers. |
| **`CLOCK_PROCESS_CPUTIME_ID`** | Total CPU time consumed by all threads in process | No | Profiling process performance with nanosecond resolution. |
| **`CLOCK_THREAD_CPUTIME_ID`** | High-precision CPU time of the calling thread | No | Profiling thread workloads and identifying thread contention. |
| **`CLOCK_REALTIME_COARSE` / `CLOCK_MONOTONIC_COARSE`** | Fast in-kernel jiffy-level clock reads | No | High-throughput logging where avoiding hardware clock read overhead (VDSO) matters more than sub-millisecond precision. |

### 5.2 Absolute High-Resolution Sleep (`clock_nanosleep`) & Sleep Drift Elimination
The classical relative `nanosleep()` suffers from **Sleep Drift**:
When a process executes relative sleeps in a loop, preemption between sleep intervals and rounding up during signal restarts causes delays to accumulate:

$$\text{Actual Total Time} > \sum \text{Requested Intervals}$$

`clock_nanosleep()` solves this via the **`TIMER_ABSTIME`** flag:

```c
struct timespec deadline;
clock_gettime(CLOCK_MONOTONIC, &deadline);

for (;;) {
    /* Calculate next periodic tick at exactly T + 10ms */
    deadline.tv_nsec += 10000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }

    /* Sleep until absolute target on CLOCK_MONOTONIC */
    int s = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
    if (s == EINTR) {
        /* Interrupted: Simply loop and call clock_nanosleep again with same deadline! */
        continue;
    }
    do_periodic_task();
}
```

> [!TIP]
> With `TIMER_ABSTIME`, if a signal interrupts `clock_nanosleep()`, there is no need to calculate remaining time. Simply re-invoke the call with the exact same `deadline`. The kernel sleeps until the specified absolute point in time, completely eliminating cumulative clock drift.

---

### 5.3 POSIX Interval Timers (`timer_create`, `timer_settime`)
Unlike `setitimer()`, which is limited to 1 timer per type per process, POSIX interval timers allow processes to allocate **unlimited independent timers**.

```c
#include <signal.h>
#include <time.h>

int timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid);
int timer_settime(timer_t timerid, int flags,
                  const struct itimerspec *new_value, struct itimerspec *old_value);
int timer_gettime(timer_t timerid, struct itimerspec *curr_value);
int timer_delete(timer_t timerid);
```

#### Flexible Notification via `struct sigevent`:
```c
struct sigevent {
    int          sigev_notify; /* Notification method */
    int          sigev_signo;  /* Timer expiration signal */
    union sigval sigev_value;  /* User data passed to signal or thread */
    /* ... thread handler union ... */
};
```

1. **`SIGEV_SIGNAL`**: Generates a signal (standard or real-time signal `SIGRTMIN..SIGRTMAX`). The user payload `sigev_value` is made available to `sigaction` handlers through `siginfo_t.si_value` or via `sigwaitinfo()`.
2. **`SIGEV_THREAD`**: Automatically invokes a callback function in a new or pool worker thread (`sigev_notify_function`). Avoids signal safety constraints!
3. **`SIGEV_THREAD_ID` (Linux-specific)**: Directs the expiration signal to a specific kernel Thread ID (`gettid()`). Essential for multi-threaded runtime architectures.
4. **`SIGEV_NONE`**: Arm timer silently; status is queried synchronously using `timer_gettime()`.

#### Struct `itimerspec`:
```c
struct itimerspec {
    struct timespec it_interval; /* Periodic repeat interval (0 = single-shot) */
    struct timespec it_value;    /* Initial expiration deadline (0 = disarm) */
};
```

### 5.4 Handling Timer Overruns (`timer_getoverrun`)
If a periodic timer expires multiple times while its notification signal is blocked or delayed due to CPU scheduling contention:
- UNIX signals do not queue infinite duplicates (preventing queue saturation).
- Instead, the kernel merges notifications and increments an **overrun counter**.
- When the signal is finally accepted, retrieve missed ticks via:
  ```c
  int overruns = timer_getoverrun(timerid);
  /* or non-standard Linux field in siginfo_t: si->si_overrun */
  ```

---

## 6. Pillar 5: Linux `timerfd` Subsystem & Event-Loop Integration

### 6.1 The Event-Loop Impedance Mismatch
In high-concurrency network servers and embedded event dispatchers (e.g., using `epoll`, `poll`, or `select`), handling timers presents historical architectural hurdles:
- Signals (`SIGALRM`, POSIX signals) interrupt system calls, require async-signal-safe code, and suffer from delivery ordering races.
- Worker threads (`SIGEV_THREAD`) introduce thread synchronization overhead, mutex contention, and context switching.

### 6.2 The `timerfd` Solution
The Linux `timerfd` subsystem (available since Linux 2.6.25) represents a timer as a **standard file descriptor**. When the timer expires, the file descriptor becomes **readable** (`POLLIN` / `EPOLLIN`).

```
                    +---------------------------+
                    |  Hardware Clock / Kernel  |
                    +---------------------------+
                                  |
                           Timer Expiration
                                  v
                    +---------------------------+
                    |    timerfd (Linux VFS)    |
                    | [uint64_t expiration cnt] |
                    +---------------------------+
                                  |
                      Notifies Ready List (POLLIN)
                                  v
      +-------------------------------------------------------+
      |                  epoll_wait() Loop                    |
      |                                                       |
      |  [ Socket FD ]  --> Non-blocking TCP Read / Write     |
      |  [ timerfd   ]  --> Periodic Maintenance / Keepalive  |
      |  [ signalfd  ]  --> Graceful Shutdown Handling       |
      +-------------------------------------------------------+
```

### 6.3 `timerfd` API Specification
```c
#include <sys/timerfd.h>

int timerfd_create(int clockid, int flags);
int timerfd_settime(int fd, int flags,
                    const struct itimerspec *new_value,
                    struct itimerspec *old_value);
int timerfd_gettime(int fd, struct itimerspec *curr_value);
```

- **`timerfd_create()`**:
  - `clockid`: `CLOCK_REALTIME` or `CLOCK_MONOTONIC`.
  - `flags`:
    - `TFD_NONBLOCK`: Sets non-blocking I/O on the descriptor (avoids separate `fcntl()` call).
    - `TFD_CLOEXEC`: Sets close-on-exec flag (`FD_CLOEXEC`).
- **`timerfd_settime()`**:
  - Arms or disarms the timer using `struct itimerspec`.
  - Use flag `TFD_TIMER_ABSTIME` for absolute scheduling.

### 6.4 Reading Expiration Data
When `timerfd` triggers `EPOLLIN`:
```c
uint64_t expirations;
ssize_t s = read(tfd, &expirations, sizeof(uint64_t));
if (s == sizeof(uint64_t)) {
    printf("Timer fired! Missed/Accumulated ticks: %llu\n",
           (unsigned long long)expirations);
}
```
- A successful `read()` returns an 8-byte unsigned integer (`uint64_t`) indicating the number of expirations that occurred since the last `read()` or arming call.
- If no expirations have occurred:
  - In blocking mode: `read()` blocks until the next expiration.
  - In non-blocking mode (`TFD_NONBLOCK`): returns `-1` with `errno = EAGAIN`.
- When finished, close the descriptor with `close(tfd)` to release kernel timer resources.

---

## 7. Comprehensive Timing Mechanism Comparison Matrix

| Mechanism | Resolution | Clocks Supported | Multiplicity | Delivery Mechanism | Overrun Detection | Event Loop Integration | Portability |
|:---|:---|:---|:---|:---|:---|:---|:---|
| **`alarm()`** | 1 second | Wall-clock only | 1 per process | `SIGALRM` | No | Indirect (`signalfd`) | Standard POSIX |
| **`setitimer()`** | 1 microsecond | Real, Virtual, Prof | 1 per type | `SIGALRM`, `SIGVTALRM`, `SIGPROF` | No | Indirect (`signalfd`) | Traditional UNIX / POSIX |
| **`sleep()`** | 1 second | Wall-clock | Thread-level | Process suspended | N/A | None | Standard POSIX |
| **`nanosleep()`** | Nanosecond | Wall-clock | Thread-level | Process suspended | N/A | None | Standard POSIX |
| **`clock_nanosleep()`** | Nanosecond | `CLOCK_REALTIME`, `CLOCK_MONOTONIC`, etc. | Thread-level | Suspended (`TIMER_ABSTIME` drift-free) | N/A | None | Standard POSIX |
| **POSIX Timers (`timer_create`)** | Nanosecond | All POSIX clocks + CPU clocks | Unlimited | Signal (`SIGEV_SIGNAL`) or Thread (`SIGEV_THREAD`) | Yes (`timer_getoverrun`) | Indirect via `signalfd` or thread | POSIX.1b |
| **Linux `timerfd`** | Nanosecond | `CLOCK_REALTIME`, `CLOCK_MONOTONIC` | Unlimited | Readable 8-byte `uint64_t` on File Descriptor | Yes (encoded in read value) | **Native (`epoll`, `poll`, `select`)** | Linux-specific ($\ge$ 2.6.25) |

---

## 8. Summary & Key Engineering Directives

1. **Benchmarking & Interval Measurement**: Never use `gettimeofday()` or `CLOCK_REALTIME` for measuring time durations. System administrators or NTP daemons can step wall-clock time backward, resulting in negative intervals. **Always use `clock_gettime(CLOCK_MONOTONIC, ...)`**.
2. **Periodic Loops Without Drift**: When building periodic tasks, replace relative sleep loops with **`clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &target, NULL)`**.
3. **Multi-Threaded Safety**: Avoid legacy static buffer functions (`localtime`, `ctime`, `gmtime`). Always invoke their reentrant counterparts (`localtime_r`, `ctime_r`, `gmtime_r`).
4. **Asynchronous Server Architecture**: In modern event-driven engines (`epoll`), avoid signal-based timers. Use **`timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)`** to handle timer events uniformly alongside client socket descriptors.
