# Chapter 11: Time, Clocks, Timers & Sleeping

This chapter covers time management, clocks, interval timers, and sleep mechanisms in Linux systems programming, synthesizing foundational POSIX standards and Linux-specific kernel innovations.

---

## 1. Objectives

- **Master Time Representations**: Understand calendar time (Epoch, `time_t`, `struct timeval`, `struct timespec`, and `Y2038`), broken-down time (`struct tm`), and thread-safe conversion APIs (`localtime_r`, `strftime`, `strptime`).
- **Understand Kernel Software Clocks & Jiffies**: Differentiate between user CPU time and system CPU time (`times()`, `clock()`), and explore how high-resolution timers (`CONFIG_HIGH_RES_TIMERS`) decouple timer precision from software jiffy ticks.
- **Implement Classical UNIX Timers & Sleep**: Master `setitimer()`, `alarm()`, `sleep()`, and `nanosleep()`, including timeout patterns for blocking I/O without `SA_RESTART`.
- **Utilize Modern POSIX Clocks & Timers**: Leverage `clock_gettime()` with `CLOCK_MONOTONIC` to eliminate wall-clock drift, use `clock_nanosleep()` with `TIMER_ABSTIME`, and deploy POSIX interval timers (`timer_create()`) with signal and thread notifications (`SIGEV_SIGNAL`, `SIGEV_THREAD`).
- **Integrate Timers with Event Loops via `timerfd`**: Employ Linux-specific `timerfd_create()` and `timerfd_settime()` to monitor timers as standard file descriptors inside `epoll` event loops.

---

## 2. References

- **The Linux Programming Interface (TLPI)**:
  - Chapter 10: *Time* (Lines 11572 – 12932)
  - Chapter 23: *Timers and Sleeping* (Lines 26736 – 28544)

---

## 3. Directory Structure

```text
11_time_and_timers/
├── README.md               # Chapter overview and quick reference
├── Makefile                # Build automation (-Wall -Wextra -Werror -g -lrt -pthread)
├── docs/
│   └── concepts.md         # Comprehensive theoretical synthesis and API guide
├── include/                # Header definitions
└── src/                    # Practical C source implementations
    ├── 01_calendar_and_cpu_time.c   # Wall-clock, monotonic benchmarking & CPU times
    ├── 02_blocking_timeout_alarm.c  # Timeout on blocking system call (alarm + EINTR)
    ├── 03_posix_periodic_timer.c    # POSIX real-time timer (timer_create + overruns)
    └── 04_timerfd_epoll_loop.c      # Linux timerfd integrated with epoll event loop
```

---

## 4. Documentation Guide

- Read the complete theoretical synthesis in **[docs/concepts.md](docs/concepts.md)**:
  - **Pillar 1**: Time Representation & Conversion (`time_t`, `struct tm`, timezones, locales, smooth clock slewing via `adjtime`).
  - **Pillar 2**: Process Time & Hardware Timing (`tms`, `sysconf(_SC_CLK_TCK)`, jiffies, and `hrtimers`).
  - **Pillar 3**: Classical Timers & Sleep (`setitimer`, `alarm`, `sleep`, `nanosleep`, blocking I/O timeouts).
  - **Pillar 4**: POSIX Clocks & Dynamic Timers (`clock_gettime`, `TIMER_ABSTIME`, `timer_create`, `timer_getoverrun`).
  - **Pillar 5**: Linux `timerfd` Subsystem & Event-Loop Integration (`epoll`).
  - **Comparison Matrix**: Detailed breakdown comparing all timing mechanisms.

---

## 5. Practical Applications & Build Instructions

### Compilation
All programs are compiled using strict flags (`-Wall -Wextra -Werror -g`) with realtime and threading libraries (`-lrt -pthread`):

```bash
cd linux_system_engineering/03_system_and_io/11_time_and_timers
make
```

### Running Practical Demos

1. **Calendar Time, Monotonic & CPU Benchmarking**:
   ```bash
   ./build/01_calendar_and_cpu_time
   ```
   *Demonstrates thread-safe `localtime_r()`, `strftime()`, nanosecond `CLOCK_MONOTONIC` elapsed time, and kernel user/system CPU cycles via `times()` and `USER_HZ`.*

2. **Blocking I/O Timeout with `alarm()`**:
   ```bash
   ./build/02_blocking_timeout_alarm
   ```
   *Demonstrates interrupting a stalled `read()` syscall via `SIGALRM` and `sa_flags = 0` (no `SA_RESTART`), trapping `errno == EINTR`, and safely disarming with `alarm(0)`.*

3. **POSIX Real-Time Interval Timer (`timer_create`)**:
   ```bash
   ./build/03_posix_periodic_timer
   ```
   *Demonstrates dynamic timer instantiation on `CLOCK_MONOTONIC`, Real-Time Signal notification (`SIGRTMIN+1`), payload inspection via `siginfo_t.si_value`, and checking overruns via `timer_getoverrun()`.*

4. **Linux `timerfd` Event-Loop Integration (`epoll`)**:
   ```bash
   ./build/04_timerfd_epoll_loop
   ```
   *Demonstrates modern zero-signal timer polling inside an `epoll_wait()` event loop by reading 8-byte expiration counters directly from `timerfd`.*
