; Shared FC64 mkdir implementation.
; mkdir [-pv] [-m MODE] [--] DIRECTORY...
; MODE is an octal POSIX mode. Windows accepts and parses it but its backend
; does not expose POSIX creation modes, so the mode argument is ignored there.

; fc_cmd_mkdir_write(handle, buffer, length) -> RAX=0/-1.
fc_cmd_mkdir_write:
    mov rcx, os_write
    jmp fc_write_all

; fc_cmd_mkdir_write_string(handle, string) -> RAX=0/-1.
fc_cmd_mkdir_write_string:
    push rbx
    mov rbx, rdi
    mov rdi, rsi
    call fc_strlen
    mov rdx, rax
    mov rsi, rdi
    mov rdi, rbx
    call fc_cmd_mkdir_write
    pop rbx
    ret

; Parse an octal mode string. RDI=string; RAX=mode, RDX=1 if valid.
fc_cmd_mkdir_parse_mode:
    xor eax, eax
    xor ecx, ecx
.mode_loop:
    movzx edx, byte [rdi + rcx]
    test dl, dl
    jz .mode_done
    cmp dl, '0'
    jb .mode_invalid
    cmp dl, '7'
    ja .mode_invalid
    shl eax, 3
    sub edx, '0'
    or eax, edx
    inc ecx
    cmp ecx, 4
    ja .mode_invalid
    jmp .mode_loop
.mode_done:
    test ecx, ecx
    jz .mode_invalid
    cmp eax, 07777o
    ja .mode_invalid
    mov edx, 1
    ret
.mode_invalid:
    xor edx, edx
    mov eax, -1
    ret

; Emit the verbose creation line for the path in RDI.
fc_cmd_mkdir_verbose:
    push rbx
    push r12
    sub rsp, 8
    mov r12, rdi
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_mkdir_verbose_prefix
    mov edx, fc_cmd_mkdir_verbose_prefix_length
    call fc_cmd_mkdir_write
    test rax, rax
    jnz .verbose_done
    mov rdi, r12
    call fc_strlen
    mov rbx, rax
    mov rdi, [os_stdout]
    mov rsi, r12
    mov rdx, rbx
    call fc_cmd_mkdir_write
    test rax, rax
    jnz .verbose_done
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_mkdir_quote_newline
    mov edx, 2
    call fc_cmd_mkdir_write
.verbose_done:
    add rsp, 8
    pop r12
    pop rbx
    ret

; Ensure one directory exists. RDI=path, RSI=mode, RAX=0/-1.
; Existing directories are accepted; existing non-directories are rejected.
fc_cmd_mkdir_ensure:
    push rbx
    push r12
    push r13
    mov r12, rdi
    mov r13, rsi
    mov rdi, r12
    mov rsi, fc_cmd_mkdir_info
    call os_stat
    test rax, rax
    jnz .ensure_create
    cmp dword [fc_cmd_mkdir_info + 36], FC_TYPE_DIRECTORY
    jne .ensure_failed
    xor eax, eax
    jmp .ensure_done
.ensure_create:
    mov rdi, r12
    mov rsi, r13
    call os_mkdir
    test rax, rax
    jnz .ensure_race_check
    test qword [fc_cmd_mkdir_flags], 2
    jz .ensure_created
    mov rdi, r12
    call fc_cmd_mkdir_verbose
    test rax, rax
    jnz .ensure_failed
.ensure_created:
    xor eax, eax
    jmp .ensure_done
.ensure_race_check:
    ; Another process may have created the directory after our first stat.
    mov rdi, r12
    mov rsi, fc_cmd_mkdir_info
    call os_stat
    test rax, rax
    jnz .ensure_failed
    cmp dword [fc_cmd_mkdir_info + 36], FC_TYPE_DIRECTORY
    jne .ensure_failed
    xor eax, eax
    jmp .ensure_done
.ensure_failed:
    mov eax, -1
.ensure_done:
    pop r13
    pop r12
    pop rbx
    ret

; Create one operand, recursively when -p is active.
; RDI=path, RSI=mode, RAX=0/-1.
fc_cmd_mkdir_one:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov r13, rsi
    test qword [fc_cmd_mkdir_flags], 1
    jnz .one_parents

    mov rdi, r12
    mov rsi, r13
    call os_mkdir
    test rax, rax
    jnz .one_failed
    test qword [fc_cmd_mkdir_flags], 2
    jz .one_success
    mov rdi, r12
    call fc_cmd_mkdir_verbose
    jmp .one_done

.one_parents:
    mov rdi, r12
    call fc_strlen
    cmp rax, 8191
    ja .one_failed
    mov r14, rax
    mov rdi, fc_cmd_mkdir_work_path
    mov rsi, r12
    mov rcx, r14
    rep movsb
    mov byte [fc_cmd_mkdir_work_path + r14], 0

    ; Walk separators and create each existing prefix. Leading roots and
    ; drive roots are never treated as directories to create.
    mov r15, 0
    xor ebx, ebx
.path_scan:
    cmp rbx, r14
    jae .path_final
    mov al, [fc_cmd_mkdir_work_path + rbx]
    cmp al, '/'
    je .path_separator
    cmp al, '\\'
    jne .path_next
.path_separator:
    test rbx, rbx
    jz .path_next
    ; Preserve the drive root in paths such as C:\folder.
    cmp rbx, 2
    jne .path_unc_check
    cmp byte [fc_cmd_mkdir_work_path + 1], ':'
    je .path_next
.path_unc_check:
    ; For UNC paths, skip the server and share roots.
    cmp byte [fc_cmd_mkdir_work_path], '\\'
    jne .path_prefix
    cmp byte [fc_cmd_mkdir_work_path + 1], '\\'
    jne .path_prefix
    cmp r15, 2
    jb .path_count_unc
    jmp .path_prefix
.path_count_unc:
    inc r15
    jmp .path_next
.path_prefix:
    mov byte [fc_cmd_mkdir_work_path + rbx], 0
    mov rdi, fc_cmd_mkdir_work_path
    mov rsi, 0777o
    call fc_cmd_mkdir_ensure
    mov rdx, rax
    ; Restore the separator from the original operand.
    mov al, [r12 + rbx]
    mov [fc_cmd_mkdir_work_path + rbx], al
    test rdx, rdx
    jnz .one_failed
.path_next:
    inc rbx
    jmp .path_scan

.path_final:
    mov rdi, fc_cmd_mkdir_work_path
    mov rsi, r13
    call fc_cmd_mkdir_ensure
    jmp .one_done

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

; mkdir [-pv] [-m MODE] [--] DIRECTORY...
fc_cmd_mkdir:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov r13, rsi
    mov r14, 1
    mov qword [fc_cmd_mkdir_flags], 0
    mov qword [fc_cmd_mkdir_mode], 0777o
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
    mov rdi, fc_cmd_mkdir_opt_parents
    call fc_streq
    test eax, eax
    jnz .set_parents
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_mkdir_opt_verbose
    call fc_streq
    test eax, eax
    jnz .set_verbose
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_mkdir_opt_help
    call fc_streq
    test eax, eax
    jnz .help
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_mkdir_opt_version
    call fc_streq
    test eax, eax
    jnz .version
    mov rsi, [r13 + r14 * 8]
    mov rdx, rsi
    mov rdi, fc_cmd_mkdir_opt_mode_prefix
.mode_prefix_compare:
    mov al, [rdi]
    test al, al
    jz .mode_equals
    cmp al, [rdx]
    jne .bad_option
    inc rdi
    inc rdx
    jmp .mode_prefix_compare

.set_parents:
    or qword [fc_cmd_mkdir_flags], 1
    inc r14
    jmp .parse
.set_verbose:
    or qword [fc_cmd_mkdir_flags], 2
    inc r14
    jmp .parse
.mode_equals:
    mov rsi, [r13 + r14 * 8]
    add rsi, 7
    jmp .parse_mode
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
    cmp al, 'm'
    je .short_mode
    jmp .bad_option
.short_parents:
    or qword [fc_cmd_mkdir_flags], 1
    inc rsi
    jmp .short_loop
.short_verbose:
    or qword [fc_cmd_mkdir_flags], 2
    inc rsi
    jmp .short_loop
.short_mode:
    inc rsi
    cmp byte [rsi], 0
    jne .parse_mode
    inc r14
    cmp r14, r12
    jae .usage
    mov rsi, [r13 + r14 * 8]
.parse_mode:
    mov rdi, rsi
    call fc_cmd_mkdir_parse_mode
    test rdx, rdx
    jz .bad_mode
    mov [fc_cmd_mkdir_mode], rax
    inc r14
    jmp .parse

.help:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_mkdir_help_text
    mov edx, fc_cmd_mkdir_help_length
    call fc_cmd_mkdir_write
    xor eax, eax
    jmp .return
.version:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_mkdir_version_text
    mov edx, fc_cmd_mkdir_version_length
    call fc_cmd_mkdir_write
    xor eax, eax
    jmp .return
.bad_option:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_mkdir_bad_option_text
    mov edx, fc_cmd_mkdir_bad_option_length
    call fc_cmd_mkdir_write
    mov eax, 2
    jmp .return
.bad_mode:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_mkdir_bad_mode_text
    mov edx, fc_cmd_mkdir_bad_mode_length
    call fc_cmd_mkdir_write
    mov eax, 2
    jmp .return
.usage:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_mkdir_usage_text
    mov edx, fc_cmd_mkdir_usage_length
    call fc_cmd_mkdir_write
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
    mov rsi, [fc_cmd_mkdir_mode]
    call fc_cmd_mkdir_one
    test eax, eax
    jz .operand_next
    mov ebx, 1
    ; Keep processing independent operands after a failure.
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_mkdir_error_prefix
    mov edx, fc_cmd_mkdir_error_prefix_length
    call fc_cmd_mkdir_write
    mov rdi, [r13 + r14 * 8]
    call fc_strlen
    mov rdx, rax
    mov rdi, [os_stderr]
    mov rsi, [r13 + r14 * 8]
    call fc_cmd_mkdir_write
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_mkdir_error_suffix
    mov edx, fc_cmd_mkdir_error_suffix_length
    call fc_cmd_mkdir_write
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
