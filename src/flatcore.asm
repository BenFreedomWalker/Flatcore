include 'src/include/flatcore.inc'

format ELF64 executable 3
entry _start

segment readable executable

; Flatcore: native Linux x86-64 multicall userspace.

_start:
    ; Capture the kernel-provided process arguments and normalize them through
    ; the shared FC64 startup helper before entering the legacy command table.
    mov rax, [rsp]
    mov [argc], rax
    lea rax, [rsp + 8]
    mov [argv], rax

    mov rdi, [argc]
    mov rsi, [argv]
    mov rdx, binary_name
    mov rcx, normalized_argc
    mov r8, normalized_argv
    call fc_prepare_command
    cmp rax, 1
    je command_help
    test rax, rax
    jnz .invalid_startup

    mov rax, [normalized_argv]
    cmp rax, [argv]
    je .multicall_startup
    mov r14, 2
    jmp .startup_ready
.multicall_startup:
    mov r14, 1
.startup_ready:
    mov rsi, [rax]
    jmp dispatch

.invalid_startup:
    mov edi, 2
    jmp process_exit

dispatch:
    mov rbx, rsi

    mov rsi, rbx
    mov rdi, name_help
    call streq
    test eax, eax
    jnz command_help

    mov rsi, rbx
    mov rdi, name_echo
    call streq
    test eax, eax
    jnz command_echo

    mov rsi, rbx
    mov rdi, name_true
    call streq
    test eax, eax
    jnz command_true

    mov rsi, rbx
    mov rdi, name_false
    call streq
    test eax, eax
    jnz command_false

    mov rsi, rbx
    mov rdi, name_pwd
    call streq
    test eax, eax
    jnz command_pwd

    mov rsi, rbx
    mov rdi, name_cat
    call streq
    test eax, eax
    jnz command_cat

    mov rsi, rbx
    mov rdi, name_ls
    call streq
    test eax, eax
    jnz command_ls

    mov rsi, rbx
    mov rdi, name_mkdir
    call streq
    test eax, eax
    jnz command_mkdir

    mov rsi, rbx
    mov rdi, name_rmdir
    call streq
    test eax, eax
    jnz command_rmdir

    mov rsi, rbx
    mov rdi, name_rm
    call streq
    test eax, eax
    jnz command_rm

    mov rsi, rbx
    mov rdi, name_touch
    call streq
    test eax, eax
    jnz command_touch

    mov rsi, rbx
    mov rdi, name_cp
    call streq
    test eax, eax
    jnz command_cp

    mov rsi, rbx
    mov rdi, name_mv
    call streq
    test eax, eax
    jnz command_mv

    mov rsi, rbx
    mov rdi, name_clear
    call streq
    test eax, eax
    jnz command_clear

    mov rsi, rbx
    mov rdi, name_uname
    call streq
    test eax, eax
    jnz command_uname

    mov rsi, rbx
    mov rdi, name_basename
    call streq
    test eax, eax
    jnz command_basename

    mov rsi, rbx
    mov rdi, name_dirname
    call streq
    test eax, eax
    jnz command_dirname

    mov rsi, rbx
    mov rdi, name_head
    call streq
    test eax, eax
    jnz command_head

    mov rsi, rbx
    mov rdi, name_wc
    call streq
    test eax, eax
    jnz command_wc

    mov rsi, rbx
    mov rdi, name_ln
    call streq
    test eax, eax
    jnz command_ln

    mov rsi, rbx
    mov rdi, name_readlink
    call streq
    test eax, eax
    jnz command_readlink

    mov rsi, rbx
    mov rdi, name_chmod
    call streq
    test eax, eax
    jnz command_chmod

    mov rsi, rbx
    mov rdi, name_kill
    call streq
    test eax, eax
    jnz command_kill

    mov rsi, rbx
    mov rdi, name_sync
    call streq
    test eax, eax
    jnz command_sync

    mov rsi, rbx
    mov rdi, name_id
    call streq
    test eax, eax
    jnz command_id

    mov rsi, rbx
    mov rdi, name_hostname
    call streq
    test eax, eax
    jnz command_hostname

    mov rsi, rbx
    mov rdi, name_uptime
    call streq
    test eax, eax
    jnz command_uptime

    mov rsi, rbx
    mov rdi, name_sleep
    call streq
    test eax, eax
    jnz command_sleep

    mov rsi, unknown_message
    call write_err
    mov edi, 127
    jmp process_exit

include 'src/commands.inc'
include 'src/commands_ls.inc'
include 'src/commands_extra.inc'
include 'src/commands_system.inc'
include 'src/runtime.inc'

include 'src/common/memory.inc'
include 'src/common/strings.inc'
include 'src/common/arguments.inc'
include 'src/common/utf.inc'
include 'src/common/formatting.inc'
include 'src/common/output.inc'
include 'src/common/startup.inc'
include 'src/common/dispatcher.inc'
include 'src/common/errors.inc'
include 'src/data.inc'
include 'src/common/errors_data.inc'
