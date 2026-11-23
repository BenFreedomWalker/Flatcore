; First shared command tranche: true, false, and echo.
; All handlers use the FC64 command ABI: (argc, argv) -> exit status.

fc_cmd_true:
    xor eax, eax
    ret

fc_cmd_false:
    mov eax, 1
    ret

; echo [-n] [ARG...]
; Arguments are emitted byte-for-byte as UTF-8, separated by one ASCII space.
fc_cmd_echo:
    push rbx
    push r12
    push r13
    push r14
    sub rsp, 8
    mov r12, rdi
    mov r13, rsi
    mov r14, 1                 ; default: append newline
    cmp r12, 1
    jbe .echo_operands
    mov rdi, [r13 + 8]
    mov rsi, fc_cmd_echo_no_newline
    call fc_streq
    test eax, eax
    jz .echo_operands
    mov r14, 0
    add r13, 8
    dec r12
.echo_operands:
    mov ebx, 1
.echo_loop:
    cmp rbx, r12
    jae .echo_finish
    mov rdi, [r13 + rbx * 8]
    call fc_strlen
    mov rdx, rax
    mov rsi, [r13 + rbx * 8]
    mov rdi, [os_stdout]
    mov rcx, os_write
    call fc_write_all
    test eax, eax
    jnz .echo_error
    inc rbx
    cmp rbx, r12
    jae .echo_loop
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_space
    mov edx, 1
    mov rcx, os_write
    call fc_write_all
    test eax, eax
    jnz .echo_error
    jmp .echo_loop
.echo_finish:
    test r14, r14
    jz .echo_success
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_newline
    mov edx, 1
    mov rcx, os_write
    call fc_write_all
    test eax, eax
    jnz .echo_error
.echo_success:
    xor eax, eax
    jmp .echo_return
.echo_error:
    mov eax, 1
.echo_return:
    add rsp, 8
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
