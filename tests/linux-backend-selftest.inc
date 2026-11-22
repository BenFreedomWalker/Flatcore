; Linux FC64 backend integration checks.
; Returns zero on success; nonzero values identify the failing operation.
fc_linux_backend_selftest:
    push rbx
    push r12
    push r13
    sub rsp, 16

    ; getcwd must return a NUL-terminated path and its byte length.
    mov rdi, linux_test_cwd
    mov esi, 4096
    mov rdx, linux_test_length
    call os_getcwd
    test eax, eax
    jnz .fail1
    cmp qword [linux_test_length], 0
    je .fail1

    ; Create/truncate a file, write a short payload, and close it.
    mov rdi, linux_test_path
    mov esi, FC_OPEN_WRITE or FC_OPEN_CREATE or FC_OPEN_TRUNCATE
    mov edx, 600o
    mov rcx, linux_test_handle
    call os_open
    test eax, eax
    jnz .fail2
    mov rdi, [linux_test_handle]
    mov rsi, linux_test_payload
    mov edx, 2
    mov rcx, linux_test_length
    call os_write
    test eax, eax
    jnz .fail3
    cmp qword [linux_test_length], 2
    jne .fail3
    mov rdi, [linux_test_handle]
    call os_close
    test eax, eax
    jnz .fail4

    ; Reopen and read back the exact payload.
    mov rdi, linux_test_path
    mov esi, FC_OPEN_READ
    xor edx, edx
    mov rcx, linux_test_handle
    call os_open
    test eax, eax
    jnz .fail5
    mov rdi, [linux_test_handle]
    mov rsi, linux_test_readback
    mov edx, 2
    mov rcx, linux_test_length
    call os_read
    test eax, eax
    jnz .fail6
    cmp qword [linux_test_length], 2
    jne .fail6
    mov rdi, [linux_test_handle]
    call os_close
    test eax, eax
    jnz .fail7
    mov al, [linux_test_readback]
    cmp al, 'O'
    jne .fail8
    mov al, [linux_test_readback + 1]
    cmp al, 'K'
    jne .fail8

    ; Remove the fixture and verify the operation succeeds.
    mov rdi, linux_test_path
    call os_unlink
    test eax, eax
    jnz .fail9

    xor eax, eax
    jmp .done
.fail1: mov eax, 31
    jmp .done
.fail2: mov eax, 32
    jmp .done
.fail3:
    mov rdi, [linux_test_handle]
    call os_close
    mov eax, 33
    jmp .done
.fail4: mov eax, 34
    jmp .done
.fail5: mov eax, 35
    jmp .done
.fail6:
    mov rdi, [linux_test_handle]
    call os_close
    mov eax, 36
    jmp .done
.fail7: mov eax, 37
    jmp .done
.fail8: mov eax, 38
    jmp .done
.fail9: mov eax, 39
.done:
    add rsp, 16
    pop r13
    pop r12
    pop rbx
    ret
