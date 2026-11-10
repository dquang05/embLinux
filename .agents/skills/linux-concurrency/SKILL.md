---
name: linux-concurrency
description: Implement, review, or debug Linux user-space C/POSIX threads and C++ concurrency integrations involving shared state, races, mutexes, condition variables, deadlocks, or cancellation; not general process control or single-threaded C++.
---

# Linux concurrency

Establish the requested thread model, shared-state ownership, synchronization invariants and completion policy. Keep changes proportional to the task and preserve intentional teaching examples.

## Locate relevant knowledge

The skill directory, knowledge-base root and target project's working directory are distinct. Resolve the actual `SKILL.md` location, following symlinks (for example with `realpath`). For the canonical layout `<embLinux-root>/.agents/skills/linux-concurrency/SKILL.md`, ascend from the resolved skill directory through `skills` and `.agents` to `<embLinux-root>`. Confirm that layout and access to `linux_system_engineering/knowledge-index.md`; do not derive the knowledge base from the target project's working directory.

Read that index first, then only the relevant sections and examples, preferring `01_foundation/05_thread/`. Follow its current review statuses: reviewed material is not exhaustively verified, and not-reviewed entries are discovery hints. The data-race example is deliberately unsafe; the bounded-buffer example is finite and non-canceling, not a general shutdown design.

If canonical resolution, layout or access fails, report the unavailable knowledge source. Do not claim to have read inaccessible material or silently substitute similarly named target-project documentation. Continue with independently available sources when appropriate, disclosing the limitation.

## Perform the task

1. Identify required behavior, participating threads and the target C/C++ standard, libc and platform. Map shared mutable state, per-thread/TLS state, argument/result lifetimes, and who owns each resource. State the invariants before changing synchronization.
2. Select relevant knowledge-base material and check critical contracts against authoritative API/language sources. Distinguish ISO C/C++ memory rules, POSIX requirements, Linux behavior and implementation assumptions. For C++ integrations, account for RAII, exceptions and object lifetimes across thread boundaries; do not assume POSIX cancellation behaves like ordinary C++ exception handling.
3. Trace conflicting accesses and synchronization relationships. C/C++ data races cause undefined behavior, not merely lost increments; a matching result proves nothing. Choose atomics and memory orders only with justified ordering relationships. Fences do not repair racy ordinary accesses or instantly broadcast every update; cache coherence is not software synchronization.
4. Check mutex ownership, critical sections, lock ordering and deadlock risks under the actual resource model. Semaphores can support mutual exclusion but do not have mutex ownership semantics. Condition waits require a predicate protected by the associated mutex, checked in a loop after reacquisition; a wakeup does not guarantee the predicate is true.
5. Review `pthread_create()`, `pthread_join()`, `pthread_detach()`, partial startup, shutdown and cleanup paths. Check each API's actual return contract: many pthread operations return error numbers directly rather than setting errno. A failed join does not establish termination; do not read worker results or destroy shared synchronization objects on that assumption. Distinguish intentional whole-process failure from cooperative shutdown; avoid introducing a shutdown framework without a task requirement.
6. Consider cancellation, signals and multithreaded fork only when relevant. Deferred cancellation still needs resource/invariant cleanup; condition waits reacquire their mutex before cancellation cleanup. Signal masks are per-thread and process-directed delivery is not broadcast. In a multithreaded fork child only the caller survives, with applicable async-signal-safe restrictions until exec; atfork is not a universal inherited-lock repair.
7. Use the target project's build system and appropriate compiler flags. Run focused static analysis, bounded runtime checks, sanitizers or Valgrind when authorized and useful; check tool availability without automatically installing tools. Distinguish static reasoning, dynamic detection and proof. Passing native runs or clean Helgrind/TSan output do not prove thread safety or compatibility with another ABI/target. Report commands, assumptions, unsupported targets and untested failure paths.

## Ownership boundaries

Use `linux-process-control` as the primary skill for ordinary fork/exec/reaping and process lifecycle work. Coordinate both skills for multithreaded fork, process/thread signal boundaries or process termination affecting thread ownership. Do not merge their responsibilities or edit another skill as a side effect.

Socket protocols, unrelated network-server architecture, filesystem operations, generic kernel development and ordinary single-threaded C++ are outside this skill's primary scope. Read only material needed for the concrete concurrency requirement; do not scan entire repositories or copy textbook chapters into the task context.
