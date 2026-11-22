; Initial Flatcore core command set.

command_help:
    mov rsi, help_text
    call write_out
    xor edi, edi
    jmp process_exit

command_true:
    xor edi, edi
    jmp process_exit

command_false:
    mov edi, 1
    jmp process_exit

command_echo:
    mov r12, [argc]
    mov r13, [argv]
    mov r15, r14
.echo_loop:
    cmp r15, r12
    jae .echo_newline
    mov rsi, [r13 + r15 * 8]
    call write_out
    inc r15
    cmp r15, r12
    jae .echo_newline
    mov rsi, space_text
    call write_out
    jmp .echo_loop
.echo_newline:
    mov rsi, newline_text
    call write_out
    xor edi, edi
    jmp process_exit

command_pwd:
    mov eax, 79                 ; SYS_getcwd
    mov rdi, cwd_buffer
    mov esi, 4096
    syscall
    test rax, rax
    js .pwd_error
    dec rax
    mov rdx, rax
    mov rsi, cwd_buffer
    mov edi, 1
    call write_all
    mov rsi, newline_text
    call write_out
    xor edi, edi
    jmp process_exit
.pwd_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit

command_cat:
    mov r12, [argc]
    mov r13, [argv]
    cmp r12, r14
    je .cat_stdin
    mov r15, r14
.cat_next:
    cmp r15, r12
    jae .cat_done
    mov eax, 2                  ; SYS_open
    mov rdi, [r13 + r15 * 8]
    xor esi, esi                ; O_RDONLY
    xor edx, edx
    syscall
    test rax, rax
    js .cat_open_error
    mov rbx, rax
    mov rdi, rbx
    mov esi, 1
    call copy_fd
    test eax, eax
    jnz .cat_copy_error
    mov eax, 3                  ; SYS_close
    mov edi, ebx
    syscall
    inc r15
    jmp .cat_next
.cat_copy_error:
    mov eax, 3                  ; SYS_close
    mov edi, ebx
    syscall
    jmp .cat_error
.cat_stdin:
    xor edi, edi
    mov esi, 1
    call copy_fd
    test eax, eax
    jnz .cat_error
.cat_done:
    xor edi, edi
    jmp process_exit
.cat_open_error:
.cat_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit

command_mkdir:
    mov r12, [argc]
    mov r13, [argv]
    cmp r12, r14
    je command_usage
    mov r15, r14
.mkdir_loop:
    cmp r15, r12
    jae .mkdir_done
    mov eax, 83                 ; SYS_mkdir
    mov rdi, [r13 + r15 * 8]
    mov esi, 493                ; 0755
    syscall
    test rax, rax
    js .mkdir_error
    inc r15
    jmp .mkdir_loop
.mkdir_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit
.mkdir_done:
    xor edi, edi
    jmp process_exit

command_rmdir:
    mov r12, [argc]
    mov r13, [argv]
    cmp r12, r14
    je command_usage
    mov r15, r14
.rmdir_loop:
    cmp r15, r12
    jae .rmdir_done
    mov eax, 84                 ; SYS_rmdir
    mov rdi, [r13 + r15 * 8]
    syscall
    test rax, rax
    js .rmdir_error
    inc r15
    jmp .rmdir_loop
.rmdir_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit
.rmdir_done:
    xor edi, edi
    jmp process_exit

command_rm:
    mov r12, [argc]
    mov r13, [argv]
    cmp r12, r14
    je command_usage
    mov r15, r14
.rm_loop:
    cmp r15, r12
    jae .rm_done
    mov eax, 87                 ; SYS_unlink
    mov rdi, [r13 + r15 * 8]
    syscall
    test rax, rax
    js .rm_error
    inc r15
    jmp .rm_loop
.rm_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit
.rm_done:
    xor edi, edi
    jmp process_exit

command_touch:
    mov r12, [argc]
    mov r13, [argv]
    cmp r12, r14
    je command_usage
    mov r15, r14
.touch_loop:
    cmp r15, r12
    jae .touch_done
    mov rbx, [r13 + r15 * 8]
    mov eax, 2                  ; SYS_open
    mov rdi, rbx
    mov esi, 65                 ; O_WRONLY | O_CREAT
    mov edx, 420                ; 0644
    syscall
    test rax, rax
    js .touch_error
    mov rbx, rax
    mov eax, 3                  ; SYS_close
    mov edi, ebx
    syscall
    mov rbx, [r13 + r15 * 8]
    mov eax, 280                ; SYS_utimensat
    mov rdi, -100               ; AT_FDCWD
    mov rsi, rbx
    xor edx, edx                ; NULL times = current time
    xor r10d, r10d
    syscall
    test rax, rax
    js .touch_error
    inc r15
    jmp .touch_loop
.touch_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit
.touch_done:
    xor edi, edi
    jmp process_exit

command_cp:
    mov r12, [argc]
    lea rdx, [r14 + 2]
    cmp r12, rdx
    jne command_usage
    mov r13, [argv]
    mov eax, 2                  ; SYS_open source
    mov rdi, [r13 + r14 * 8]
    xor esi, esi
    xor edx, edx
    syscall
    test rax, rax
    js .cp_error
    mov rbx, rax
    mov eax, 2                  ; SYS_open destination
    mov rdi, [r13 + r14 * 8 + 8]
    mov esi, 577                ; O_WRONLY | O_CREAT | O_TRUNC
    mov edx, 420                ; 0644
    syscall
    test rax, rax
    js .cp_close_source
    mov r14, rax
    mov rdi, rbx
    mov rsi, r14
    call copy_fd
    mov r12, rax
    mov eax, 3                  ; SYS_close destination
    mov edi, r14d
    syscall
    mov eax, 3                  ; SYS_close source
    mov edi, ebx
    syscall
    test r12, r12
    js .cp_error
    xor edi, edi
    jmp process_exit
.cp_close_source:
    mov eax, 3
    mov edi, ebx
    syscall
.cp_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit

command_mv:
    mov r12, [argc]
    lea rdx, [r14 + 2]
    cmp r12, rdx
    jne command_usage
    mov r13, [argv]
    mov eax, 82                 ; SYS_rename
    mov rdi, [r13 + r14 * 8]
    mov rsi, [r13 + r14 * 8 + 8]
    syscall
    test rax, rax
    js .mv_error
    xor edi, edi
    jmp process_exit
.mv_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit

command_clear:
    mov rsi, clear_text
    call write_out
    xor edi, edi
    jmp process_exit

command_uname:
    mov eax, 63                 ; SYS_uname
    mov rdi, uname_buffer
    syscall
    test rax, rax
    js .uname_error
    mov rsi, uname_buffer
    call write_out
    mov rsi, space_text
    call write_out
    mov rsi, uname_buffer + 65
    call write_out
    mov rsi, space_text
    call write_out
    mov rsi, uname_buffer + 130
    call write_out
    mov rsi, space_text
    call write_out
    mov rsi, uname_buffer + 195
    call write_out
    mov rsi, space_text
    call write_out
    mov rsi, uname_buffer + 260
    call write_out
    mov rsi, newline_text
    call write_out
    xor edi, edi
    jmp process_exit
.uname_error:
    mov rsi, operation_error
    call write_err
    mov edi, 1
    jmp process_exit

command_usage:
    mov rsi, usage_text
    call write_err
    mov edi, 2
    jmp process_exit
