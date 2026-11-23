; Shared FC64 ls implementation. No native APIs or syscalls are used here.
; Supported options: -a -A -l -d -1, --all, --almost-all,
; --directory, --help, --version, and --.

; fc_cmd_ls_write(handle, buffer, length) -> RAX=0/-1.
fc_cmd_ls_write:
    mov rcx, os_write
    jmp fc_write_all

; fc_cmd_ls_newline() -> RAX=0/-1.
fc_cmd_ls_newline:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_newline
    mov edx, 1
    jmp fc_cmd_ls_write

; fc_cmd_ls_error() writes a generic diagnostic and returns.
fc_cmd_ls_error:
    sub rsp, 8
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_ls_error_text
    mov edx, fc_cmd_ls_error_length
    call fc_cmd_ls_write
    add rsp, 8
    ret

; fc_cmd_ls_emit(name, full_path) -> RAX=0/-1.
; The name is printed; full_path is used for metadata in long mode.
fc_cmd_ls_emit:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov r13, rsi
    test qword [fc_cmd_ls_flags], 4
    jz .emit_name

    mov rdi, r13
    mov rsi, fc_cmd_ls_info
    call os_stat
    test rax, rax
    jnz .emit_failed

    ; File type character.
    mov al, '-'
    mov ecx, dword [fc_cmd_ls_info + 36]
    cmp ecx, FC_TYPE_DIRECTORY
    jne .type_link
    mov al, 'd'
    jmp .type_store
.type_link:
    cmp ecx, FC_TYPE_SYMLINK
    jne .type_fifo
    mov al, 'l'
    jmp .type_store
.type_fifo:
    cmp ecx, FC_TYPE_FIFO
    jne .type_socket
    mov al, 'p'
    jmp .type_store
.type_socket:
    cmp ecx, FC_TYPE_SOCKET
    jne .type_device
    mov al, 's'
    jmp .type_store
.type_device:
    cmp ecx, FC_TYPE_DEVICE
    jne .type_store
    mov al, 'c'
.type_store:
    mov [fc_cmd_ls_mode], al

    ; Nine POSIX permission bits. Backends with no POSIX mode support
    ; report mode=0, producing explicit dashes rather than invented modes.
    mov ebx, dword [fc_cmd_ls_info + 40]
    xor r14d, r14d
.permission_loop:
    cmp r14d, 9
    jae .permissions_done
    movzx ecx, word [fc_cmd_ls_perm_bits + r14 * 2]
    mov al, '-'
    test ebx, ecx
    jz .permission_store
    mov al, [fc_cmd_ls_perm_chars + r14]
.permission_store:
    mov [fc_cmd_ls_mode + r14 + 1], al
    inc r14d
    jmp .permission_loop
.permissions_done:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_ls_mode
    mov edx, 10
    call fc_cmd_ls_write
    test rax, rax
    jnz .emit_failed

    mov rdi, [os_stdout]
    mov rsi, fc_cmd_space
    mov edx, 1
    call fc_cmd_ls_write
    test rax, rax
    jnz .emit_failed

    mov rdi, qword [fc_cmd_ls_info]
    mov rsi, fc_cmd_ls_number
    mov edx, 32
    call fc_u64toa
    cmp rax, -1
    je .emit_failed
    mov r15, rax
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_ls_number
    mov rdx, r15
    call fc_cmd_ls_write
    test rax, rax
    jnz .emit_failed
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_space
    mov edx, 1
    call fc_cmd_ls_write
    test rax, rax
    jnz .emit_failed

.emit_name:
    mov rdi, r12
    call fc_strlen
    mov rdx, rax
    mov rdi, [os_stdout]
    mov rsi, r12
    call fc_cmd_ls_write
    test rax, rax
    jnz .emit_failed
    call fc_cmd_ls_newline
    jmp .emit_done
.emit_failed:
    mov rax, -1
.emit_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; fc_cmd_ls_join(base, entry_name) -> RAX=0/-1.
; Builds a UTF-8 path in fc_cmd_ls_full_path.
fc_cmd_ls_join:
    push rbx
    push r12
    push r13
    mov r12, rdi
    mov r13, rsi
    mov rdi, r12
    call fc_strlen
    mov rbx, rax
    cmp rbx, 4000
    ja .join_failed
    mov rdi, fc_cmd_ls_full_path
    mov rsi, r12
    mov rcx, rbx
    rep movsb
    test rbx, rbx
    jz .join_separator
    mov al, [fc_cmd_ls_full_path + rbx - 1]
    cmp al, '/'
    je .join_name
    cmp al, '\'
    je .join_name
.join_separator:
    mov byte [fc_cmd_ls_full_path + rbx], '/'
    inc rbx
.join_name:
    mov rdi, r13
    call fc_strlen
    cmp rax, 4094
    ja .join_failed
    mov rcx, rax
    lea rdi, [fc_cmd_ls_full_path + rbx]
    mov rsi, r13
    rep movsb
    mov byte [fc_cmd_ls_full_path + rbx + rax], 0
    xor eax, eax
    jmp .join_done
.join_failed:
    mov rax, -1
.join_done:
    pop r13
    pop r12
    pop rbx
    ret

; fc_cmd_ls_directory(path) -> RAX=0/-1.
fc_cmd_ls_directory:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov rdi, r12
    lea rsi, [fc_cmd_ls_dir_handle]
    call os_dir_open
    test rax, rax
    jnz .directory_failed

.directory_next:
    mov rdi, [fc_cmd_ls_dir_handle]
    mov rsi, fc_cmd_ls_info
    mov rdx, fc_cmd_ls_entry_name
    mov ecx, 1024
    mov r8, fc_cmd_ls_name_length
    call os_dir_next
    cmp rax, 1
    je .directory_entry
    test rax, rax
    jz .directory_close
    jmp .directory_failed_close

.directory_entry:
    ; Skip "." and ".." unless -a; -A includes other dotfiles.
    cmp byte [fc_cmd_ls_entry_name], '.'
    jne .directory_visible
    cmp byte [fc_cmd_ls_entry_name + 1], 0
    je .directory_dot
    cmp byte [fc_cmd_ls_entry_name + 1], '.'
    jne .directory_hidden
    cmp byte [fc_cmd_ls_entry_name + 2], 0
    je .directory_dot
.directory_hidden:
    test qword [fc_cmd_ls_flags], 3
    jz .directory_next
    jmp .directory_visible
.directory_dot:
    test qword [fc_cmd_ls_flags], 1
    jz .directory_next

.directory_visible:
    mov rdi, r12
    mov rsi, fc_cmd_ls_entry_name
    call fc_cmd_ls_join
    test rax, rax
    jnz .directory_failed_close
    mov rdi, fc_cmd_ls_entry_name
    mov rsi, fc_cmd_ls_full_path
    call fc_cmd_ls_emit
    test rax, rax
    jnz .directory_failed_close
    jmp .directory_next

.directory_close:
    mov rdi, [fc_cmd_ls_dir_handle]
    call os_dir_close
    test rax, rax
    jnz .directory_failed
    xor eax, eax
    jmp .directory_done

.directory_failed_close:
    mov rdi, [fc_cmd_ls_dir_handle]
    call os_dir_close
.directory_failed:
    call fc_cmd_ls_error
    mov eax, -1
.directory_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; ls [OPTION...] [FILE...]
fc_cmd_ls:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi                 ; argc
    mov r13, rsi                 ; argv
    mov r14, 1                   ; argument index
    xor ebx, ebx               ; aggregate operand status
    mov qword [fc_cmd_ls_flags], 0
    xor r15d, r15d               ; options-ended flag

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
    mov rdi, fc_cmd_ls_opt_all
    call fc_streq
    test eax, eax
    jnz .set_all
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_ls_opt_almost
    call fc_streq
    test eax, eax
    jnz .set_almost
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_ls_opt_directory
    call fc_streq
    test eax, eax
    jnz .set_directory
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_ls_opt_help
    call fc_streq
    test eax, eax
    jnz .help
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_ls_opt_version
    call fc_streq
    test eax, eax
    jnz .version
    jmp .bad_option

.set_all:
    or qword [fc_cmd_ls_flags], 1
    inc r14
    jmp .parse
.set_almost:
    or qword [fc_cmd_ls_flags], 2
    inc r14
    jmp .parse
.set_directory:
    or qword [fc_cmd_ls_flags], 8
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
    cmp al, 'a'
    je .short_all
    cmp al, 'A'
    je .short_almost
    cmp al, 'l'
    je .short_long
    cmp al, 'd'
    je .short_directory
    cmp al, '1'
    je .short_one
    jmp .bad_option
.short_all:
    or qword [fc_cmd_ls_flags], 1
    inc rsi
    jmp .short_loop
.short_almost:
    or qword [fc_cmd_ls_flags], 2
    inc rsi
    jmp .short_loop
.short_long:
    or qword [fc_cmd_ls_flags], 4
    inc rsi
    jmp .short_loop
.short_directory:
    or qword [fc_cmd_ls_flags], 8
    inc rsi
    jmp .short_loop
.short_one:
    or qword [fc_cmd_ls_flags], 16
    inc rsi
    jmp .short_loop
.short_done:
    inc r14
    jmp .parse

.help:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_ls_help_text
    mov edx, fc_cmd_ls_help_length
    call fc_cmd_ls_write
    xor eax, eax
    jmp .return
.version:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_ls_version_text
    mov edx, fc_cmd_ls_version_length
    call fc_cmd_ls_write
    xor eax, eax
    jmp .return
.bad_option:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_ls_bad_option_text
    mov edx, fc_cmd_ls_bad_option_length
    call fc_cmd_ls_write
    mov eax, 2
    jmp .return

.parsed:
    cmp r14, r12
    jb .operand_loop
    mov rdi, fc_cmd_ls_dot
    call fc_cmd_ls_process_operand
    jmp .return

.operand_loop:
    cmp r14, r12
    jae .success
    mov rdi, [r13 + r14 * 8]
    call fc_cmd_ls_process_operand
    test eax, eax
    jnz .operand_failed
    inc r14
    jmp .operand_loop
.operand_failed:
    mov ebx, 1
    ; Continue processing later operands, like GNU ls.
    inc r14
    jmp .operand_loop
.success:
    mov eax, ebx
.return:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; fc_cmd_ls_process_operand(path) -> RAX=0/-1.
fc_cmd_ls_process_operand:
    push rbx
    push r12
    push r13
    mov r12, rdi
    mov rdi, r12
    mov rsi, fc_cmd_ls_info
    call os_stat
    test rax, rax
    jnz .operand_error
    cmp dword [fc_cmd_ls_info + 36], FC_TYPE_DIRECTORY
    jne .print_operand
    test qword [fc_cmd_ls_flags], 8
    jnz .print_operand
    mov rdi, r12
    call fc_cmd_ls_directory
    jmp .operand_done
.print_operand:
    mov rdi, r12
    mov rsi, r12
    call fc_cmd_ls_emit
    test rax, rax
    jz .operand_done
    call fc_cmd_ls_error
    mov rax, -1
    jmp .operand_done
.operand_error:
    call fc_cmd_ls_error
    mov rax, -1
.operand_done:
    pop r13
    pop r12
    pop rbx
    ret
