; Flatcore Windows x86-64 shared runtime self-test executable.
include 'src/include/flatcore.inc'
include 'win64a.inc'
format PE64 console
entry start

section '.text' code readable executable
include 'src/common/memory.inc'
include 'src/common/strings.inc'
include 'src/common/arguments.inc'
include 'src/common/utf.inc'
include 'src/common/formatting.inc'
include 'src/common/startup.inc'
include 'src/common/dispatcher.inc'
include 'src/common/errors.inc'
include 'src/platform/windows/io.inc'
include 'src/platform/windows/filesystem.inc'
include 'tests/windows-backend-selftest.inc'
include 'tests/runtime-selftest.inc'

start:
    sub rsp, 40
    call os_init
    call fc_runtime_selftest
    mov ecx, eax
    test ecx, ecx
    jnz .exit
    call fc_windows_backend_selftest
    mov ecx, eax
.exit:
    call [ExitProcess]

section '.data' data readable writeable
include 'src/platform/windows/data.inc'
include 'src/common/errors_data.inc'
include 'tests/runtime-selftest-data.inc'
include 'tests/windows-backend-selftest-data.inc'

section '.idata' import data readable writeable
library kernel32, 'KERNEL32.DLL'
import kernel32, \
    ExitProcess, 'ExitProcess', \
    GetStdHandle, 'GetStdHandle', \
    GetLastError, 'GetLastError', \
    ReadFile, 'ReadFile', \
    WriteFile, 'WriteFile', \
    CloseHandle, 'CloseHandle', \
    CreateFileW, 'CreateFileW', \
    CreateDirectoryW, 'CreateDirectoryW', \
    RemoveDirectoryW, 'RemoveDirectoryW', \
    DeleteFileW, 'DeleteFileW', \
    MoveFileW, 'MoveFileW', \
    GetCurrentDirectoryW, 'GetCurrentDirectoryW', \
    SetCurrentDirectoryW, 'SetCurrentDirectoryW'
