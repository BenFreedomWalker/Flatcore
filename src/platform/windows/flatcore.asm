format PE64 console
entry start

section '.text' code readable executable

; Flatcore's first native Windows x86-64 entry point.
; This bootstrap deliberately uses Win32 APIs rather than Linux syscalls.

start:
    sub rsp, 40

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
help_text db 'Flatcore - native Windows x86-64 multicall userspace', 13, 10
          db 'Usage: flatcore.exe COMMAND [ARGUMENTS...]', 13, 10
          db 'Windows commands: help, echo, true, false, pwd, hostname', 13, 10, 0
error_text db 'flatcore: Windows API operation failed', 13, 10, 0
unknown_text db 'flatcore: unknown command in this Windows build', 13, 10, 0
newline_text db 13, 10

section '.data' data readable writeable
cwd_buffer rb 4096
computer_name_buffer rb 256
computer_name_length dd 256

section '.idata' import data readable writeable

library kernel32, 'KERNEL32.DLL'

import kernel32, \
    GetCommandLineA, 'GetCommandLineA', \
    GetCurrentDirectoryA, 'GetCurrentDirectoryA', \
    GetComputerNameA, 'GetComputerNameA', \
    GetStdHandle, 'GetStdHandle', \
    WriteFile, 'WriteFile', \
    ExitProcess, 'ExitProcess'
