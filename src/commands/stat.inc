; Shared stat command built on the FC_FILE_INFO backend contract.
; Reports metadata that both native backends can represent without guessing.
fc_cmd_stat:
    push rbx
    push r12
    push r13
    push r14
    push r15
    cmp rdi, 2
    jne .stat_usage
    mov r12, rsi
    mov r13, [r12 + 8]
    mov rdi, r13
    lea rsi, [fc_cmd_stat_info]
    call os_stat
    test eax, eax
    jnz .stat_error
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_stat_file_prefix
    call fc_cmd_stat_write_cstr
    mov rdi, [os_stdout]
    mov rsi, r13
    call fc_cmd_stat_write_cstr
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_stat_newline
    call fc_cmd_stat_write_cstr
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_stat_size_prefix
    call fc_cmd_stat_write_cstr
    mov rdi, [os_stdout]
    mov rsi, [fc_cmd_stat_info]
    call fc_cmd_stat_write_u64
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_stat_bytes_newline
    call fc_cmd_stat_write_cstr
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_stat_type_prefix
    call fc_cmd_stat_write_cstr
    mov eax, [fc_cmd_stat_info + 36]
    cmp eax, FC_TYPE_REGULAR
    je .stat_regular
    cmp eax, FC_TYPE_DIRECTORY
    je .stat_directory
    cmp eax, FC_TYPE_SYMLINK
    je .stat_symlink
    cmp eax, FC_TYPE_FIFO
    je .stat_fifo
    cmp eax, FC_TYPE_SOCKET
    je .stat_socket
    cmp eax, FC_TYPE_DEVICE
    je .stat_device
    mov rsi, fc_cmd_stat_type_unknown
    jmp .stat_type_write
.stat_regular:
    mov rsi, fc_cmd_stat_type_regular
    jmp .stat_type_write
.stat_directory:
    mov rsi, fc_cmd_stat_type_directory
    jmp .stat_type_write
.stat_symlink:
    mov rsi, fc_cmd_stat_type_symlink
    jmp .stat_type_write
.stat_fifo:
    mov rsi, fc_cmd_stat_type_fifo
    jmp .stat_type_write
.stat_socket:
    mov rsi, fc_cmd_stat_type_socket
    jmp .stat_type_write
.stat_device:
    mov rsi, fc_cmd_stat_type_device
.stat_type_write:
    mov rdi, [os_stdout]
    call fc_cmd_stat_write_cstr
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_stat_newline
    call fc_cmd_stat_write_cstr
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_stat_hidden_prefix
    call fc_cmd_stat_write_cstr
    test dword [fc_cmd_stat_info + 32], FC_ATTR_HIDDEN
    jz .stat_not_hidden
    mov rsi, fc_cmd_stat_yes_newline
    jmp .stat_hidden_write
.stat_not_hidden:
    mov rsi, fc_cmd_stat_no_newline
.stat_hidden_write:
    mov rdi, [os_stdout]
    call fc_cmd_stat_write_cstr
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_stat_readonly_prefix
    call fc_cmd_stat_write_cstr
    test dword [fc_cmd_stat_info + 32], FC_ATTR_READ_ONLY
    jz .stat_not_readonly
    mov rsi, fc_cmd_stat_yes_newline
    jmp .stat_readonly_write
.stat_not_readonly:
    mov rsi, fc_cmd_stat_no_newline
.stat_readonly_write:
    mov rdi, [os_stdout]
    call fc_cmd_stat_write_cstr
    xor eax, eax
    jmp .stat_return
.stat_usage:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_stat_usage_text
    call fc_cmd_stat_write_cstr
    mov eax, 2
    jmp .stat_return
.stat_error:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_stat_error_prefix
    call fc_cmd_stat_write_cstr
    mov rdi, [os_stderr]
    mov rsi, r13
    call fc_cmd_stat_write_cstr
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_stat_error_suffix
    call fc_cmd_stat_write_cstr
    mov eax, 1
.stat_return:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; fc_cmd_stat_write_cstr(handle, string) -> RAX=0/-1.
fc_cmd_stat_write_cstr:
    push rbx
    push r12
    sub rsp, 8
    mov r12, rdi
    mov rbx, rsi
    mov rdi, rbx
    call fc_strlen
    mov rdx, rax
    mov rdi, r12
    mov rsi, rbx
    mov rcx, os_write
    call fc_write_all
    add rsp, 8
    pop r12
    pop rbx
    ret

; fc_cmd_stat_write_u64(handle, value) -> RAX=0/-1.
fc_cmd_stat_write_u64:
    push rbx
    push r12
    sub rsp, 8
    mov r12, rdi
    mov rdi, rsi
    mov rsi, fc_cmd_stat_number
    mov edx, 32
    call fc_u64toa
    cmp rax, -1
    je .number_error
    mov rdx, rax
    mov rdi, r12
    mov rsi, fc_cmd_stat_number
    mov rcx, os_write
    call fc_write_all
    jmp .number_done
.number_error:
    mov rax, -1
.number_done:
    add rsp, 8
    pop r12
    pop rbx
    ret
