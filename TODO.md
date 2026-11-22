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

- [x] Inventory every existing source file, command, global datum, helper, syscall, and Win32 API use.
- [x] Record the current Windows and Linux command sets and their behavioral differences.
- [x] Identify register-clobber assumptions and current calling conventions in both implementations.
- [x] Inspect both build scripts and document their assembler/include requirements.
- [x] Inspect both smoke-test suites and record current coverage.
- [x] Define the migration map from existing files into the new source tree.
- [x] Preserve the current implementation as a recoverable baseline until the new architecture builds.

### Baseline inventory (2026-11-23)

**Current source layout**

- Linux entry/dispatcher: `src/flatcore.asm`.
- Linux runtime helpers: `src/runtime.inc`.
- Linux command implementations: `src/commands.inc`, `src/commands_extra.inc`, `src/commands_ls.inc`, and `src/commands_system.inc`.
- Linux global data and command strings: `src/data.inc`.
- Separate Windows implementation: `src/platform/windows/flatcore.asm`.
- Linux build/test: `build.sh` and `tests/smoke.sh`.
- Windows build/test: `build-windows.bat` and `tests/windows-smoke.bat`.

**Command inventory**

- Linux dispatcher currently exposes 28 commands: `help`, `echo`, `true`, `false`, `pwd`, `cat`, `ls`, `mkdir`, `rmdir`, `rm`, `touch`, `cp`, `mv`, `clear`, `uname`, `basename`, `dirname`, `head`, `wc`, `ln`, `readlink`, `chmod`, `kill`, `sync`, `id`, `hostname`, `uptime`, and `sleep`. (The build script creates command-name symlinks for the dispatcher command set.)
- Windows currently implements 17 commands: `help`, `echo`, `true`, `false`, `pwd`, `hostname`, `mkdir`, `rmdir`, `rm`, `touch`, `cat`, `ls`, `cp`, `mv`, `basename`, `dirname`, and `head` (including `help`; actual availability is determined by its dispatch table).
- The two targets have separate dispatchers, parsers, data, output helpers, and command bodies. Their command coverage and semantics are not at parity.
- Windows currently parses a raw `GetCommandLineA` string itself and uses ANSI (`A)-suffixed) Win32 APIs. It does not currently normalize a Windows argv vector or provide UTF-8/UTF-16 boundary handling.
- Linux receives the kernel argc/argv stack directly and uses Linux x86-64 syscalls.

**Linux native interface currently used**

- Syscalls include `read` (0), `write` (1), `open` (2), `close` (3), `getcwd` (79), `rename` (82), `mkdir` (83), `rmdir` (84), `link` (86), `unlink` (87), `symlink` (88), `readlink` (89), `chmod` (90), `getuid` (102), `getgid` (104), `sysinfo` (99), `kill` (62), `uname` (63), `sync` (162), `getdents64` (217), `newfstatat` (262), `utimensat` (280), `nanosleep` (35), and `exit` (60).
- Runtime helpers are Linux-specific: `write_all`, `copy_fd`, and `process_exit` issue syscalls directly. Commands also issue syscalls directly rather than using a backend interface.
- The Linux syscall ABI is used directly (syscall number in RAX; arguments in RDI, RSI, RDX, R10, R8, R9). There is no documented shared internal calling convention yet.

**Windows native interface currently used**

- The Windows executable imports Kernel32 functions including `GetCommandLineA`, `GetCurrentDirectoryA`, `GetComputerNameA`, `CreateDirectoryA`, `RemoveDirectoryA`, `DeleteFileA`, `CreateFileA`, `ReadFile`, `CloseHandle`, `GetSystemTimeAsFileTime`, `SetFileTime`, `FindFirstFileA`, `FindNextFileA`, `FindClose`, `CopyFileA`, `MoveFileA`, `GetStdHandle`, `WriteFile`, and `ExitProcess`.
- Windows code uses the Win64 ABI (RCX, RDX, R8, R9 plus stack arguments; 32-byte shadow space and 16-byte stack alignment at call boundaries). There is no common internal ABI shared with Linux.
- Most Windows commands are implemented inline in the single Windows assembly file. Linux and Windows implementations have different feature sets and behavior.

**Build and test baseline**

- Linux: `build.sh` invokes FASM on `src/flatcore.asm`, creates symlinks for command invocation, and supports `build`, `test`, `install`, and `clean`.
- Windows: `build-windows.bat` invokes FASM on `src/platform/windows/flatcore.asm`, locates the FASM include directory for `win64a.inc`, and optionally runs the Windows batch smoke test with the `test` argument.
- Linux smoke coverage exercises basic command behavior, file operations, ls behavior, links, permissions, system utilities, and error cases.
- Windows smoke coverage exercises basic startup, status codes, echo, pwd, hostname, basic directory/file operations, ls, cp/mv, cat/head, basename, and dirname.
- The current test suites are not shared and do not compare equivalent cases across targets.
- The current implementation is preserved in Git history on `main`. No old implementation files will be removed until the replacement builds and passes the baseline tests.

**Migration map**

| Existing source | Destination |
|---|---|
| `src/flatcore.asm` dispatcher/startup | `src/common/dispatcher.inc` plus `src/platform/linux/entry.asm` |
| `src/runtime.inc` | Common string/output/memory helpers where portable; syscall operations move to `src/platform/linux/` |
| `src/commands*.inc` | One implementation per command under `src/commands/`; OS operations replaced by backend calls |
| `src/data.inc` | Common command data in `src/commands/` and runtime data in `src/common/`; Linux-only layouts move to Linux backend |
| `src/platform/windows/flatcore.asm` | Windows entry/backend under `src/platform/windows/`; command bodies migrate into shared command files |
| `build.sh`, `build-windows.bat` | Retain front-end scripts, make both assemble the same common source set with one selected backend |
| `tests/smoke.sh`, `tests/windows-smoke.bat` | Shared behavioral test cases with thin Linux/Windows runners |

**Runtime verification:** pending; the first user run exposed test-runner defects before the suites could execute. The defects have been repaired on `main`; rerun both commands below to establish the baseline.

**Test-runner repairs (2026-11-23)**

- Normalized `build.sh` to Unix LF line endings so its Bash shebang works under WSL.
- Rebuilt `tests/smoke.sh` as a clean, valid script; the previous file contained duplicated/truncated trailing content.
- Corrected the Windows smoke test root path: it now resolves the executable from the repository root rather than looking under `tests/build/`.
- Made `build-windows.bat test` propagate the smoke-test exit status and stop on assembler failure.
- No command implementation or platform architecture code was changed.

## Stage 1 — Define the shared ABI and OS backend contract

- [x] Define one internal FASM calling convention for all shared runtime and backend functions.
- [x] Specify argument registers, return registers, preserved registers, stack alignment, and scratch-register rules.
- [x] Define consistent success and error return conventions, including how native OS error codes are exposed.
- [x] Define standard input, standard output, and standard error handles.
- [x] Define file-handle representation and invalid-handle representation.
- [x] Define the shared file metadata structure and its field widths.
- [x] Define directory iteration results and end-of-directory behavior.
- [x] Define path and string encoding rules (UTF-8 internally).
- [x] Define backend capabilities and behavior for unsupported operations.
- [x] Document every public runtime/backend entry point in source comments.

**Stage 1 contract (2026-11-23)**

- Added `src/include/api.inc` as the authoritative FC64 ABI/backend contract.
- FC64 uses RDI, RSI, RDX, RCX, R8, R9 for arguments and RAX for results; RBX, RBP, R12-R15, and RSP are callee-preserved. RSP is 16-byte aligned immediately before CALL. Calls with more than six arguments use a parameter block.
- Backend calls use 0 for success and -1 for failure, with normalized and native error details in common diagnostic state. Directory iteration has explicit entry / EOF / error results.
- Defined opaque 64-bit handles, standard stream handles, UTF-8 string/path boundaries, the 64-byte FC_FILE_INFO layout, normalized error codes, open flags, file types, attributes, and optional capability bits.
- Documented the native ABI adapters: Linux syscall ABI and Microsoft x64 ABI remain confined to their respective backends.
- This stage defines the contract only. It does not yet replace or alter either existing executable implementation.

**Runtime verification:** pending; Stage 1 is a specification-only change. Validate the include syntax during Stage 2 when both new build entry points consume it.

## Stage 2 — Establish the new source layout and build selection

- [x] Create the src/common/ shared runtime area.
- [x] Create the src/commands/ shared command area.
- [x] Create src/platform/linux/ for Linux entry and backend code.
- [x] Create src/platform/windows/ for Windows entry and backend code.
- [x] Create a shared include umbrella at src/include/flatcore.inc.
- [ ] Split the API contract into dedicated constants and structure include files (deferred until implementation needs require it).
- [ ] Make the Linux build include the shared runtime/command sources plus only the Linux entry/backend.
- [ ] Make the Windows build include the same shared runtime/command sources plus only the Windows entry/backend.
- [ ] Ensure switching build targets requires only selecting the build target, not editing command source.
- [x] Keep output paths separate so one target cannot overwrite the other.
- [x] Ensure both builds continue using FASM only.

**Stage 2 source work completed so far (2026-11-23)**

- Created the shared runtime, command, and per-platform source areas without deleting or moving the proven legacy implementation.
- Added src/include/flatcore.inc as the single shared include entry point; it currently imports the FC64 API contract.
- Both existing native entry files now include the shared umbrella, so both assemblers validate the same contract.
- Made the Windows build run from the repository root so shared include paths resolve consistently regardless of the caller's working directory.
- Kept the existing Linux and Windows output locations separate.
- The legacy entry points and command implementations are intentionally still active. Switching both executables to the new shared runtime is a later migration step, not something to fake with empty stubs.

**Runtime verification: passed (2026-11-23).** The user ran `./build.sh test` under WSL with FASM 1.73.32 (18 passes, 17,844-byte executable) and `build-windows.bat test` under Windows with FASM 1.73.35 (18 passes, 16,896-byte executable). Both legacy smoke suites passed. Both targets assembled successfully and both existing smoke suites passed. The Linux `flatcore: operation failed` line is emitted by an expected negative/error-path test; the suite completed successfully.

## Stage 3 — Implement the common runtime

- [x] Implement the shared command-selection/argv-normalization helper for explicit and multicall invocation.
- [x] Integrate the UTF-8 command-line tokenizer and UTF-16 conversion into the Windows native startup capture path; legacy command handlers still consume their original command-line tail during migration.
- [x] Use the Linux process-provided argc/argv and normalize them through the shared startup helper.
- [x] Implement shared string length, comparison, copy, and search primitives.
- [x] Implement shared memory/buffer helpers required by commands.
- [x] Implement shared unsigned integer parsing and decimal formatting.
- [x] Implement strict UTF-16LE-to-UTF-8 conversion with surrogate validation.
- [x] Implement a backend-neutral complete-write loop and NUL-terminated string output helper.
- [x] Implement shared normalized-error-to-message mapping.
- [x] Implement the shared table-driven dispatcher and define its command registry/handler layout.
- [x] Define and implement multicall invocation normalization (dispatch by executable name where supported; Linux command links retain their existing dispatch path).
- [x] Define the shared command-handler exit-status contract; migrate command bodies in the later command-port stage.
- [x] Keep final process termination at the native entry boundary (Linux exit syscall / Windows ExitProcess); shared handlers return status only.
- [x] Add direct runtime tests for the shared helpers and run them on both platforms.

**Stage 3 implementation checkpoint (2026-11-23)**

Added platform-neutral FC64 modules under src/common/:

- memory.inc: byte-copy and byte-fill primitives.
- strings.inc: UTF-8 byte length, equality, bounded copy, and substring search.
- arguments.inc: a bounded Windows-style UTF-8 command-line tokenizer with quote/backslash handling. utf.inc: strict UTF-16LE-to-UTF-8 conversion with surrogate-pair validation. startup.inc: common explicit-command versus multicall argv normalization. errors.inc/errors_data.inc: normalized error-code mapping code and separate static message data, placed in executable/read-only sections as appropriate. These helpers are compiled into both targets; replacing the legacy native startup/dispatch paths and wiring the Windows Unicode APIs remain pending.
- formatting.inc: checked unsigned decimal parsing and decimal formatting.
- output.inc: complete-write loop routed through a backend callback, plus NUL-terminated string output.
- dispatcher.inc: one registry-driven dispatcher. Registry entries are pairs of command-name and handler pointers; handlers receive argc/argv and return the process status in RAX.

Both existing native source files include and assemble these same common modules. The legacy command dispatchers remain active for now; the new shared startup selector and dispatcher are compiled but not yet used by the executable entry paths. Windows still uses its ANSI command-line API; Unicode startup integration is pending. No existing command implementation has been removed.

**Runtime verification: passed for the first common-module integration checkpoint (2026-11-23).** The user ran `./build.sh test` under WSL with FASM 1.73.32 and `build-windows.bat test` under Windows with FASM 1.73.35. Both targets assembled the shared modules and both existing smoke suites passed. The Linux `flatcore: operation failed` line is the expected negative/error-path case. The newly added startup-selection and error-description helpers now also compile into both targets, with code and static strings placed in their correct executable/read-only sections; the legacy smoke suites still do not exercise these routines directly, so dedicated helper tests remain required. A subsequent user checkpoint on 2026-11-23 again passed both builds and smoke suites after adding startup normalization and error mapping.

**Shared helper test harness — runtime verification passed (2026-11-23).**

- `tests/runtime-selftest.inc` exercises string comparison/copy, UINT64_MAX formatting and parsing, UTF-16-to-UTF-8 conversion (including a surrogate pair), Windows-style quoted argument parsing, explicit and multicall argv normalization, table-driven dispatch, and normalized error messages.
- `tests/runtime-linux.asm` and `tests/runtime-windows.asm` provide native test entry points while executing the same shared test routine.
- Both `build.sh test` and `build-windows.bat test` now assemble and run the helper harness before the existing smoke suite.
- The multicall normalizer rewrites only the argv[0] pointer to its basename; argument strings remain unchanged. This is required for registry lookup when launched through a path such as `/usr/bin/echo`.
- Initial test runs exposed three faulty test assertions, not runtime defects: the explicit-command test compared the argv-vector pointer instead of `argv[0]`; the multicall test compared pointers rather than basename contents; and two failure paths initially shared a generic status. These were corrected in commits `60f7390`, `8108199`, and `9077ff8` (with earlier diagnostic work in `493de5f`).

**Linux startup test correction (2026-11-23; awaiting user verification)**

- The first dual-platform run after native startup integration passed Windows completely. Linux's shared helper harness passed, but the smoke suite exited with status 2.
- Inspection found a control-flow omission in `src/flatcore.asm`: successful startup normalization fell directly into the invalid-startup exit block instead of jumping to the legacy dispatcher.
- Added the missing `jmp dispatch` after selecting the normalized command pointer (commit `75b11d1`). Invalid startup still exits with status 2.
- Rerun both platform suites; Windows should remain unchanged and Linux should now reach its command dispatcher and complete the smoke tests.

**Native startup integration — runtime verification passed (2026-11-23).**

- The user rebuilt both targets after the dispatcher handoff correction. Windows shared runtime helper tests and smoke tests passed; Linux shared runtime helper tests and smoke tests passed. Both returned exit status 0.
- The Linux `flatcore: operation failed` line is expected output from a negative/error-path smoke case; the suite then reports success.
- The Windows startup path captures and validates the Unicode command line through the shared UTF-16 conversion, tokenizer, and command-normalization helpers. Legacy command handlers still use their original ANSI argument path pending command migration.
- The Linux native entry captures kernel argc/argv, normalizes command selection through the shared helper, and now explicitly jumps into the existing dispatcher after successful normalization.
- Native startup integration is verified. The shared dispatcher is helper-tested but is not yet the production command dispatcher; activation is deferred until shared commands and their registry are ready.

## Stage 4 — Implement the filesystem and I/O backends

- [ ] Implement os_read, os_write, and complete-write handling.
- [ ] Implement os_open, os_close, and file creation/truncation modes.
- [ ] Implement os_stat using the shared metadata structure.
- [ ] Implement os_dir_open, os_dir_next, and os_dir_close.
- [x] Implement os_mkdir, os_rmdir, os_unlink, and os_rename in the Linux backend.
- [x] Implement os_getcwd and os_chdir in the Linux backend.
- [ ] Implement file timestamps and the operations required by touch.
- [ ] Complete Linux syscall-backed versions.
- [ ] Implement Windows Unicode Win32-backed versions.
- [x] Add Linux errno-to-FC_ERR_* normalization while retaining the native errno and error domain.
- [ ] Define and test behavior for symlinks/reparse points and unsupported metadata differences.

**Stage 4 Linux backend source checkpoint (2026-11-23)**

- Added `src/platform/linux/io.inc` with FC64 `os_read`, `os_write`, `os_close`, native process exit, and normalized Linux errno handling.
- Added `src/platform/linux/filesystem.inc` with FC64 `os_open`, `os_mkdir`, `os_rmdir`, `os_unlink`, `os_rename`, `os_getcwd`, and `os_chdir`.
- Added `src/platform/linux/data.inc` for standard descriptors and backend error state. The Linux runtime self-test target now assembles these backend modules as part of its build.
- Preserved FC64 output pointers across the Linux `syscall` instruction, which clobbers RCX and R11. Open access-mode decoding masks the access bits so CREATE/TRUNCATE/APPEND flags do not invalidate the mode.
- These routines are not yet wired into the production executable or covered by direct filesystem assertions. `os_stat`, directory iteration, timestamps, and the Windows Unicode backend remain outstanding.

**Runtime verification:** pending; no Stage 4 backend test checkpoint has been requested yet.

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
