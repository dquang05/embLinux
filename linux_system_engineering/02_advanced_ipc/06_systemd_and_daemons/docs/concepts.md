# Core Concepts: Systemd and Daemons

This chapter covers the fundamentals of Daemons in Linux, the steps to manually detach a process from a terminal, how to handle initialization signals, and how logging works using the `syslog` facility, as well as the modern `systemd` approach. For detailed theory and API usage, refer to the specific topics linked below.

## 1. Daemons Overview
A daemon is a long-running background process that is not attached to a controlling terminal. It typically starts at system boot and runs until the system shuts down, handling system-wide tasks like network requests or hardware events.
👉 **Deep Dive:** [Daemon Basics and Creation](01_daemon_creation.md)
- Covers the specific sequence of system calls (`fork()`, `setsid()`, `umask()`, `chdir()`, `close()`) required to properly convert a standard process into a daemon (as detailed in TLPI Chapter 37).

## 2. Reinitializing Daemons (SIGHUP)
Daemons need a way to reload their configuration without terminating, as stopping a daemon might disrupt active services. By convention, daemons catch the `SIGHUP` signal to re-read configuration files and reopen log files.
👉 **Deep Dive:** [Signal Handling in Daemons](02_daemon_signals.md)
- Explains how to set up safe signal handlers for `SIGHUP` (reinitialize) and `SIGTERM` (graceful shutdown).

## 3. Logging Messages using syslog
Since daemons lack a controlling terminal, they cannot print error messages or general information to `stdout` or `stderr`. The standard solution in C is the `syslog` facility, which provides a centralized logging mechanism.
👉 **Deep Dive:** [The syslog API](03_syslog_api.md)
- Details the usage of `openlog()`, `syslog()`, and `closelog()`, along with understanding priority levels (e.g., `LOG_ERR`, `LOG_INFO`) and facilities.

## 4. Modern Daemons with Systemd
Modern Linux distributions use `systemd` to manage services. This significantly simplifies daemon creation because `systemd` can automatically handle backgrounding, logging, and restarting, allowing developers to write simpler foreground applications.
👉 **Deep Dive:** [Systemd Services](04_systemd_services.md)
- Explains how to write a `.service` file, the difference between `Type=simple` and `Type=forking`, and how standard output is automatically directed to `systemd-journald`.
