; Flatcore Linux x86-64 shared runtime self-test executable.
include 'src/include/flatcore.inc'
format ELF64 executable 3
entry _start

segment readable executable
include 'src/common/memory.inc'
include 'src/common/strings.inc'
include 'src/common/arguments.inc'
include 'src/common/utf.inc'
include 'src/common/formatting.inc'
include 'src/common/startup.inc'
include 'src/common/dispatcher.inc'
include 'src/common/errors.inc'
include 'src/platform/linux/io.inc'
include 'src/platform/linux/filesystem.inc'
include 'tests/runtime-selftest.inc'

_start:
    ; Linux enters _start with RSP 16-byte aligned; CALL establishes FC64 entry alignment.
    call fc_runtime_selftest
    mov edi, eax
    mov eax, 60
    syscall

segment readable writeable
include 'src/common/errors_data.inc'
include 'src/platform/linux/data.inc'
include 'tests/runtime-selftest-data.inc'
