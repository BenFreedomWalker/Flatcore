; Shared FC64 rm implementation.
; rm [-dfv] [--] FILE...
; -f ignores missing operands, -d permits removal of empty directories,
; -v reports each successful removal. Recursive removal is deliberately
; not enabled until the shared traversal/deletion contract is in place.

fc_cmd_rm_write:
    mov rcx, os_write
    jmp fc_write_all

; Print "rm: removed 'PATH'\n". RDI=path.
fc_cmd_rm_verbose:
    push rbx
    push r12
    sub rsp, 8
    mov r12, rdi
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_rm_verbose_prefix
    mov edx, fc_cmd_rm_verbose_prefix_length
    call fc_cmd_rm_write
    test rax, rax
    jnz .verbose_done
    mov rdi, r12
    call fc_strlen
    mov rbx, rax
    mov rdi, [os_stdout]
    mov rsi, r12
    mov rdx, rbx
    call fc_cmd_rm_write
    test rax, rax
    jnz .verbose_done
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_rm_quote_newline
    mov edx, 2
    call fc_cmd_rm_write
.verbose_done:
    add rsp, 8
    pop r12
    pop rbx
    ret

; Remove one file, symbolic link, or (with -d) empty directory.
; RDI=path; RAX=0 success/-1 failure.
fc_cmd_rm_one:
    push rbx
    push r12
    sub rsp, 8
    mov r12, rdi
    mov rdi, r12
    mov rsi, fc_cmd_rm_info
    call os_stat
    test rax, rax
    jnz .stat_failed

    cmp dword [fc_cmd_rm_info + 36], FC_TYPE_DIRECTORY
    jne .remove_file
    test qword [fc_cmd_rm_flags], 2
    jz .one_failed
    mov rdi, r12
    call os_rmdir
    test rax, rax
    jnz .one_failed
    jmp .removed

.remove_file:
    mov rdi, r12
    call os_unlink
    test rax, rax
    jnz .one_failed

.removed:
    test qword [fc_cmd_rm_flags], 4
    jz .one_success
    mov rdi, r12
    call fc_cmd_rm_verbose
    test rax, rax
    jnz .one_failed
.one_success:
    xor eax, eax
    jmp .one_done

.stat_failed:
    test qword [fc_cmd_rm_flags], 1
    jz .one_failed
    cmp dword [os_error_code], FC_ERR_NOT_FOUND
    jne .one_failed
    xor eax, eax
    jmp .one_done
.one_failed:
    mov eax, -1
.one_done:
    add rsp, 8
    pop r12
    pop rbx
    ret

; rm [-dfv] [--] FILE...
fc_cmd_rm:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov r13, rsi
    mov r14, 1
    mov qword [fc_cmd_rm_flags], 0
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
    mov rdi, fc_cmd_rm_opt_force
    call fc_streq
    test eax, eax
    jnz .set_force
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_rm_opt_dir
    call fc_streq
    test eax, eax
    jnz .set_dir
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_rm_opt_verbose
    call fc_streq
    test eax, eax
    jnz .set_verbose
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_rm_opt_help
    call fc_streq
    test eax, eax
    jnz .help
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_rm_opt_version
    call fc_streq
    test eax, eax
    jnz .version
    jmp .bad_option

.set_force:
    or qword [fc_cmd_rm_flags], 1
    inc r14
    jmp .parse
.set_dir:
    or qword [fc_cmd_rm_flags], 2
    inc r14
    jmp .parse
.set_verbose:
    or qword [fc_cmd_rm_flags], 4
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
    cmp al, 'f'
    je .short_force
    cmp al, 'd'
    je .short_dir
    cmp al, 'v'
    je .short_verbose
    jmp .bad_option
.short_force:
    or qword [fc_cmd_rm_flags], 1
    inc rsi
    jmp .short_loop
.short_dir:
    or qword [fc_cmd_rm_flags], 2
    inc rsi
    jmp .short_loop
.short_verbose:
    or qword [fc_cmd_rm_flags], 4
    inc rsi
    jmp .short_loop
.short_done:
    inc r14
    jmp .parse

.help:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_rm_help_text
    mov edx, fc_cmd_rm_help_length
    call fc_cmd_rm_write
    xor eax, eax
    jmp .return
.version:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_rm_version_text
    mov edx, fc_cmd_rm_version_length
    call fc_cmd_rm_write
    xor eax, eax
    jmp .return
.bad_option:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_rm_bad_option_text
    mov edx, fc_cmd_rm_bad_option_length
    call fc_cmd_rm_write
    mov eax, 2
    jmp .return
.usage:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_rm_usage_text
    mov edx, fc_cmd_rm_usage_length
    call fc_cmd_rm_write
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
    call fc_cmd_rm_one
    test eax, eax
    jz .operand_next
    mov ebx, 1
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_rm_error_prefix
    mov edx, fc_cmd_rm_error_prefix_length
    call fc_cmd_rm_write
    mov rdi, [r13 + r14 * 8]
    call fc_strlen
    mov rdx, rax
    mov rdi, [os_stderr]
    mov rsi, [r13 + r14 * 8]
    call fc_cmd_rm_write
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_rm_error_suffix
    mov edx, fc_cmd_rm_error_suffix_length
    call fc_cmd_rm_write
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
