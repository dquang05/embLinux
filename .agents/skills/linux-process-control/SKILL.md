---
name: linux-process-control
description: Use for Linux C or C++ tasks involving process creation, exec, child reaping, process lifecycle, inherited file descriptors, or lifecycle-related signals; not for general threading or socket programming.
---

# Linux process control

Identify the requested task, target process relationships, lifecycle, status reporting, inherited descriptors, resource ownership and cleanup requirements. Select suitable APIs and verify critical contracts using authoritative sources when needed; distinguish POSIX, Linux and GNU behavior.

## Locate relevant knowledge

The skill directory, knowledge-base root, and target project root are distinct. Never infer the knowledge-base root from the target project's working directory.

Resolve the actual filesystem location of this `SKILL.md`, following symlinks when possible (for example, `realpath` on the skill file). For the canonical layout `<embLinux-root>/.agents/skills/linux-process-control/SKILL.md`, ascend from the resolved skill directory through `skills` and `.agents` to `<embLinux-root>`, then locate `linux_system_engineering/knowledge-index.md`. Confirm the expected layout and file are accessible before using them.

Read the index and only the relevant sections or examples. `04_process` is reviewed, not exhaustively verified; other indexed modules are not reviewed. Do not blindly copy educational examples into production code.

If resolution fails, the canonical layout differs, or sandbox permissions prevent access, do not substitute similarly named files from the target project or claim to have read unavailable material. Report the missing knowledge source and continue with independently available sources where appropriate.

## Perform the task

- Consider `fork`/`exec`, exit status and child reaping as relevant; check inherited file descriptors and lifecycle-related signals. Handle applicable API failures, `EINTR`, cleanup and process ownership without expanding into unrelated IPC, general concurrency or kernel development.
- Follow the target project's instructions and build system. Compile and test when authorized and appropriate; report evidence, uncertainty, assumptions and untested failure paths.
