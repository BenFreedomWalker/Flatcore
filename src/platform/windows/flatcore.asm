format PE64 console
entry start

section '.text' code readable executable

; Flatcore's first native Windows x86-64 entry point.
; This bootstrap deliberately uses Win32 APIs rather than Linux syscalls.

start:
    sub rsp, 72

    call [GetCommandLineA]
    mov r12, rax

    ; Skip argv[0], honoring a quoted executable path.
    mov rsi, r12
    cmp byte [rsi], '"'
    jne .skip_unquoted_exe
    inc rsi
.skip_quoted_exe:
    cmp byte [rsi], 0
    je .no_command
    cmp byte [rsi], '"'
    je .after_exe
    inc rsi
    jmp .skip_quoted_exe
.skip_unquoted_exe:
    cmp byte [rsi], 0
    je .no_command
    cmp byte [rsi], ' '
    je .after_exe
    cmp byte [rsi], 9
    je .after_exe
    inc rsi
    jmp .skip_unquoted_exe
.after_exe:
    ; Skip whitespace before command.
.skip_space:
    cmp byte [rsi], ' '
    je .space_next
    cmp byte [rsi], 9
    jne .command_start
.space_next:
    inc rsi
    jmp .skip_space
.command_start:
    mov r13, rsi
    xor ecx, ecx
.command_scan:
    mov al, [rsi + rcx]
    test al, al
    jz .command_end
    cmp al, ' '
    je .command_end
    cmp al, 9
    je .command_end
    inc rcx
    jmp .command_scan
.command_end:
    mov r14, rcx
    cmp byte [r13 + r14], 0
    je .args_at_end
    mov byte [r13 + r14], 0
    lea rsi, [r13 + r14 + 1]
    jmp .args_space
.args_at_end:
    lea rsi, [r13 + r14]
    ; Skip whitespace before command arguments.
.args_space:
    cmp byte [rsi], ' '
    je .args_next
    cmp byte [rsi], 9
    jne .dispatch
.args_next:
    inc rsi
    jmp .args_space

.dispatch:
    mov rdi, r13
    mov rsi, help_name
    call string_equal
    test eax, eax
    jnz .help

    mov rdi, r13
    mov rsi, true_name
    call string_equal
    test eax, eax
    jnz .success

    mov rdi, r13
    mov rsi, false_name
    call string_equal
    test eax, eax
    jnz .failure

    mov rdi, r13
    mov rsi, echo_name
    call string_equal
    test eax, eax
    jnz .echo

    mov rdi, r13
    mov rsi, pwd_name
    call string_equal
    test eax, eax
    jnz .pwd

    mov rdi, r13
    mov rsi, hostname_name
    call string_equal
    test eax, eax
    jnz .hostname

    mov rdi, r13
    mov rsi, mkdir_name
    call string_equal
    test eax, eax
    jnz .mkdir

    mov rdi, r13
    mov rsi, rmdir_name
    call string_equal
    test eax, eax
    jnz .rmdir

    mov rdi, r13
    mov rsi, rm_name
    call string_equal
    test eax, eax
    jnz .rm

    mov rdi, r13
    mov rsi, touch_name
    call string_equal
    test eax, eax
    jnz .touch

    mov rdi, r13
    mov rsi, cat_name
    call string_equal
    test eax, eax
    jnz .cat

    mov rdi, r13
    mov rsi, ls_name
    call string_equal
    test eax, eax
    jnz .ls

    mov rsi, unknown_text
    call write_stdout
    mov ecx, 127
    jmp .exit

.help:
    mov rsi, help_text
    call write_stdout
    jmp .success

.pwd:
    mov ecx, 4096
    mov rdx, cwd_buffer
    call [GetCurrentDirectoryA]
    test eax, eax
    jz .api_error
    mov edx, eax
    mov rsi, cwd_buffer
    call write_stdout_n
    mov rsi, newline_text
    mov edx, 2
    call write_stdout_n
    jmp .success

.hostname:
    mov dword [computer_name_length], 256
    mov rcx, computer_name_buffer
    mov rdx, computer_name_length
    call [GetComputerNameA]
    test eax, eax
    jz .api_error
    mov edx, [computer_name_length]
    mov rsi, computer_name_buffer
    call write_stdout_n
    mov rsi, newline_text
    mov edx, 2
    call write_stdout_n
    jmp .success

.ls:
    call first_argument
    test rax, rax
    jnz .ls_copy_path
    mov rax, dot_path
.ls_copy_path:
    mov rsi, rax
    mov rdi, ls_pattern
.ls_copy_loop:
    mov al, [rsi]
    test al, al
    jz .ls_pattern_end
    mov [rdi], al
    inc rsi
    inc rdi
    jmp .ls_copy_loop
.ls_pattern_end:
    mov byte [rdi], '\\'
    inc rdi
    mov byte [rdi], '*'
    inc rdi
    mov byte [rdi], 0
    mov rcx, ls_pattern
    mov rdx, find_data
    call [FindFirstFileA]
    cmp rax, -1
    je .api_error
    mov r15, rax
.ls_entry:
    lea rdi, [find_data + 44]
    call string_length
    test rax, rax
    jz .ls_next
    mov rsi, find_data + 44
    mov rdx, rax
    call write_stdout_n
    mov rsi, newline_text
    mov edx, 2
    call write_stdout_n
.ls_next:
    mov rcx, r15
    mov rdx, find_data
    call [FindNextFileA]
    test eax, eax
    jnz .ls_entry
    mov rcx, r15
    call [FindClose]
    jmp .success

.mkdir:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rcx, rax
    xor edx, edx
    call [CreateDirectoryA]
    test eax, eax
    jz .api_error
    jmp .success

.rmdir:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rcx, rax
    call [RemoveDirectoryA]
    test eax, eax
    jz .api_error
    jmp .success

.rm:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rcx, rax
    call [DeleteFileA]
    test eax, eax
    jz .api_error
    jmp .success

.touch:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rcx, rax
    mov edx, 40000000h           ; GENERIC_WRITE
    xor r8d, r8d                 ; no sharing
    xor r9d, r9d                 ; default security
    mov qword [rsp + 32], 4      ; OPEN_ALWAYS
    mov qword [rsp + 40], 80h    ; FILE_ATTRIBUTE_NORMAL
    mov qword [rsp + 48], 0
    call [CreateFileA]
    cmp rax, -1
    je .api_error
    mov r15, rax
    lea rcx, [current_file_time]
    call [GetSystemTimeAsFileTime]
    mov rcx, r15
    xor edx, edx
    xor r8d, r8d
    lea r9, [current_file_time]
    call [SetFileTime]
    test eax, eax
    jz .touch_close_error
    mov rcx, r15
    call [CloseHandle]
    jmp .success
.touch_close_error:
    mov rcx, r15
    call [CloseHandle]
    jmp .api_error

.cat:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rcx, rax
    mov edx, 80000000h           ; GENERIC_READ
    mov r8d, 1                   ; FILE_SHARE_READ
    xor r9d, r9d
    mov qword [rsp + 32], 3      ; OPEN_EXISTING
    mov qword [rsp + 40], 80h
    mov qword [rsp + 48], 0
    call [CreateFileA]
    cmp rax, -1
    je .api_error
    mov r15, rax
.cat_read:
    mov rcx, r15
    mov rdx, io_buffer
    mov r8d, 4096
    lea r9, [rsp + 56]
    mov qword [rsp + 32], 0
    mov dword [rsp + 56], 0
    call [ReadFile]
    test eax, eax
    jz .cat_close_error
    mov edx, [rsp + 56]
    test edx, edx
    jz .cat_close
    mov rsi, io_buffer
    call write_stdout_n
    jmp .cat_read
.cat_close_error:
    mov rcx, r15
    call [CloseHandle]
    jmp .api_error
.cat_close:
    mov rcx, r15
    call [CloseHandle]
    jmp .success

.usage_error:
    mov rsi, usage_error_text
    call write_stdout
    mov ecx, 2
    jmp .exit

.api_error:
    mov rsi, error_text
    call write_stdout
    mov ecx, 1
    jmp .exit

.echo:
    ; Write the remaining raw command-line text, followed by a newline.
    mov rdi, rsi
    call string_length
    test rax, rax
    jz .echo_newline
    mov rdx, rax
    call write_stdout_n
.echo_newline:
    mov rsi, newline_text
    mov edx, 2
    call write_stdout_n
    jmp .success

.no_command:
    mov rsi, help_text
    call write_stdout
.success:
    xor ecx, ecx
    jmp .exit
.failure:
    mov ecx, 1
    jmp .exit

.exit:
    call [ExitProcess]

; Compare zero-terminated strings: RDI and RSI. EAX=1 if equal.
string_equal:
.loop:
    mov al, [rdi]
    cmp al, [rsi]
    jne .no
    test al, al
    jz .yes
    inc rdi
    inc rsi
    jmp .loop
.yes:
    mov eax, 1
    ret
.no:
    xor eax, eax
    ret

; Return the length of the zero-terminated string in RDI.
string_length:
    xor eax, eax
.loop:
    cmp byte [rdi + rax], 0
    je .done
    inc rax
    jmp .loop
.done:
    ret

; Write a zero-terminated string in RSI to standard output.
write_stdout:
    sub rsp, 40
    mov rdi, rsi
    call string_length
    mov rdx, rax
    mov rsi, rdi
    add rsp, 40
    jmp write_stdout_n

; Write RDX bytes from RSI to standard output.
write_stdout_n:
    sub rsp, 40
    mov [rsp + 32], rsi
    mov [rsp + 24], rdx
    mov ecx, -11                 ; STD_OUTPUT_HANDLE
    call [GetStdHandle]
    mov rcx, rax
    mov rdx, [rsp + 32]
    mov r8, [rsp + 24]
    lea r9, [rsp + 16]
    mov qword [rsp + 16], 0
    call [WriteFile]
    add rsp, 40
    ret

section '.rdata' data readable

help_name db 'help', 0
echo_name db 'echo', 0
true_name db 'true', 0
false_name db 'false', 0
pwd_name db 'pwd', 0
hostname_name db 'hostname', 0
mkdir_name db 'mkdir', 0
rmdir_name db 'rmdir', 0
rm_name db 'rm', 0
touch_name db 'touch', 0
cat_name db 'cat', 0
help_text db 'Flatcore - native Windows x86-64 multicall userspace', 13, 10
          db 'Usage: flatcore.exe COMMAND [ARGUMENTS...]', 13, 10
          db 'Windows commands: help, echo, true, false, pwd, hostname, ls, mkdir, rmdir, rm, touch, cat', 13, 10, 0
error_text db 'flatcore: Windows API operation failed', 13, 10, 0
usage_error_text db 'flatcore: command requires a path argument', 13, 10, 0
unknown_text db 'flatcore: unknown command in this Windows build', 13, 10, 0
newline_text db 13, 10

section '.data' data readable writeable
cwd_buffer rb 4096
computer_name_buffer rb 256
computer_name_length dd 256
current_file_time dq 0
io_buffer rb 4096

section '.idata' import data readable writeable

library kernel32, 'KERNEL32.DLL'

import kernel32, \
    GetCommandLineA, 'GetCommandLineA', \
    GetCurrentDirectoryA, 'GetCurrentDirectoryA', \
    GetComputerNameA, 'GetComputerNameA', \
    CreateDirectoryA, 'CreateDirectoryA', \
    RemoveDirectoryA, 'RemoveDirectoryA', \
    DeleteFileA, 'DeleteFileA', \
    CreateFileA, 'CreateFileA', \
    ReadFile, 'ReadFile', \
    CloseHandle, 'CloseHandle', \
    GetSystemTimeAsFileTime, 'GetSystemTimeAsFileTime', \
    SetFileTime, 'SetFileTime', \
    GetStdHandle, 'GetStdHandle', \
    WriteFile, 'WriteFile', \
    ExitProcess, 'ExitProcess'
