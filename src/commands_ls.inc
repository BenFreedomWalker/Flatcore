; ls - directory listing with grouped short options and GNU-style switches.
; Supported: -a -A -l -d -1, --all --almost-all --directory --help --version, --.

command_ls:
    mov qword [ls_flags], 0
    mov r12, [argc]
    mov r13, [argv]
    mov r8, r14
    xor r9d, r9d                ; option parsing has ended when nonzero
.ls_parse_argument:
    cmp r8, r12
    jae .ls_parsed
    mov rsi, [r13 + r8 * 8]
    test r9d, r9d
    jnz .ls_parsed
    cmp byte [rsi], '-'
    jne .ls_parsed
    cmp byte [rsi + 1], 0
    je .ls_parsed
    cmp byte [rsi + 1], '-'
    jne .ls_short_options
    cmp byte [rsi + 2], 0
    je .ls_end_options
    mov rdi, ls_opt_all
    call streq
    test eax, eax
    jnz .ls_set_all
    mov rsi, [r13 + r8 * 8]
    mov rdi, ls_opt_almost
    call streq
    test eax, eax
    jnz .ls_set_almost
    mov rsi, [r13 + r8 * 8]
    mov rdi, ls_opt_directory
    call streq
    test eax, eax
    jnz .ls_set_directory
    mov rsi, [r13 + r8 * 8]
    mov rdi, ls_opt_help
    call streq
    test eax, eax
    jnz .ls_help
    mov rsi, [r13 + r8 * 8]
    mov rdi, ls_opt_version
    call streq
    test eax, eax
    jnz .ls_version
    jmp .ls_bad_option
.ls_set_all:
    or qword [ls_flags], 1
    inc r8
    jmp .ls_parse_argument
.ls_set_almost:
    or qword [ls_flags], 2
    inc r8
    jmp .ls_parse_argument
.ls_set_directory:
    or qword [ls_flags], 8
    inc r8
    jmp .ls_parse_argument
.ls_end_options:
    mov r9d, 1
    inc r8
    jmp .ls_parse_argument
.ls_short_options:
    inc rsi
.ls_short_loop:
    mov al, [rsi]
    test al, al
    jz .ls_short_done
    cmp al, 'a'
    je .ls_short_all
    cmp al, 'A'
    je .ls_short_almost
    cmp al, 'l'
    je .ls_short_long
    cmp al, 'd'
    je .ls_short_directory
    cmp al, '1'
    je .ls_short_one
    jmp .ls_bad_option
.ls_short_all:
    or qword [ls_flags], 1
    inc rsi
    jmp .ls_short_loop
.ls_short_almost:
    or qword [ls_flags], 2
    inc rsi
    jmp .ls_short_loop
.ls_short_long:
    or qword [ls_flags], 4
    inc rsi
    jmp .ls_short_loop
.ls_short_directory:
    or qword [ls_flags], 8
    inc rsi
    jmp .ls_short_loop
.ls_short_one:
    or qword [ls_flags], 16
    inc rsi
    jmp .ls_short_loop
.ls_short_done:
    inc r8
    jmp .ls_parse_argument
.ls_help:
    mov rsi, ls_help_text
    call write_out
    xor edi, edi
    jmp process_exit
.ls_version:
    mov rsi, ls_version_text
    call write_out
    xor edi, edi
    jmp process_exit
.ls_bad_option:
    mov rbx, [r13 + r8 * 8]
    mov rsi, ls_bad_option_prefix
    call write_err
    mov rsi, rbx
    call write_err
    mov rsi, newline_text
    call write_err
    mov edi, 2
    jmp process_exit
.ls_parsed:
    mov [ls_arg_index], r8
    cmp r8, r12
    jne .ls_operands
    test qword [ls_flags], 8
    jnz .ls_default_directory
    mov rdi, dot_text
    call ls_one
    test eax, eax
    js .ls_failed
    xor edi, edi
    jmp process_exit
.ls_default_directory:
    mov rsi, dot_text
    call write_out
    mov rsi, newline_text
    call write_out
    xor edi, edi
    jmp process_exit
.ls_operands:
    mov r8, [ls_arg_index]
.ls_operand_loop:
    cmp r8, r12
    jae .ls_success
    mov rdi, [r13 + r8 * 8]
    mov [ls_arg_index], r8
    test qword [ls_flags], 8
    jnz .ls_print_operand
    call ls_one
    mov r8, [ls_arg_index]
    test eax, eax
    js .ls_failed
    jmp .ls_next_operand
.ls_print_operand:
    mov rsi, rdi
    call write_out
    mov rsi, newline_text
    call write_out
.ls_next_operand:
    mov r8, [ls_arg_index]
    inc r8
    jmp .ls_operand_loop
.ls_success:
    xor edi, edi
    jmp process_exit
.ls_failed:
    mov edi, 1
    jmp process_exit

; Open and list one directory. RDI = path. Returns EAX=-1 on error.
ls_one:
    push r12
    push r13
    push r14
    push r15
    push rbx
    mov eax, 2                  ; SYS_open
    mov esi, 65536              ; O_DIRECTORY
    xor edx, edx
    syscall
    test rax, rax
    js .ls_open_error
    mov r12, rax
.ls_read:
    mov eax, 217                ; SYS_getdents64
    mov edi, r12d
    mov rsi, io_buffer
    mov edx, 4096
    syscall
    test rax, rax
    jz .ls_close
    js .ls_read_error
    mov r13, rax
    mov r14, io_buffer
.ls_entry:
    test r13, r13
    jz .ls_read
    movzx ebx, word [r14 + 16]  ; d_reclen
    test ebx, ebx
    jz .ls_read_error
    lea r15, [r14 + 19]         ; d_name
    ; Skip dot entries unless -a was explicitly requested.
    cmp byte [r15], '.'
    jne .ls_check_hidden
    cmp byte [r15 + 1], 0
    je .ls_dot_entry
    cmp byte [r15 + 1], '.'
    jne .ls_check_hidden
    cmp byte [r15 + 2], 0
    je .ls_dot_entry
.ls_check_hidden:
    cmp byte [r15], '.'
    jne .ls_emit_entry
    test qword [ls_flags], 3
    jz .ls_skip_entry
.ls_emit_entry:
    test qword [ls_flags], 4
    jnz .ls_emit_long
    mov rsi, r15
    call write_out
    mov rsi, newline_text
    call write_out
    jmp .ls_advance
.ls_emit_long:
    mov [ls_name_ptr], r15
    mov eax, 262                ; SYS_newfstatat
    mov rdi, r12
    mov rsi, r15
    mov rdx, ls_stat
    xor r10d, r10d
    syscall
    test rax, rax
    js .ls_stat_error
    call ls_format_mode
    mov rsi, ls_mode_text
    call write_out
    mov rsi, space_text
    call write_out
    mov rax, qword [ls_stat + 48] ; st_size (64-bit off_t)
    call write_number
    mov rsi, space_text
    call write_out
    mov rsi, [ls_name_ptr]
    call write_out
    mov rsi, newline_text
    call write_out
    jmp .ls_advance
.ls_dot_entry:
    test qword [ls_flags], 1
    jnz .ls_emit_entry
.ls_skip_entry:
.ls_advance:
    add r14, rbx
    sub r13, rbx
    jmp .ls_entry
.ls_stat_error:
    mov rsi, operation_error
    call write_err
    jmp .ls_read_error
.ls_read_error:
    mov eax, 3
    mov edi, r12d
    syscall
    mov eax, -1
    jmp .ls_return
.ls_close:
    mov eax, 3
    mov edi, r12d
    syscall
    xor eax, eax
    jmp .ls_return
.ls_open_error:
    mov rsi, operation_error
    call write_err
    mov eax, -1
.ls_return:
    pop rbx
    pop r15
    pop r14
    pop r13
    pop r12
    ret

; Build a compact GNU-style type + rwx permission string from struct stat.
; st_mode is at offset 24 in the x86-64 struct stat.
ls_format_mode:
    push rbx
    push r12
    mov eax, [ls_stat + 24]
    mov ebx, eax
    and eax, 0xF000
    mov byte [ls_mode_text], '-'
    cmp eax, 0x4000
    jne .mode_symlink
    mov byte [ls_mode_text], 'd'
    jmp .mode_permissions
.mode_symlink:
    cmp eax, 0xA000
    jne .mode_fifo
    mov byte [ls_mode_text], 'l'
    jmp .mode_permissions
.mode_fifo:
    cmp eax, 0x1000
    jne .mode_socket
    mov byte [ls_mode_text], 'p'
    jmp .mode_permissions
.mode_socket:
    cmp eax, 0xC000
    jne .mode_char
    mov byte [ls_mode_text], 's'
    jmp .mode_permissions
.mode_char:
    cmp eax, 0x2000
    jne .mode_block
    mov byte [ls_mode_text], 'c'
    jmp .mode_permissions
.mode_block:
    cmp eax, 0x6000
    jne .mode_permissions
    mov byte [ls_mode_text], 'b'
.mode_permissions:
    xor r12d, r12d
.mode_perm_loop:
    cmp r12d, 9
    jae .mode_done
    movzx eax, word [ls_perm_bits + r12 * 2]
    test ebx, eax
    jz .mode_dash
    mov al, [ls_perm_chars + r12]
    jmp .mode_store
.mode_dash:
    mov al, '-'
.mode_store:
    mov [ls_mode_text + r12 + 1], al
    inc r12d
    jmp .mode_perm_loop
.mode_done:
    mov byte [ls_mode_text + 10], 0
    pop r12
    pop rbx
    ret
