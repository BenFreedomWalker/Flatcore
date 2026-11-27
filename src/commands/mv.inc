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
    sub rsp, 56                 ; Win64 shadow space + aligned local storage
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

    mov rax, [r13 + r14 * 8]
    mov [rsp + 40], rax         ; source path
    mov rax, [r13 + r14 * 8 + 8]
    mov [rsp + 48], rax         ; destination path

    ; If DEST is an existing directory, append SOURCE's basename to it.
    mov rdi, [rsp + 48]
    mov esi, FC_OPEN_READ or FC_OPEN_DIRECTORY
    xor edx, edx
    lea rcx, [rsp + 32]
    call os_open
    test rax, rax
    jnz .rename_direct
    mov rdi, [rsp + 32]
    call os_close

    ; Find SOURCE's basename, ignoring trailing path separators.
    mov rdi, [rsp + 40]
    call fc_strlen
    mov r9, rax
.source_trim:
    test r9, r9
    jz .rename_direct
    mov r10, [rsp + 40]
    mov al, [r10 + r9 - 1]
    cmp al, '/'
    je .source_trim_one
    cmp al, '\'
    jne .source_trimmed
.source_trim_one:
    dec r9
    jmp .source_trim
.source_trimmed:
    mov r10, r9
.source_scan:
    test r10, r10
    jz .source_base_start
    dec r10
    mov rax, [rsp + 40]
    mov dl, [rax + r10]
    cmp dl, '/'
    je .source_base_found
    cmp dl, '\'
    jne .source_scan
.source_base_found:
    lea r11, [rax + r10 + 1]
    mov rdx, r9
    sub rdx, r10
    dec rdx
    jmp .destination_measure
.source_base_start:
    mov r11, [rsp + 40]
    mov rdx, r9

.destination_measure:
    ; Measure DEST and preserve filesystem roots while trimming separators.
    mov rdi, [rsp + 48]
    call fc_strlen
    mov r8, rax
    cmp r8, 3
    jne .destination_trim
    mov rax, [rsp + 48]
    cmp byte [rax + 1], ':'
    jne .destination_trim
    mov cl, [rax + 2]
    cmp cl, '/'
    je .destination_trim_done
    cmp cl, '\'
    je .destination_trim_done
.destination_trim:
    cmp r8, 1
    jbe .destination_trim_done
    mov rax, [rsp + 48]
    mov cl, [rax + r8 - 1]
    cmp cl, '/'
    je .destination_trim_one
    cmp cl, '\'
    jne .destination_trim_done
.destination_trim_one:
    dec r8
    jmp .destination_trim
.destination_trim_done:
    ; Check target-path capacity, including separator and terminating NUL.
    mov r9, r8
    test r9, r9
    jz .rename_direct
    mov rax, [rsp + 48]
    mov cl, [rax + r9 - 1]
    cmp cl, '/'
    je .destination_separator_ready
    cmp cl, '\'
    je .destination_separator_ready
    inc r9
.destination_separator_ready:
    lea rax, [r9 + rdx + 1]
    cmp rax, 65536
    ja .rename_direct

    ; Copy the destination prefix.
    mov rsi, [rsp + 48]
    lea rdi, [fc_cmd_mv_target_path]
    mov rcx, r8
.destination_copy:
    test rcx, rcx
    jz .destination_copy_done
    mov al, [rsi]
    mov [rdi], al
    inc rsi
    inc rdi
    dec rcx
    jmp .destination_copy
.destination_copy_done:
    ; Add a separator only when DEST does not already end in one.
    mov r9, r8
    mov al, [fc_cmd_mv_target_path + r9 - 1]
    cmp al, '/'
    je .append_basename
    cmp al, '\'
    je .append_basename
    mov byte [fc_cmd_mv_target_path + r9], '/'
    inc r9
.append_basename:
    mov rcx, rdx
    lea rdi, [fc_cmd_mv_target_path + r9]
.copy_basename:
    test rcx, rcx
    jz .finish_target_path
    mov al, [r11]
    mov [rdi], al
    inc r11
    inc rdi
    dec rcx
    jmp .copy_basename
.finish_target_path:
    mov byte [rdi], 0
    mov rdi, [rsp + 40]
    lea rsi, [fc_cmd_mv_target_path]
    call os_rename
    jmp .rename_result

.rename_direct:
    mov rdi, [rsp + 40]
    mov rsi, [rsp + 48]
    call os_rename
.rename_result:
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
    add rsp, 56
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
