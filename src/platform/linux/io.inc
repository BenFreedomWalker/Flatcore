; Linux x86-64 FC64 I/O backend.
; All public routines obey the private FC64 calling convention.

; os_read(handle, buffer, byte_count, out_bytes_read)
os_read:
    mov eax, 0
    syscall
    test rax, rax
    js .read_error
    mov [rcx], rax
    xor eax, eax
    ret
.read_error:
    neg eax
    jmp fc_linux_set_error

; os_write(handle, buffer, byte_count, out_bytes_written)
os_write:
    mov eax, 1
    syscall
    test rax, rax
    js .write_error
    mov [rcx], rax
    xor eax, eax
    ret
.write_error:
    neg eax
    jmp fc_linux_set_error

; os_close(handle)
os_close:
    mov eax, 3
    syscall
    test rax, rax
    js .close_error
    xor eax, eax
    ret
.close_error:
    neg eax
    jmp fc_linux_set_error

; os_process_exit(exit_status) -- Linux native process termination.
os_process_exit:
    mov eax, 60
    syscall
    ud2

; Convert a positive Linux errno in EAX to FC diagnostics and return -1.
; Native errno is preserved in os_native_error.
fc_linux_set_error:
    mov [os_native_error], rax
    mov dword [os_error_domain], FC_ERROR_DOMAIN_LINUX
    mov dword [os_error_code], FC_ERR_IO
    cmp eax, 2
    je .not_found
    cmp eax, 17
    je .exists
    cmp eax, 1
    je .access
    cmp eax, 13
    je .access
    cmp eax, 20
    je .not_dir
    cmp eax, 21
    je .is_dir
    cmp eax, 39
    je .not_empty
    cmp eax, 22
    je .invalid
    cmp eax, 12
    je .no_memory
    cmp eax, 28
    je .no_space
    cmp eax, 30
    je .read_only
    cmp eax, 4
    je .interrupted
    cmp eax, 38
    je .unsupported
    cmp eax, 36
    je .too_small
    mov rax, -1
    ret
.not_found:
    mov dword [os_error_code], FC_ERR_NOT_FOUND
    jmp .failed
.exists:
    mov dword [os_error_code], FC_ERR_EXISTS
    jmp .failed
.access:
    mov dword [os_error_code], FC_ERR_ACCESS
    jmp .failed
.not_dir:
    mov dword [os_error_code], FC_ERR_NOT_DIR
    jmp .failed
.is_dir:
    mov dword [os_error_code], FC_ERR_IS_DIR
    jmp .failed
.not_empty:
    mov dword [os_error_code], FC_ERR_NOT_EMPTY
    jmp .failed
.invalid:
    mov dword [os_error_code], FC_ERR_INVALID
    jmp .failed
.no_memory:
    mov dword [os_error_code], FC_ERR_NO_MEMORY
    jmp .failed
.no_space:
    mov dword [os_error_code], FC_ERR_NO_SPACE
    jmp .failed
.read_only:
    mov dword [os_error_code], FC_ERR_READ_ONLY
    jmp .failed
.interrupted:
    mov dword [os_error_code], FC_ERR_INTERRUPTED
    jmp .failed
.unsupported:
    mov dword [os_error_code], FC_ERR_UNSUPPORTED
    jmp .failed
.too_small:
    mov dword [os_error_code], FC_ERR_TOO_SMALL
.failed:
    mov rax, -1
    ret

; Backend diagnostic state and standard descriptors.
section readable writeable
align 8
os_stdin dq 0
os_stdout dq 1
os_stderr dq 2
os_error_code dd FC_ERR_NONE
os_error_domain dd FC_ERROR_DOMAIN_NONE
os_native_error dq 0
