; Shared FC64 rmdir implementation.
; rmdir [-pv] [--] DIRECTORY...
; -p removes the operand and then its empty parent directories.
; -v reports each directory successfully removed.

fc_cmd_rmdir_write:
    mov rcx, os_write
    jmp fc_write_all

; Print "rmdir: removed 'PATH'\n". RDI=path.
fc_cmd_rmdir_verbose:
    push rbx
    push r12
    sub rsp, 8
    mov r12, rdi
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_rmdir_verbose_prefix
    mov edx, fc_cmd_rmdir_verbose_prefix_length
    call fc_cmd_rmdir_write
    test rax, rax
    jnz .verbose_done
    mov rdi, r12
    call fc_strlen
    mov rbx, rax
    mov rdi, [os_stdout]
    mov rsi, r12
    mov rdx, rbx
    call fc_cmd_rmdir_write
    test rax, rax
    jnz .verbose_done
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_rmdir_quote_newline
    mov edx, 2
    call fc_cmd_rmdir_write
.verbose_done:
    add rsp, 8
    pop r12
    pop rbx
    ret

; Remove a single directory, then its parents when -p is enabled.
; RDI=operand path; RAX=0 success/-1 failure.
fc_cmd_rmdir_one:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov rdi, r12
    call fc_strlen
    cmp rax, 8191
    ja .one_failed
    mov r13, rax
    test r13, r13
    jz .one_failed
    mov rdi, fc_cmd_rmdir_work_path
    mov rsi, r12
    mov rcx, r13
    rep movsb
    mov byte [fc_cmd_rmdir_work_path + r13], 0

    ; Trim trailing separators without destroying POSIX or drive roots.
.trim_trailing:
    cmp r13, 1
    jbe .remove_current
    mov al, [fc_cmd_rmdir_work_path + r13 - 1]
    cmp al, '/'
    je .trim_one
    cmp al, '\\'
    jne .remove_current
.trim_one:
    ; Keep "/" and "C:\\" intact.
    cmp r13, 1
    je .remove_current
    cmp r13, 3
    jne .trim_store
    cmp byte [fc_cmd_rmdir_work_path + 1], ':'
    je .remove_current
.trim_store:
    dec r13
    mov byte [fc_cmd_rmdir_work_path + r13], 0
    jmp .trim_trailing

.remove_current:
    mov rdi, fc_cmd_rmdir_work_path
    call os_rmdir
    test rax, rax
    jnz .one_failed
    test qword [fc_cmd_rmdir_flags], 2
    jz .parents_check
    mov rdi, fc_cmd_rmdir_work_path
    call fc_cmd_rmdir_verbose
    test rax, rax
    jnz .one_failed

.parents_check:
    test qword [fc_cmd_rmdir_flags], 1
    jz .one_success
    ; Locate the final separator and truncate to its parent.
    mov r14, r13
.parent_scan:
    test r14, r14
    jz .one_success
    dec r14
    mov al, [fc_cmd_rmdir_work_path + r14]
    cmp al, '/'
    je .parent_separator
    cmp al, '\\'
    jne .parent_scan
.parent_separator:
    ; Preserve a POSIX root or a Windows drive root.
    test r14, r14
    jz .parent_root
    cmp r14, 2
    jne .parent_truncate
    cmp byte [fc_cmd_rmdir_work_path + 1], ':'
    je .parent_drive_root
.parent_truncate:
    mov byte [fc_cmd_rmdir_work_path + r14], 0
    mov r13, r14
    ; Do not attempt to remove a filesystem root or the caller's . / ...
    cmp r13, 1
    jne .parent_check_dotdot
    mov al, [fc_cmd_rmdir_work_path]
    cmp al, '/'
    je .one_success
    cmp al, '\\'
    je .one_success
    cmp al, '.'
    je .one_success
    jmp .remove_parent
.parent_check_dotdot:
    cmp r13, 2
    jne .remove_parent
    cmp word [fc_cmd_rmdir_work_path], '..'
    je .one_success
    jmp .remove_parent
.parent_root:
    jmp .one_success
.parent_drive_root:
    jmp .one_success
.remove_parent:
    mov rdi, fc_cmd_rmdir_work_path
    call os_rmdir
    test rax, rax
    jnz .one_failed
    test qword [fc_cmd_rmdir_flags], 2
    jz .parents_check
    mov rdi, fc_cmd_rmdir_work_path
    call fc_cmd_rmdir_verbose
    test rax, rax
    jnz .one_failed
    ; A root itself is never removed by -p.
    cmp r13, 1
    je .one_success
    cmp r13, 3
    je .one_success
    jmp .parents_check

.one_success:
    xor eax, eax
    jmp .one_done
.one_failed:
    mov eax, -1
.one_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; rmdir [-pv] [--] DIRECTORY...
fc_cmd_rmdir:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov r13, rsi
    mov r14, 1
    mov qword [fc_cmd_rmdir_flags], 0
    xor r15d, r15d

.parse:
    cmp r14, r12
    jae .parsed
    mov rsi, [r13 + r14 * 8]
    test r15d, r15d
    jnz .parsed
    cmp byte [rsi], '-'
    jne .parsed
    cmp byte [rsi + 1], 0
    je .parsed
    cmp byte [rsi + 1], '-'
    jne .short_options
    cmp byte [rsi + 2], 0
    je .end_options
    mov rdi, fc_cmd_rmdir_opt_parents
    call fc_streq
    test eax, eax
    jnz .set_parents
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_rmdir_opt_verbose
    call fc_streq
    test eax, eax
    jnz .set_verbose
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_rmdir_opt_help
    call fc_streq
    test eax, eax
    jnz .help
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_rmdir_opt_version
    call fc_streq
    test eax, eax
    jnz .version
    jmp .bad_option

.set_parents:
    or qword [fc_cmd_rmdir_flags], 1
    inc r14
    jmp .parse
.set_verbose:
    or qword [fc_cmd_rmdir_flags], 2
    inc r14
    jmp .parse
.end_options:
    mov r15d, 1
    inc r14
    jmp .parse

.short_options:
    inc rsi
.short_loop:
    mov al, [rsi]
    test al, al
    jz .short_done
    cmp al, 'p'
    je .short_parents
    cmp al, 'v'
    je .short_verbose
    jmp .bad_option
.short_parents:
    or qword [fc_cmd_rmdir_flags], 1
    inc rsi
    jmp .short_loop
.short_verbose:
    or qword [fc_cmd_rmdir_flags], 2
    inc rsi
    jmp .short_loop
.short_done:
    inc r14
    jmp .parse

.help:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_rmdir_help_text
    mov edx, fc_cmd_rmdir_help_length
    call fc_cmd_rmdir_write
    xor eax, eax
    jmp .return
.version:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_rmdir_version_text
    mov edx, fc_cmd_rmdir_version_length
    call fc_cmd_rmdir_write
    xor eax, eax
    jmp .return
.bad_option:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_rmdir_bad_option_text
    mov edx, fc_cmd_rmdir_bad_option_length
    call fc_cmd_rmdir_write
    mov eax, 2
    jmp .return
.usage:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_rmdir_usage_text
    mov edx, fc_cmd_rmdir_usage_length
    call fc_cmd_rmdir_write
    mov eax, 2
    jmp .return

.parsed:
    cmp r14, r12
    jae .usage
    xor ebx, ebx
.operand_loop:
    cmp r14, r12
    jae .operands_done
    mov rdi, [r13 + r14 * 8]
    call fc_cmd_rmdir_one
    test eax, eax
    jz .operand_next
    mov ebx, 1
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_rmdir_error_prefix
    mov edx, fc_cmd_rmdir_error_prefix_length
    call fc_cmd_rmdir_write
    mov rdi, [r13 + r14 * 8]
    call fc_strlen
    mov rdx, rax
    mov rdi, [os_stderr]
    mov rsi, [r13 + r14 * 8]
    call fc_cmd_rmdir_write
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_rmdir_error_suffix
    mov edx, fc_cmd_rmdir_error_suffix_length
    call fc_cmd_rmdir_write
.operand_next:
    inc r14
    jmp .operand_loop
.operands_done:
    mov eax, ebx
.return:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
