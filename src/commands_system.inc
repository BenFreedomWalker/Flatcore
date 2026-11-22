; Basic system utilities using native Linux x86-64 syscalls.

command_hostname:
    mov eax, 63
    mov rdi, uname_buffer
    syscall
    test rax, rax
    js .hostname_error
    lea rsi, [uname_buffer + 65]
    xor edx, edx
.hostname_length:
    cmp byte [rsi + rdx], 0
    je .hostname_write
    inc rdx
    cmp rdx, 65
    jb .hostname_length
.hostname_write:
    mov edi, 1
    call write_all
    test eax, eax
    jnz .hostname_error
    mov rsi, newline_text
    call write_out
    xor edi, edi
    jmp process_exit
.hostname_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit

command_uptime:
    mov eax, 99
    mov rdi, sysinfo_buffer
    syscall
    test rax, rax
    js .uptime_error
    mov rax, qword [sysinfo_buffer]
    call write_number
    mov rsi, uptime_suffix
    call write_out
    xor edi, edi
    jmp process_exit
.uptime_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit

command_sleep:
    mov r12, [argc]
    lea rax, [r14 + 1]
    cmp r12, rax
    jne command_usage
    mov r13, [argv]
    mov rsi, [r13 + r14 * 8]
    xor ebx, ebx
    xor r15d, r15d
.sleep_parse:
    movzx eax, byte [rsi]
    test al, al
    jz .sleep_parsed
    cmp al, '0'
    jb .sleep_invalid
    cmp al, '9'
    ja .sleep_invalid
    imul rbx, rbx, 10
    sub eax, '0'
    add rbx, rax
    inc r15
    inc rsi
    jmp .sleep_parse
.sleep_parsed:
    test r15, r15
    jz .sleep_invalid
    mov [sleep_request], rbx
    mov qword [sleep_request + 8], 0
    mov eax, 35
    mov rdi, sleep_request
    mov rsi, sleep_remaining
    syscall
    test rax, rax
    js .sleep_error
    xor edi, edi
    jmp process_exit
.sleep_invalid:
    mov rsi, usage_text
    call write_err
    mov edi, 2
    jmp process_exit
.sleep_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit
