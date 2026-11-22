format ELF64 executable 3
entry _start

segment readable executable

; Flatcore: native Linux x86-64 multicall userspace.

_start:
    mov rax, [rsp]
    mov [argc], rax
    lea rax, [rsp + 8]
    mov [argv], rax

    ; Resolve argv[0] basename.
    mov rsi, [rax]
    mov rbx, rsi
.basename:
    mov al, [rsi]
    test al, al
    jz .basename_done
    cmp al, '/'
    jne .basename_next
    lea rbx, [rsi + 1]
.basename_next:
    inc rsi
    jmp .basename
.basename_done:

    ; "flatcore COMMAND ..." or symlink invocation such as "ls ...".
    mov rsi, rbx
    mov rdi, binary_name
    call streq
    test eax, eax
    jnz .explicit_command
    mov r14, 1
    mov rsi, rbx
    jmp dispatch

.explicit_command:
    mov rax, [argc]
    cmp rax, 2
    jb command_help
    mov r14, 2
    mov rax, [argv]
    mov rsi, [rax + 8]

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

    mov rsi, unknown_message
    call write_err
    mov edi, 127
    jmp process_exit

include 'src/commands.inc'
include 'src/commands_extra.inc'
include 'src/runtime.inc'
include 'src/data.inc'
