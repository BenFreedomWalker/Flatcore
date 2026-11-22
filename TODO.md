# Flatcore Architecture Redesign

This file is the working engineering checklist for the shared-code redesign. It is separate from the README.

## Goal

Implement Flatcore once in pure FASM and build it for both Windows x86-64 (PE32+) and Linux x86-64 (ELF64). Command behavior, option parsing, formatting, and error handling must be shared. Only the native entry point and operating-system backend may differ.

No C or C++ components, no C runtime, and no third-party runtime dependencies.

## Working protocol

- Work through this checklist in order, one coherent stage at a time.
- Do not begin implementation until the user replies **ok**.
- After each approved stage, update this checklist with completed source work.
- Mark source implementation and static review separately from runtime verification.
- Do not mark runtime tests complete until the user runs the requested test and reports the result.
- When a stage is ready for user verification, explicitly say **test** and provide the exact commands.
- The user will run the tests and report the output before the next stage proceeds.
- Do not modify the README unless explicitly requested.

## Architecture decisions

- Shared command source must contain no direct Linux syscalls or Win32 API calls.
- Shared command source must not contain platform conditionals.
- Each platform has a native entry point and backend that implement the same documented internal OS interface.
- Shared runtime owns argument handling, command dispatch, string handling, formatting, common buffers, and command exit-status conventions.
- Commands return status to the dispatcher; commands do not terminate the process themselves.
- Use UTF-8 as the shared runtime's internal string encoding. Windows entry/backend performs UTF-16/UTF-8 conversion at OS boundaries and uses Unicode Win32 APIs.
- Linux backend uses native Linux x86-64 syscalls and must remain suitable for the eventual Flatcore PID 1/userspace role.
- Both targets compile the same shared command files. Build selection chooses the platform entry point/backend; developers do not edit shared command files to switch targets.
- Keep platform-specific features behind explicit backend capabilities rather than leaking platform details into shared commands.

## Stage 0 — Audit and freeze the current baseline

- [ ] Inventory every existing source file, command, global datum, helper, syscall, and Win32 API use.
- [ ] Record the current Windows and Linux command sets and their behavioral differences.
- [ ] Identify register-clobber assumptions and current calling conventions in both implementations.
- [ ] Inspect both build scripts and document their assembler/include requirements.
- [ ] Inspect both smoke-test suites and record current coverage.
- [ ] Define the migration map from existing files into the new source tree.
- [ ] Preserve the current implementation as a recoverable baseline until the new architecture builds.

**Runtime verification:** pending; baseline build and smoke tests must be run on both targets before migration is considered safe.

## Stage 1 — Define the shared ABI and OS backend contract

- [ ] Define one internal FASM calling convention for all shared runtime and backend functions.
- [ ] Specify argument registers, return registers, preserved registers, stack alignment, and scratch-register rules.
- [ ] Define consistent success and error return conventions, including how native OS error codes are exposed.
- [ ] Define standard input, standard output, and standard error handles.
- [ ] Define file-handle representation and invalid-handle representation.
- [ ] Define the shared file metadata structure and its field widths.
- [ ] Define directory iteration results and end-of-directory behavior.
- [ ] Define path and string encoding rules (UTF-8 internally).
- [ ] Define backend capabilities and behavior for unsupported operations.
- [ ] Document every public runtime/backend entry point in source comments.

**Runtime verification:** pending.

## Stage 2 — Establish the new source layout and build selection

- [ ] Create the src/common/ shared runtime area.
- [ ] Create the src/commands/ shared command area.
- [ ] Create src/platform/linux/ for Linux entry and backend code.
- [ ] Create src/platform/windows/ for Windows entry and backend code.
- [ ] Create shared API, constants, and structure include files.
- [ ] Make the Linux build include the shared sources plus only the Linux entry/backend.
- [ ] Make the Windows build include the same shared sources plus only the Windows entry/backend.
- [ ] Ensure switching build targets requires only selecting the build target, not editing command source.
- [ ] Keep output paths separate so one target cannot overwrite the other.
- [ ] Ensure both builds continue using FASM only.

**Runtime verification:** pending; both targets must produce their native executable formats.

## Stage 3 — Implement the common runtime

- [ ] Implement shared process startup state and argument-vector representation.
- [ ] Implement Windows command-line parsing with correct quoting and escaping, converting arguments to UTF-8.
- [ ] Use the Linux process-provided argc/argv and normalize them to the same shared representation.
- [ ] Implement shared string length, comparison, copy, and search primitives.
- [ ] Implement shared memory/buffer helpers required by commands.
- [ ] Implement shared integer parsing and number formatting.
- [ ] Implement shared output formatting and complete-write loops.
- [ ] Implement shared error-message formatting.
- [ ] Implement one command dispatcher and command registration table.
- [ ] Support invocation as flatcore COMMAND ...
- [ ] Define and implement multicall invocation behavior (dispatch by executable name where supported).
- [ ] Make commands return an exit status to the dispatcher.
- [ ] Make each native entry point perform the final OS-specific process exit.

**Runtime verification:** pending; test argument quoting, empty arguments, spaces, UTF-8, output streams, unknown commands, and exit statuses on both targets.

## Stage 4 — Implement the filesystem and I/O backends

- [ ] Implement os_read, os_write, and complete-write handling.
- [ ] Implement os_open, os_close, and file creation/truncation modes.
- [ ] Implement os_stat using the shared metadata structure.
- [ ] Implement os_dir_open, os_dir_next, and os_dir_close.
- [ ] Implement os_mkdir, os_rmdir, os_unlink, and os_rename.
- [ ] Implement os_getcwd and os_chdir.
- [ ] Implement file timestamps and the operations required by touch.
- [ ] Implement Linux syscall-backed versions.
- [ ] Implement Windows Unicode Win32-backed versions.
- [ ] Normalize common filesystem errors without discarding useful native error information.
- [ ] Define and test behavior for symlinks/reparse points and unsupported metadata differences.

**Runtime verification:** pending; run the same filesystem test cases on Windows and Linux.

## Stage 5 — Port shared commands

- [ ] Port true, false, and echo.
- [ ] Port pwd, basename, and dirname.
- [ ] Port cat, including standard input and multiple file operands.
- [ ] Port ls, including common GNU-style options and multiple operands.
- [ ] Port mkdir, including -p, -m, and -v where supported.
- [ ] Port rmdir.
- [ ] Port rm, including -f, -i, -r/-R, and -v.
- [ ] Port cp, including -f, -i, -n, -r/-R, and -v.
- [ ] Port mv, including -f, -i, -n, and -v.
- [ ] Port touch.
- [ ] Port head and tail.
- [ ] Port wc.
- [ ] Port ln and readlink, accounting for backend capability differences.
- [ ] Port chmod, accounting for Windows permission semantics and reporting unsupported modes clearly.
- [ ] Port clear, uname, hostname, uptime, and sleep through shared command logic and backend functions.
- [ ] Port remaining existing Linux system commands only after their OS requirements are represented in the backend contract.
- [ ] Ensure common commands support --help, --version where appropriate, -- option termination, combined short options where applicable, consistent diagnostics, and correct exit statuses.

**Runtime verification:** pending; command parity suite must run against both executables.

## Stage 6 — Make the test suite cross-platform

- [ ] Create platform-neutral test cases and fixtures for shared command behavior.
- [ ] Run identical command cases against the Linux and Windows builds.
- [ ] Compare normalized stdout, stderr, filesystem effects, and exit statuses.
- [ ] Add tests for spaces, Unicode names, hidden files, empty files, large files, and multiple operands.
- [ ] Add tests for invalid options, missing operands, permission failures, and nonexistent paths.
- [ ] Keep platform-specific expected differences explicit and narrowly scoped.
- [ ] Make build.sh test run the Linux suite.
- [ ] Make the Windows test script run the same behavioral cases against the Windows executable.
- [ ] Add a parity report so differences cannot silently accumulate.

**Runtime verification:** pending; user runs both test commands and reports results.

## Stage 7 — Retire the duplicated implementations

- [ ] Remove the old Linux command implementations after each command has migrated and passed tests.
- [ ] Remove the old Windows command implementations after each command has migrated and passed tests.
- [ ] Remove obsolete duplicate data and helper routines.
- [ ] Verify no shared command includes platform-specific APIs or syscalls.
- [ ] Verify both builds include the same shared command source set.
- [ ] Verify clean builds from a fresh checkout on both platforms.

**Runtime verification:** pending.

## Stage 8 — Prepare the Linux userspace foundation

- [ ] Separate ordinary process startup from the Linux PID 1 startup path.
- [ ] Define PID 1 signal handling and child-reaping requirements.
- [ ] Implement safe early-boot initialization without assuming a conventional desktop/session environment.
- [ ] Define mount, device, and pseudo-filesystem responsibilities.
- [ ] Define service supervision and shutdown/reboot interfaces.
- [ ] Keep Linux-only init and system-management facilities outside the shared command layer.
- [ ] Add an isolated test strategy for PID 1 behavior before using Flatcore as a real init.

**Runtime verification:** pending; PID 1 tests require a disposable VM or similarly isolated environment.

## Completion criteria

- [ ] Both native targets build from the same shared command and runtime sources.
- [ ] No command implementation is duplicated between Windows and Linux.
- [ ] Platform-specific code is confined to native entry points and backend implementations.
- [ ] Shared commands have matching documented behavior and option coverage.
- [ ] Cross-platform tests pass, with intentional OS differences documented.
- [ ] Linux retains a viable path to a pure-FASM PID 1 and complete userspace.
