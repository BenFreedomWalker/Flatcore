; Additional Flatcore core utilities.
; All commands use the shared Linux x86-64 syscall runtime.

; basename PATH
command_basename:
    mov r12, [argc]
    mov r13, [argv]
    lea rax, [r14 + 1]
    cmp r12, rax
    jne command_usage
    mov rbx, [r13 + r14 * 8]
    xor r15d, r15d
.basename_length:
    cmp byte [rbx + r15], 0
    je .basename_trim
    inc r15
    jmp .basename_length
.basename_trim:
    cmp r15, 1
    jbe .basename_find
    cmp byte [rbx + r15 - 1], '/'
    jne .basename_find
    dec r15
    jmp .basename_trim
.basename_find:
    xor r14d, r14d
    mov rax, r15
.basename_find_loop:
    test rax, rax
    jz .basename_write
    dec rax
    cmp byte [rbx + rax], '/'
    jne .basename_find_loop
    lea r14, [rax + 1]
.basename_write:
    test r15, r15
    jnz .basename_nonempty
    mov rsi, newline_text
    call write_out
    xor edi, edi
    jmp process_exit
.basename_nonempty:
    mov rsi, rbx
    add rsi, r14
    mov rdx, r15
    sub rdx, r14
    test rdx, rdx
    jnz .basename_output
    mov rsi, slash_text
    mov edx, 1
.basename_output:
    mov edi, 1
    call write_all
    mov rsi, newline_text
    call write_out
    xor edi, edi
    jmp process_exit

; dirname PATH
command_dirname:
    mov r12, [argc]
    mov r13, [argv]
    lea rax, [r14 + 1]
    cmp r12, rax
    jne command_usage
    mov rbx, [r13 + r14 * 8]
    xor r15d, r15d
.dirname_length:
    cmp byte [rbx + r15], 0
    je .dirname_trim
    inc r15
    jmp .dirname_length
.dirname_trim:
    cmp r15, 1
    jbe .dirname_find
    cmp byte [rbx + r15 - 1], '/'
    jne .dirname_find
    dec r15
    jmp .dirname_trim
.dirname_find:
    cmp r15, 1
    jne .dirname_find_slash
    cmp byte [rbx], '/'
    je .dirname_root
.dirname_find_slash:
    mov rax, r15
.dirname_find_loop:
    test rax, rax
    jz .dirname_dot
    dec rax
    cmp byte [rbx + rax], '/'
    jne .dirname_find_loop
    test rax, rax
    jz .dirname_root
    mov rdx, rax
    mov rsi, rbx
    mov edi, 1
    call write_all
    jmp .dirname_newline
.dirname_dot:
    mov rsi, dot_text
    call write_out
    jmp .dirname_newline
.dirname_root:
    mov rsi, slash_text
    call write_out
.dirname_newline:
    mov rsi, newline_text
    call write_out
    xor edi, edi
    jmp process_exit

; head [FILE] - write the first ten lines.
command_head:
    mov r12, [argc]
    mov r13, [argv]
    xor r15d, r15d
    xor r13d, r13d
    cmp r12, r14
    je .head_stdin
    lea rax, [r14 + 1]
    cmp r12, rax
    jne command_usage
    mov rax, [argv]
    mov rdi, [rax + r14 * 8]
    xor esi, esi
    xor edx, edx
    mov eax, 2                  ; SYS_open
    syscall
    test rax, rax
    js .head_error
    mov r13, rax
    mov r15d, 1
    jmp .head_read
.head_stdin:
    xor r13d, r13d
.head_read:
    xor r12d, r12d
.head_read_loop:
    xor eax, eax                ; SYS_read
    mov edi, r13d
    mov rsi, io_buffer
    mov edx, 4096
    syscall
    test rax, rax
    jz .head_done
    js .head_error_close
    mov rbx, rax
    xor r14d, r14d
.head_scan:
    cmp r14, rbx
    jae .head_write_chunk
    cmp byte [io_buffer + r14], 10
    jne .head_scan_next
    inc r12
    inc r14
    cmp r12, 10
    je .head_write_limit
    jmp .head_scan
.head_scan_next:
    inc r14
    jmp .head_scan
.head_write_chunk:
    mov rsi, io_buffer
    mov rdx, rbx
    mov edi, 1
    call write_all
    jmp .head_read_loop
.head_write_limit:
    mov rsi, io_buffer
    mov rdx, r14
    mov edi, 1
    call write_all
    jmp .head_done
.head_done:
    test r15, r15
    jz .head_exit
    mov eax, 3                  ; SYS_close
    mov edi, r13d
    syscall
.head_exit:
    xor edi, edi
    jmp process_exit
.head_error_close:
    test r15, r15
    jz .head_error
    mov eax, 3
    mov edi, r13d
    syscall
.head_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit

; wc [FILE] - print lines, words and bytes.
command_wc:
    mov r12, [argc]
    mov r13, [argv]
    xor r15d, r15d
    cmp r12, r14
    je .wc_stdin
    lea rax, [r14 + 1]
    cmp r12, rax
    jne command_usage
    mov rax, [argv]
    mov rdi, [rax + r14 * 8]
    xor esi, esi
    xor edx, edx
    mov eax, 2                  ; SYS_open
    syscall
    test rax, rax
    js .wc_error
    mov r15, rax
    jmp .wc_start
.wc_stdin:
    xor r15d, r15d
.wc_start:
    xor r12d, r12d              ; lines
    xor r13d, r13d              ; words
    xor r14d, r14d              ; bytes
    xor ebx, ebx                ; currently inside a word
.wc_read:
    xor eax, eax                ; SYS_read
    mov edi, r15d
    mov rsi, io_buffer
    mov edx, 4096
    syscall
    test rax, rax
    jz .wc_done
    js .wc_error_close
    add r14, rax
    mov r8, io_buffer
    mov r9, rax
.wc_scan:
    test r9, r9
    jz .wc_read
    mov al, [r8]
    cmp al, 10
    jne .wc_not_newline
    inc r12
.wc_not_newline:
    cmp al, 32
    je .wc_whitespace
    cmp al, 9
    je .wc_whitespace
    cmp al, 10
    je .wc_whitespace
    cmp al, 11
    je .wc_whitespace
    cmp al, 12
    je .wc_whitespace
    cmp al, 13
    je .wc_whitespace
    test ebx, ebx
    jnz .wc_advance
    inc r13
    mov ebx, 1
    jmp .wc_advance
.wc_whitespace:
    xor ebx, ebx
.wc_advance:
    inc r8
    dec r9
    jmp .wc_scan
.wc_done:
    test r15, r15
    jz .wc_print
    mov eax, 3                  ; SYS_close
    mov edi, r15d
    syscall
.wc_print:
    mov rax, r12
    call write_number
    mov rsi, space_text
    call write_out
    mov rax, r13
    call write_number
    mov rsi, space_text
    call write_out
    mov rax, r14
    call write_number
    mov rsi, newline_text
    call write_out
    xor edi, edi
    jmp process_exit
.wc_error_close:
    test r15, r15
    jz .wc_error
    mov eax, 3
    mov edi, r15d
    syscall
.wc_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit

; ln [-s] TARGET LINK_NAME
command_ln:
    mov r12, [argc]
    mov r13, [argv]
    mov r15, r14
    xor ebx, ebx
    mov rax, [r13 + r15 * 8]
    mov rsi, rax
    mov rdi, symlink_option
    call streq
    test eax, eax
    jz .ln_arguments
    inc r15
    mov ebx, 1
.ln_arguments:
    lea rax, [r15 + 2]
    cmp r12, rax
    jne command_usage
    mov rdi, [r13 + r15 * 8]
    mov rsi, [r13 + r15 * 8 + 8]
    test ebx, ebx
    jz .ln_hard
    mov eax, 88                 ; SYS_symlink
    syscall
    jmp .ln_result
.ln_hard:
    mov eax, 86                 ; SYS_link
    syscall
.ln_result:
    test rax, rax
    js .ln_error
    xor edi, edi
    jmp process_exit
.ln_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit

; readlink PATH
command_readlink:
    mov r12, [argc]
    lea rax, [r14 + 1]
    cmp r12, rax
    jne command_usage
    mov r13, [argv]
    mov rdi, [r13 + r14 * 8]
    mov rsi, io_buffer
    mov edx, 4095
    mov eax, 89                 ; SYS_readlink
    syscall
    test rax, rax
    js .readlink_error
    mov rdx, rax
    mov rsi, io_buffer
    mov edi, 1
    call write_all
    mov rsi, newline_text
    call write_out
    xor edi, edi
    jmp process_exit
.readlink_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit

; Convert unsigned RAX to decimal and write it to stdout.
write_number:
    push rbx
    lea rsi, [num_buffer + 32]
    xor ecx, ecx
    mov ebx, 10
.write_number_loop:
    xor edx, edx
    div rbx
    add dl, '0'
    dec rsi
    mov [rsi], dl
    inc ecx
    test rax, rax
    jnz .write_number_loop
    mov edx, ecx
    mov edi, 1
    call write_all
    pop rbx
    ret


; chmod OCTAL_MODE PATH
command_chmod:
    mov r12, [argc]
    mov r13, [argv]
    lea rax, [r14 + 2]
    cmp r12, rax
    jne command_usage

    mov rsi, [r13 + r14 * 8]
    xor ebx, ebx
    xor r15d, r15d
.chmod_parse:
    movzx eax, byte [rsi]
    test al, al
    jz .chmod_parsed
    cmp al, '0'
    jb .chmod_invalid
    cmp al, '7'
    ja .chmod_invalid
    sub eax, '0'
    shl ebx, 3
    or ebx, eax
    inc r15
    inc rsi
    jmp .chmod_parse
.chmod_parsed:
    test r15, r15
    jz .chmod_invalid
    mov eax, 90                 ; SYS_chmod
    mov rdi, [r13 + r14 * 8 + 8]
    mov esi, ebx
    syscall
    test rax, rax
    js .chmod_error
    xor edi, edi
    jmp process_exit
.chmod_invalid:
    mov rsi, usage_text
    call write_err
    mov edi, 2
    jmp process_exit
.chmod_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit

; kill PID - send SIGTERM to a positive decimal PID.
command_kill:
    mov r12, [argc]
    mov r13, [argv]
    lea rax, [r14 + 1]
    cmp r12, rax
    jne command_usage
    mov rsi, [r13 + r14 * 8]
    xor ebx, ebx
    xor r15d, r15d
.kill_parse:
    movzx eax, byte [rsi]
    test al, al
    jz .kill_parsed
    cmp al, '0'
    jb .kill_invalid
    cmp al, '9'
    ja .kill_invalid
    imul ebx, ebx, 10
    sub eax, '0'
    add ebx, eax
    inc r15
    inc rsi
    jmp .kill_parse
.kill_parsed:
    test r15, r15
    jz .kill_invalid
    test ebx, ebx
    jz .kill_invalid
    mov eax, 62                 ; SYS_kill
    mov edi, ebx
    mov esi, 15                 ; SIGTERM
    syscall
    test rax, rax
    js .kill_error
    xor edi, edi
    jmp process_exit
.kill_invalid:
    mov rsi, usage_text
    call write_err
    mov edi, 2
    jmp process_exit
.kill_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit

; sync - flush filesystem buffers.
command_sync:
    mov eax, 162                ; SYS_sync
    syscall
    xor edi, edi
    jmp process_exit

; id - report numeric real UID and GID.
command_id:
    mov eax, 102                ; SYS_getuid
    syscall
    mov r12, rax
    mov eax, 104                ; SYS_getgid
    syscall
    mov r13, rax
    mov rsi, uid_text
    call write_out
    mov rax, r12
    call write_number
    mov rsi, gid_text
    call write_out
    mov rax, r13
    call write_number
    mov rsi, newline_text
    call write_out
    xor edi, edi
    jmp process_exit
