; Shared FC64 cp implementation.
; cp [--] SOURCE DEST
; Copies a regular file through the common read/write backend. The destination
; is created or truncated; source and destination must be different paths.

fc_cmd_cp_write:
    mov rcx, os_write
    jmp fc_write_all

fc_cmd_cp:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov r13, rsi
    mov r14, 1

    cmp r12, 2
    jne .parse_options
    mov rsi, [r13 + 8]
    mov rdi, fc_cmd_cp_opt_help
    call fc_streq
    test eax, eax
    jnz .help
    mov rsi, [r13 + 8]
    mov rdi, fc_cmd_cp_opt_version
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

    mov rsi, [r13 + r14 * 8]
    mov rdi, [r13 + r14 * 8 + 8]
    call fc_streq
    test eax, eax
    jnz .same_file

    ; A path-based check is not authoritative: a concurrent rename could replace
    ; the destination after the check. Compare the opened objects below.
    mov rdi, [r13 + r14 * 8]
    mov esi, FC_OPEN_READ
    xor edx, edx
    mov rcx, fc_cmd_cp_source_handle
    call os_open
    test rax, rax
    jnz .operation_error

    ; Read the first block before opening/truncating the destination. This
    ; rejects unreadable sources and directories without destroying a target.
    mov rdi, [fc_cmd_cp_source_handle]
    mov rsi, fc_cmd_cp_buffer
    mov edx, 4096
    mov rcx, fc_cmd_cp_read_count
    call os_read
    test rax, rax
    jnz .close_source_error

    mov rdi, [r13 + r14 * 8 + 8]
    mov esi, FC_OPEN_WRITE or FC_OPEN_CREATE
    mov edx, 0644o
    mov rcx, fc_cmd_cp_destination_handle
    call os_open
    test rax, rax
    jnz .close_source_error

    mov r15, [fc_cmd_cp_source_handle]
    mov rbx, [fc_cmd_cp_destination_handle]
    mov rdi, r15
    mov rsi, rbx
    call os_same_handle
    cmp eax, 1
    je .same_open_file
    test eax, eax
    js .open_identity_error

    ; Truncate only the destination handle whose identity was just checked.
    mov rdi, rbx
    call os_truncate
    test rax, rax
    jnz .copy_failed

    ; The prefetched block is already consumed from the source handle.
    mov rdx, [fc_cmd_cp_read_count]
    test rdx, rdx
    jz .copy_loop
    mov rdi, rbx
    mov rsi, fc_cmd_cp_buffer
    mov rcx, os_write
    call fc_write_all
    test rax, rax
    jnz .copy_failed
.copy_loop:
    mov rdi, r15
    mov rsi, fc_cmd_cp_buffer
    mov edx, 4096
    mov rcx, fc_cmd_cp_read_count
    call os_read
    test rax, rax
    jnz .copy_failed
    mov rdx, [fc_cmd_cp_read_count]
    test rdx, rdx
    jz .copy_complete
    mov rdi, rbx
    mov rsi, fc_cmd_cp_buffer
    mov rcx, os_write
    call fc_write_all
    test rax, rax
    jnz .copy_failed
    jmp .copy_loop

.copy_complete:
    mov rdi, rbx
    call os_close
    test rax, rax
    jnz .close_source_error
    mov rdi, r15
    call os_close
    test rax, rax
    jnz .operation_error
    xor eax, eax
    jmp .return

.copy_failed:
    mov rdi, rbx
    call os_close
    mov rdi, r15
    call os_close
    jmp .operation_error

.open_identity_error:
    mov rdi, rbx
    call os_close
    mov rdi, r15
    call os_close
    jmp .operation_error

.same_open_file:
    mov rdi, rbx
    call os_close
    mov rdi, r15
    call os_close
    jmp .same_file

.close_source_error:
    mov rdi, [fc_cmd_cp_source_handle]
    call os_close
    jmp .operation_error
.help:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_cp_help_text
    mov edx, fc_cmd_cp_help_length
    call fc_cmd_cp_write
    xor eax, eax
    jmp .return
.version:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_cp_version_text
    mov edx, fc_cmd_cp_version_length
    call fc_cmd_cp_write
    xor eax, eax
    jmp .return
.operation_error:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_cp_error_text
    mov edx, fc_cmd_cp_error_length
    call fc_cmd_cp_write
    mov eax, 1
    jmp .return
.same_file:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_cp_same_text
    mov edx, fc_cmd_cp_same_length
    call fc_cmd_cp_write
    mov eax, 1
    jmp .return
.bad_option:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_cp_bad_option_text
    mov edx, fc_cmd_cp_bad_option_length
    call fc_cmd_cp_write
    mov eax, 2
    jmp .return
.usage:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_cp_usage_text
    mov edx, fc_cmd_cp_usage_length
    call fc_cmd_cp_write
    mov eax, 2
    jmp .return
.return:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
