format ELF64 executable 3
entry _start

; Flatcore: standalone x86-64 Linux userspace core.
; No libc or external runtime is required.

SYS_read      = 0
SYS_write     = 1
SYS_open      = 2
SYS_close     = 3
SYS_unlink    = 87
SYS_mkdir     = 83
SYS_rmdir     = 84
SYS_rename    = 82
SYS_link      = 86
SYS_getpid    = 39
SYS_getcwd    = 79
SYS_uname     = 63
SYS_getdents64 = 217
SYS_exit      = 60

STDIN  = 0
STDOUT = 1
STDERR = 2

O_RDONLY   = 0
O_WRONLY   = 1
O_CREAT    = 64
O_DIRECTORY = 65536
MODE_FILE  = 420
MODE_DIR   = 493

segment readable executable

_start:
    mov r12, [rsp]                 ; argc
    lea r13, [rsp + 8]             ; argv

    test r12, r12
    jz .usage

    ; Resolve the invoked command from argv[0]'s basename.
    mov rsi, [r13]
    mov rbx, rsi
.find_basename:
    cmp byte [rbx], 0
    je .basename_ready
    cmp byte [rbx], '/'
    jne .next_basename_char
    lea rsi, [rbx + 1]
.next_basename_char:
    inc rbx
    jmp .find_basename

.basename_ready:
    mov rdi, rsi
    mov rbx, rsi                  ; preserve basename across comparison
    mov rsi, applet_acore
    call streq
    test eax, eax
    jnz .dispatch_argument

    ; Invoked through an applet symlink: argv[0] is the command.
    mov rdi, rbx
    mov r14, 1
    jmp .dispatch

.dispatch_argument:
    cmp r12, 2
    jb .usage
    mov rdi, [r13 + 8]
    mov r14, 2

.dispatch:
    mov rbx, rdi                  ; command name survives string comparisons
    mov rsi, applet_echo
    call command_is
    test eax, eax
    jnz cmd_echo

    mov rsi, applet_true
    call command_is
    test eax, eax
    jnz cmd_true

    mov rsi, applet_false
    call command_is
    test eax, eax
    jnz cmd_false

    mov rsi, applet_help
    call command_is
    test eax, eax
    jnz cmd_help

    mov rsi, applet_pwd
    call command_is
    test eax, eax
    jnz cmd_pwd

    mov rsi, applet_cat
    call command_is
    test eax, eax
    jnz cmd_cat

    mov rsi, applet_mkdir
    call command_is
    test eax, eax
    jnz cmd_mkdir

    mov rsi, applet_rmdir
    call command_is
    test eax, eax
    jnz cmd_rmdir

    mov rsi, applet_rm
    call command_is
    test eax, eax
    jnz cmd_rm

    mov rsi, applet_touch
    call command_is
    test eax, eax
    jnz cmd_touch

    mov rsi, applet_mv
    call command_is
    test eax, eax
    jnz cmd_mv

    mov rsi, applet_ln
    call command_is
    test eax, eax
    jnz cmd_ln

    mov rsi, applet_ls
    call command_is
    test eax, eax
    jnz cmd_ls

    mov rsi, applet_uname
    call command_is
    test eax, eax
    jnz cmd_uname

    mov rsi, applet_clear
    call command_is
    test eax, eax
    jnz cmd_clear

    jmp .unknown

.usage:
    mov rsi, msg_usage
    call puts
    mov edi, 2
    jmp terminate

.unknown:
    mov rsi, msg_unknown
    call puts_err
    mov edi, 127
    jmp terminate

; Compare command name in RDI with applet name in RSI.
command_is:
    mov rdi, rbx
    jmp streq

; ------------------------------------------------------------
; Core applets
; ------------------------------------------------------------

cmd_echo:
    mov r15, r14
    xor ebp, ebp
.echo_next:
    cmp r15, r12
    jae .echo_newline
    test ebp, ebp
    jz .echo_word
    mov rsi, str_space
    call puts
.echo_word:
    mov rsi, [r13 + r15 * 8]
    call puts
    inc r15
    mov ebp, 1
    jmp .echo_next
.echo_newline:
    mov rsi, str_newline
    call puts
    xor edi, edi
    jmp terminate

cmd_true:
    xor edi, edi
    jmp terminate

cmd_false:
    mov edi, 1
    jmp terminate

cmd_help:
    mov rsi, msg_help
    call puts
    xor edi, edi
    jmp terminate

cmd_pwd:
    mov rsi, cwd_buffer
    mov edx, 4096
    mov eax, SYS_getcwd
    syscall
    test rax, rax
    js command_error
    mov rsi, cwd_buffer
    call puts
    mov rsi, str_newline
    call puts
    xor edi, edi
    jmp terminate

cmd_cat:
    cmp r14, r12
    jae .stdin
    mov r14, r14
.next_file:
    cmp r14, r12
    jae .done
    mov rdi, [r13 + r14 * 8]
    xor esi, esi
    xor edx, edx
    mov eax, SYS_open
    syscall
    test rax, rax
    js command_error
    mov rbx, rax
    mov rdi, rbx
    call copy_fd_to_stdout
    mov rdi, rbx
    mov eax, SYS_close
    syscall
    test rax, rax
    js command_error
    inc r14
    jmp .next_file
.stdin:
    xor edi, edi
    call copy_fd_to_stdout
.done:
    xor edi, edi
    jmp terminate

cmd_mkdir:
    cmp r14, r12
    jae missing_argument
    mov r15, r14
.mkdir_next:
    cmp r15, r12
    jae .mkdir_done
    mov rdi, [r13 + r15 * 8]
    mov esi, MODE_DIR
    mov eax, SYS_mkdir
    syscall
    test rax, rax
    js command_error
    inc r15
    jmp .mkdir_next
.mkdir_done:
    xor edi, edi
    jmp terminate

cmd_rmdir:
    cmp r14, r12
    jae missing_argument
    mov r15, r14
.rmdir_next:
    cmp r15, r12
    jae .rmdir_done
    mov rdi, [r13 + r15 * 8]
    mov eax, SYS_rmdir
    syscall
    test rax, rax
    js command_error
    inc r15
    jmp .rmdir_next
.rmdir_done:
    xor edi, edi
    jmp terminate

cmd_rm:
    cmp r14, r12
    jae missing_argument
    mov r15, r14
.rm_next:
    cmp r15, r12
    jae .rm_done
    mov rdi, [r13 + r15 * 8]
    mov eax, SYS_unlink
    syscall
    test rax, rax
    js command_error
    inc r15
    jmp .rm_next
.rm_done:
    xor edi, edi
    jmp terminate

cmd_touch:
    cmp r14, r12
    jae missing_argument
    mov r15, r14
.touch_next:
    cmp r15, r12
    jae .touch_done
    mov rdi, [r13 + r15 * 8]
    mov esi, O_WRONLY + O_CREAT
    mov edx, MODE_FILE
    mov eax, SYS_open
    syscall
    test rax, rax
    js command_error
    mov rdi, rax
    mov eax, SYS_close
    syscall
    inc r15
    jmp .touch_next
.touch_done:
    xor edi, edi
    jmp terminate

cmd_mv:
    cmp r12, r14
    jb missing_argument
    mov rax, r12
    sub rax, r14
    cmp rax, 2
    jne missing_argument
    mov rdi, [r13 + r14 * 8]
    mov rsi, [r13 + r14 * 8 + 8]
    mov eax, SYS_rename
    syscall
    test rax, rax
    js command_error
    xor edi, edi
    jmp terminate

cmd_ln:
    mov rax, r12
    sub rax, r14
    cmp rax, 2
    jne missing_argument
    mov rdi, [r13 + r14 * 8]
    mov rsi, [r13 + r14 * 8 + 8]
    mov eax, SYS_link
    syscall
    test rax, rax
    js command_error
    xor edi, edi
    jmp terminate

cmd_ls:
    mov r15, r14
    cmp r15, r12
    jb .ls_have_path
    mov rdi, str_dot
    jmp .ls_open
.ls_have_path:
    mov rdi, [r13 + r15 * 8]
.ls_open:
    mov esi, O_RDONLY + O_DIRECTORY
    xor edx, edx
    mov eax, SYS_open
    syscall
    test rax, rax
    js command_error
    mov rbx, rax
.ls_read:
    mov rdi, rbx
    mov rsi, dir_buffer
    mov edx, 4096
    mov eax, SYS_getdents64
    syscall
    test rax, rax
    js .ls_close_error
    jz .ls_close
    mov r14, rax
    mov r15, dir_buffer
.ls_entry:
    cmp r14, 0
    je .ls_read
    lea rsi, [r15 + 19]             ; linux_dirent64.d_name
    call puts
    mov rsi, str_newline
    call puts
    movzx eax, word [r15 + 16]      ; d_reclen
    test eax, eax
    jz .ls_close_error
    add r15, rax
    sub r14, rax
    jmp .ls_entry
.ls_close_error:
    mov rdi, rbx
    mov eax, SYS_close
    syscall
    jmp command_error
.ls_close:
    mov rdi, rbx
    mov eax, SYS_close
    syscall
    xor edi, edi
    jmp terminate

cmd_uname:
    mov rdi, uname_buffer
    mov eax, SYS_uname
    syscall
    test rax, rax
    js command_error
    mov rsi, uname_buffer
    call puts
    mov rsi, str_space
    call puts
    lea rsi, [uname_buffer + 65]
    call puts
    mov rsi, str_space
    call puts
    lea rsi, [uname_buffer + 130]
    call puts
    mov rsi, str_space
    call puts
    lea rsi, [uname_buffer + 195]
    call puts
    mov rsi, str_space
    call puts
    lea rsi, [uname_buffer + 260]
    call puts
    mov rsi, str_newline
    call puts
    xor edi, edi
    jmp terminate

cmd_clear:
    mov rsi, str_clear
    call puts
    xor edi, edi
    jmp terminate

; ------------------------------------------------------------
; Runtime helpers
; ------------------------------------------------------------

; RDI = command, RSI = expected string. EAX = 1 on equality.
streq:
.compare:
    mov al, [rdi]
    cmp al, [rsi]
    jne .not_equal
    test al, al
    jz .equal
    inc rdi
    inc rsi
    jmp .compare
.equal:
    mov eax, 1
    ret
.not_equal:
    xor eax, eax
    ret

; RSI = NUL-terminated string. Writes to stdout.
puts:
    mov rdi, STDOUT
    jmp puts_fd

; RSI = NUL-terminated string. Writes to stderr.
puts_err:
    mov rdi, STDERR

puts_fd:
    xor edx, edx
.length:
    cmp byte [rsi + rdx], 0
    je .write
    inc edx
    jmp .length
.write:
    jmp write_all

; RDI = fd, RSI = buffer, RDX = byte count.
; Returns with RAX = bytes written or negative syscall error.
write_all:
    test rdx, rdx
    jz .empty
.write_more:
    mov eax, SYS_write
    syscall
    test rax, rax
    js .return
    jz .empty
    add rsi, rax
    sub rdx, rax
    jnz .write_more
.empty:
    xor eax, eax
.return:
    ret

; RDI = input fd. Copies it to stdout until EOF.
copy_fd_to_stdout:
    mov r15, rdi
.read_more:
    mov rdi, r15
    mov rsi, io_buffer
    mov edx, 4096
    mov eax, SYS_read
    syscall
    test rax, rax
    js .copy_error
    jz .copy_done
    mov rdx, rax
    mov rsi, io_buffer
    mov edi, STDOUT
    call write_all
    test rax, rax
    js .copy_error
    jmp .read_more
.copy_error:
    mov rsi, msg_io_error
    call puts_err
    mov edi, 1
    jmp terminate
.copy_done:
    ret

missing_argument:
    mov rsi, msg_missing
    call puts_err
    mov edi, 2
    jmp terminate

command_error:
    mov rsi, msg_command_error
    call puts_err
    mov edi, 1
    jmp terminate

terminate:
    mov eax, SYS_exit
    syscall
    ud2

segment readable writeable

applet_acore db 'acore', 0
applet_echo  db 'echo', 0
applet_true  db 'true', 0
applet_false db 'false', 0
applet_help  db 'help', 0
applet_pwd   db 'pwd', 0
applet_cat   db 'cat', 0
applet_mkdir db 'mkdir', 0
applet_rmdir db 'rmdir', 0
applet_rm    db 'rm', 0
applet_touch db 'touch', 0
applet_mv    db 'mv', 0
applet_ln    db 'ln', 0
applet_ls    db 'ls', 0
applet_uname db 'uname', 0
applet_clear db 'clear', 0

str_space   db ' ', 0
str_newline db 10, 0
str_dot     db '.', 0
str_clear   db 27, '[', '2', 'J', 27, '[', 'H', 0

msg_usage db 'Usage: acore <command> [arguments]', 10, 0
msg_unknown db 'acore: unknown command', 10, 0
msg_missing db 'acore: missing or invalid arguments', 10, 0
msg_command_error db 'acore: command failed', 10, 0
msg_io_error db 'acore: input/output error', 10, 0
msg_help db 'Flatcore core applets:', 10
         db '  echo true false help pwd cat ls', 10
         db '  mkdir rmdir rm touch mv ln uname clear', 10, 0

cwd_buffer rb 4096
io_buffer rb 4096
dir_buffer rb 4096
uname_buffer rb 390
