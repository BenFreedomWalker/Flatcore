; Shared path commands: pwd, basename, dirname.
; Path syntax is intentionally platform-neutral: both slash characters are
; treated as separators so the same argv has the same result on both targets.

; fc_cmd_path_write(buffer, length) -> RAX=0 success, 1 output failure.
fc_cmd_path_write:
    push rbx
    push r12
    sub rsp, 8
    mov r12, rdi
    mov rbx, rsi
    mov rdi, [os_stdout]
    mov rsi, r12
    mov rdx, rbx
    mov rcx, os_write
    call fc_write_all
    test eax, eax
    jnz .write_failed
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_path_newline
    mov edx, 1
    mov rcx, os_write
    call fc_write_all
    test eax, eax
    jnz .write_failed
    xor eax, eax
    jmp .write_done
.write_failed:
    mov eax, 1
.write_done:
    add rsp, 8
    pop r12
    pop rbx
    ret

; fc_cmd_path_usage(message) -> RAX=2 after writing a usage diagnostic.
fc_cmd_path_usage:
    push rbx
    push r12
    sub rsp, 8
    mov r12, rdi
    mov rdi, r12
    call fc_strlen
    mov rdx, rax
    mov rdi, [os_stderr]
    mov rsi, r12
    mov rcx, os_write
    call fc_write_all
    mov eax, 2
    add rsp, 8
    pop r12
    pop rbx
    ret

; pwd
fc_cmd_pwd:
    cmp rdi, 1
    jne .pwd_usage
    push rbx
    push r12
    sub rsp, 8
    mov rdi, fc_cmd_path_buffer
    mov esi, 4096
    mov rdx, fc_cmd_path_length
    call os_getcwd
    test eax, eax
    jnz .pwd_error
    mov rdi, fc_cmd_path_buffer
    mov rsi, [fc_cmd_path_length]
    call fc_cmd_path_write
    jmp .pwd_done
.pwd_error:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_pwd_error
    mov edx, fc_cmd_pwd_error_length
    mov rcx, os_write
    call fc_write_all
    mov eax, 1
.pwd_done:
    add rsp, 8
    pop r12
    pop rbx
    ret
.pwd_usage:
    mov rdi, fc_cmd_pwd_usage_text
    jmp fc_cmd_path_usage

; basename PATH
fc_cmd_basename:
    cmp rdi, 2
    jne .basename_usage
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rsi
    mov rbx, [r12 + 8]
    mov rdi, rbx
    call fc_strlen
    mov r13, rax

    ; Remove trailing separators, but preserve POSIX and drive roots.
.basename_trim:
    cmp r13, 1
    jbe .basename_root_check
    cmp r13, 3
    jne .basename_trim_test
    cmp byte [rbx + 1], ':'
    jne .basename_trim_test
    mov al, [rbx + 2]
    cmp al, '/'
    je .basename_root_check
    cmp al, '\'
    je .basename_root_check
.basename_trim_test:
    mov al, [rbx + r13 - 1]
    cmp al, '/'
    je .basename_trim_one
    cmp al, '\'
    jne .basename_find
.basename_trim_one:
    dec r13
    jmp .basename_trim

.basename_root_check:
    cmp r13, 1
    jne .basename_drive_root
    mov al, [rbx]
    cmp al, '/'
    je .basename_root
    cmp al, '\'
    je .basename_root
.basename_drive_root:
    cmp r13, 3
    jne .basename_find
    cmp byte [rbx + 1], ':'
    jne .basename_find
    mov al, [rbx + 2]
    cmp al, '/'
    je .basename_drive_separator
    cmp al, '\'
    jne .basename_find
.basename_drive_separator:
    lea rdi, [rbx + 2]
    mov esi, 1
    call fc_cmd_path_write
    jmp .basename_done
.basename_root:
    mov rdi, rbx
    mov esi, 1
    call fc_cmd_path_write
    jmp .basename_done

.basename_find:
    mov r14, r13
.basename_scan:
    test r14, r14
    jz .basename_start
    dec r14
    mov al, [rbx + r14]
    cmp al, '/'
    je .basename_found
    cmp al, '\'
    jne .basename_scan
.basename_found:
    lea r15, [r14 + 1]
    jmp .basename_output
.basename_start:
    xor r15d, r15d
.basename_output:
    lea rdi, [rbx + r15]
    mov rsi, r13
    sub rsi, r15
    call fc_cmd_path_write
.basename_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
.basename_usage:
    mov rdi, fc_cmd_basename_usage_text
    jmp fc_cmd_path_usage

; dirname PATH
fc_cmd_dirname:
    cmp rdi, 2
    jne .dirname_usage
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rsi
    mov rbx, [r12 + 8]
    mov rdi, rbx
    call fc_strlen
    mov r13, rax

.dirname_trim:
    cmp r13, 1
    jbe .dirname_root_check
    cmp r13, 3
    jne .dirname_trim_test
    cmp byte [rbx + 1], ':'
    jne .dirname_trim_test
    mov al, [rbx + 2]
    cmp al, '/'
    je .dirname_root_check
    cmp al, '\'
    je .dirname_root_check
.dirname_trim_test:
    mov al, [rbx + r13 - 1]
    cmp al, '/'
    je .dirname_trim_one
    cmp al, '\'
    jne .dirname_scan_start
.dirname_trim_one:
    dec r13
    jmp .dirname_trim

.dirname_root_check:
    cmp r13, 1
    jne .dirname_drive_root
    mov al, [rbx]
    cmp al, '/'
    je .dirname_root
    cmp al, '\'
    je .dirname_root
.dirname_drive_root:
    cmp r13, 3
    jne .dirname_scan_start
    cmp byte [rbx + 1], ':'
    jne .dirname_scan_start
    mov al, [rbx + 2]
    cmp al, '/'
    je .dirname_root
    cmp al, '\'
    je .dirname_root

.dirname_scan_start:
    mov r14, r13
.dirname_scan:
    test r14, r14
    jz .dirname_no_separator
    dec r14
    mov al, [rbx + r14]
    cmp al, '/'
    je .dirname_separator
    cmp al, '\'
    jne .dirname_scan
.dirname_separator:
    test r14, r14
    jz .dirname_root
    ; Preserve "X:\" as the parent of "X:\name".
    cmp r14, 2
    jne .dirname_prefix
    cmp byte [rbx + 1], ':'
    jne .dirname_prefix
    lea rdi, [rbx]
    mov esi, 3
    call fc_cmd_path_write
    jmp .dirname_done
.dirname_prefix:
    mov rdi, rbx
    mov rsi, r14
    call fc_cmd_path_write
    jmp .dirname_done

.dirname_no_separator:
    ; Drive-relative paths such as C:FILE have C: as their directory.
    cmp r13, 2
    jb .dirname_dot
    cmp byte [rbx + 1], ':'
    jne .dirname_dot
    mov rdi, rbx
    mov esi, 2
    call fc_cmd_path_write
    jmp .dirname_done
.dirname_dot:
    mov rdi, fc_cmd_dot
    mov esi, 1
    call fc_cmd_path_write
    jmp .dirname_done
.dirname_root:
    mov rdi, rbx
    mov esi, 1
    call fc_cmd_path_write
.dirname_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
.dirname_usage:
    mov rdi, fc_cmd_dirname_usage_text
    jmp fc_cmd_path_usage
