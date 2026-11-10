# Linux systems engineering

This knowledge base combines Linux systems programming concepts and practical experiments. Modules generally contain `docs/` for concepts and API knowledge, `include/` for headers where present, `src/` for examples, and a module-specific `Makefile`.

- Start with [knowledge-index.md](knowledge-index.md); read only sections and examples needed for the task.
- Treat handwritten material as potentially fallible. Distinguish POSIX, Linux-specific, GNU-specific and implementation-dependent behavior; verify critical API semantics against authoritative sources when needed.
- Distinguish intentional educational simplifications from errors. Preserve the teaching purpose; do not turn every example into a production application.
- For Linux C code, check relevant API return values, `errno` versus directly returned error codes, resource ownership and cleanup, EOF, partial I/O, `EINTR`, `EPIPE`, signals, process lifecycle and synchronization where applicable.
- Use the existing build system. For applicable GCC-based C examples, use `-Wall -Wextra -Werror -g` as the baseline; consider the target project's existing configuration.
