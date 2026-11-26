; Shared FC64 mv implementation.
; mv [--] SOURCE DEST
; Replacement semantics are provided by os_rename on both platforms.

fc_cmd_mv_write:
    mov rcx, os_write
    jmp fc_write_all

fc_cmd_mv:
    push rbx
    push r12
    push r13
    push r14
    mov r12, rdi
    mov r13, rsi
    mov r14, 1

    cmp r12, 2
    jne .parse_options
    mov rsi, [r13 + 8]
    mov rdi, fc_cmd_mv_opt_help
    call fc_streq
    test eax, eax
    jnz .help
    mov rsi, [r13 + 8]
    mov rdi, fc_cmd_mv_opt_version
    call fc_streq
    test eax, eax
    jnz .version
.parse_options:
    cmp r12, 2
    jbe .usage
    mov rsi, [r13 + 8]
    cmp byte [rsi], '-'
    jne .operands
    cmp byte [rsi + 1], '-'
    jne .bad_option
    cmp byte [rsi + 2], 0
    jne .bad_option
    mov r14, 2
.operands:
    lea rax, [r14 + 2]
    cmp r12, rax
    jne .usage

    mov rdi, [r13 + r14 * 8]
    mov rsi, [r13 + r14 * 8 + 8]
    call os_rename
    test rax, rax
    jnz .operation_error
    xor eax, eax
    jmp .return

.help:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_mv_help_text
    mov edx, fc_cmd_mv_help_length
    call fc_cmd_mv_write
    xor eax, eax
    jmp .return
.version:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_mv_version_text
    mov edx, fc_cmd_mv_version_length
    call fc_cmd_mv_write
    xor eax, eax
    jmp .return
.operation_error:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_mv_error_text
    mov edx, fc_cmd_mv_error_length
    call fc_cmd_mv_write
    mov eax, 1
    jmp .return
.bad_option:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_mv_bad_option_text
    mov edx, fc_cmd_mv_bad_option_length
    call fc_cmd_mv_write
    mov eax, 2
    jmp .return
.usage:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_mv_usage_text
    mov edx, fc_cmd_mv_usage_length
    call fc_cmd_mv_write
    mov eax, 2
.return:
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
