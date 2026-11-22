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
include 'tests/runtime-selftest.inc'

start:
    sub rsp, 40
    call fc_runtime_selftest
    mov ecx, eax
    call [ExitProcess]

section '.rdata' data readable
include 'src/common/errors_data.inc'

section '.idata' import data readable writeable
library kernel32, 'KERNEL32.DLL'
import kernel32, ExitProcess, 'ExitProcess'
