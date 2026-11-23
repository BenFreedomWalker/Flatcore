; Windows x86-64 FC64 I/O backend.
; FC64 handles are native HANDLE values; paths remain UTF-8 until the API edge.

; Initialize the opaque standard stream handles from the process environment.
os_init:
    sub rsp, 40
    mov ecx, -10
    call [GetStdHandle]
    mov [os_stdin], rax
    mov ecx, -11
    call [GetStdHandle]
    mov [os_stdout], rax
    mov ecx, -12
    call [GetStdHandle]
    mov [os_stderr], rax
    add rsp, 40
    xor eax, eax
    ret

; os_read(handle, buffer, byte_count, out_bytes_read)
os_read:
    push r12
    sub rsp, 48
    mov r12, rcx
    mov eax, 0FFFFFFFFh
    cmp rdx, rax
    ja .read_invalid
    mov r9, rsp
    mov qword [rsp + 32], 0
    mov dword [rsp + 40], 0
    mov r8d, edx
    mov rcx, rdi
    mov rdx, rsi
    call [ReadFile]
    test eax, eax
    jz .read_error
    mov eax, [rsp + 40]
    mov [r12], rax
    xor eax, eax
    jmp .read_done
.read_invalid:
    mov eax, FC_ERR_INVALID
    jmp fc_windows_set_normalized_error
.read_error:
    call fc_windows_set_error
.read_done:
    add rsp, 48
    pop r12
    ret

; os_write(handle, buffer, byte_count, out_bytes_written)
os_write:
    push r12
    sub rsp, 48
    mov r12, rcx
    mov eax, 0FFFFFFFFh
    cmp rdx, rax
    ja .write_invalid
    mov r9, rsp
    mov qword [rsp + 32], 0
    mov dword [rsp + 40], 0
    mov r8d, edx
    mov rcx, rdi
    mov rdx, rsi
    call [WriteFile]
    test eax, eax
    jz .write_error
    mov eax, [rsp + 40]
    mov [r12], rax
    xor eax, eax
    jmp .write_done
.write_invalid:
    mov eax, FC_ERR_INVALID
    jmp fc_windows_set_normalized_error
.write_error:
    call fc_windows_set_error
.write_done:
    add rsp, 48
    pop r12
    ret

; os_close(handle). Standard stream handles are owned by the process.
os_close:
    cmp rdi, [os_stdin]
    je .close_invalid
    cmp rdi, [os_stdout]
    je .close_invalid
    cmp rdi, [os_stderr]
    je .close_invalid
    sub rsp, 40
    mov rcx, rdi
    call [CloseHandle]
    test eax, eax
    jz .close_error
    add rsp, 40
    xor eax, eax
    ret
.close_error:
    call fc_windows_set_error
    add rsp, 40
    ret
.close_invalid:
    mov eax, FC_ERR_INVALID
    jmp fc_windows_set_normalized_error

; Native process termination remains at the platform boundary.
os_process_exit:
    sub rsp, 40
    mov ecx, edi
    call [ExitProcess]
    ud2

; Capture GetLastError immediately and translate it to the FC_ERR_* domain.
fc_windows_set_error:
    sub rsp, 40
    call [GetLastError]
    add rsp, 40
    mov edx, eax
    mov [os_native_error], rdx
    mov dword [os_error_domain], FC_ERROR_DOMAIN_WINDOWS
    mov dword [os_error_code], FC_ERR_IO
    cmp eax, 2
    je .not_found
    cmp eax, 3
    je .not_found
    cmp eax, 5
    je .access
    cmp eax, 80
    je .exists
    cmp eax, 183
    je .exists
    cmp eax, 87
    je .invalid
    cmp eax, 8
    je .no_memory
    cmp eax, 14
    je .no_memory
    cmp eax, 112
    je .no_space
    cmp eax, 19
    je .read_only
    cmp eax, 995
    je .interrupted
    cmp eax, 1
    je .unsupported
    mov rax, -1
    ret
.not_found:
    mov dword [os_error_code], FC_ERR_NOT_FOUND
    jmp .failed
.access:
    mov dword [os_error_code], FC_ERR_ACCESS
    jmp .failed
.exists:
    mov dword [os_error_code], FC_ERR_EXISTS
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
.failed:
    mov rax, -1
    ret

; Set a normalized error without querying Win32 (for validation failures).
fc_windows_set_normalized_error:
    mov [os_error_code], eax
    mov dword [os_error_domain], FC_ERROR_DOMAIN_WINDOWS
    mov qword [os_native_error], 0
    mov rax, -1
    ret
