; Linux x86-64 FC64 directory iteration backend.
; Each live directory handle owns an independent getdents64 buffer.

LINUX_DIR_SLOT_COUNT = 8
LINUX_DIR_BUFFER_SIZE = 4096
LINUX_DIR_SLOT_SIZE = 16 + LINUX_DIR_BUFFER_SIZE

; os_dir_open(path_utf8, out_dir_handle)
; Returns an opaque token (slot index + 1) through the output pointer.
os_dir_open:
    push r12
    push r13
    sub rsp, 8
    mov r13, rsi
    test r13, r13
    jz .open_invalid
    lea r12, [linux_dir_slots]
    mov ecx, LINUX_DIR_SLOT_COUNT
.find_slot:
    cmp dword [r12], 0
    je .slot_found
    add r12, LINUX_DIR_SLOT_SIZE
    loop .find_slot
    mov dword [os_error_code], FC_ERR_NO_MEMORY
    mov dword [os_error_domain], FC_ERROR_DOMAIN_LINUX
    mov qword [os_native_error], 0
    mov rax, -1
    jmp .open_done
.slot_found:
    ; O_RDONLY | O_DIRECTORY | O_CLOEXEC.
    mov esi, 90000h
    mov eax, 2
    syscall
    test rax, rax
    js .open_error
    mov [r12 + 4], eax
    mov dword [r12 + 8], 0
    mov dword [r12 + 12], 0
    mov dword [r12], 1
    mov rax, r12
    lea rdx, [linux_dir_slots]
    sub rax, rdx
    xor edx, edx
    mov ecx, LINUX_DIR_SLOT_SIZE
    div rcx
    inc rax
    mov [r13], rax
    xor eax, eax
    jmp .open_done
.open_invalid:
    mov eax, 22
    call fc_linux_set_error
    jmp .open_done
.open_error:
    neg eax
    call fc_linux_set_error
.open_done:
    add rsp, 8
    pop r13
    pop r12
    ret

; os_dir_next(handle, out_info, name_buffer, name_capacity, out_name_len)
; Returns 1 for an entry, 0 for EOF, and -1 for an error.
os_dir_next:
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov rbx, r8
    mov r13, rsi
    mov r14, rdx
    mov r15, rcx
    test rbx, rbx
    jz .next_invalid
    mov qword [rbx], 0
    test r13, r13
    jz .next_invalid
    test r14, r14
    jz .next_invalid
    cmp rdi, 1
    jb .next_invalid
    cmp rdi, LINUX_DIR_SLOT_COUNT
    ja .next_invalid
    dec rdi
    imul r12, rdi, LINUX_DIR_SLOT_SIZE
    lea r12, [linux_dir_slots + r12]
    cmp dword [r12], 1
    jne .next_invalid

.next_record:
    mov r10d, [r12 + 8]
    cmp r10d, [r12 + 12]
    jb .record_ready

    ; Refill this handle's private getdents64 buffer.
    mov edi, [r12 + 4]
    lea rsi, [r12 + 16]
    mov edx, LINUX_DIR_BUFFER_SIZE
    mov eax, 217
    syscall
    test rax, rax
    js .next_syscall_error
    jz .next_eof
    mov [r12 + 12], eax
    mov dword [r12 + 8], 0
    xor r10d, r10d

.record_ready:
    lea rsi, [r12 + 16]
    add rsi, r10
    movzx ecx, word [rsi + 16]
    cmp ecx, 20
    jb .next_malformed
    mov r11d, ecx
    mov r8d, ecx
    mov eax, r10d
    add eax, r8d
    cmp eax, [r12 + 12]
    ja .next_malformed
    sub r8d, 19
    lea rdx, [rsi + 19]
    xor eax, eax
.scan_name:
    cmp eax, r8d
    jae .next_malformed
    cmp byte [rdx + rax], 0
    je .name_ready
    inc eax
    jmp .scan_name

.name_ready:
    mov r10d, eax
    movzx r9d, byte [rsi + 18]
    cmp r10, r15
    ja .next_too_small

    ; Copy the name, appending NUL only when the supplied capacity permits.
    mov rdi, r14
    mov rsi, rdx
    mov rcx, r10
    rep movsb
    cmp r10, r15
    jae .name_not_terminated
    mov byte [r14 + r10], 0
.name_not_terminated:
    mov [rbx], r10

    ; Consume the record only after the caller's buffer has accepted it.
    add dword [r12 + 8], r11d

    ; Clear FC_FILE_INFO; getdents64 does not provide size or timestamps.
    mov rdi, r13
    xor eax, eax
    mov ecx, 8
    rep stosq

    ; Translate Linux d_type without inventing unavailable metadata.
    cmp r9d, 8
    je .type_regular
    cmp r9d, 4
    je .type_directory
    cmp r9d, 10
    je .type_symlink
    cmp r9d, 1
    je .type_fifo
    cmp r9d, 12
    je .type_socket
    cmp r9d, 6
    je .type_device
    cmp r9d, 2
    je .type_device
    jmp .type_done
.type_regular:
    mov dword [r13 + 36], FC_TYPE_REGULAR
    jmp .type_done
.type_directory:
    mov dword [r13 + 36], FC_TYPE_DIRECTORY
    jmp .type_done
.type_symlink:
    mov dword [r13 + 36], FC_TYPE_SYMLINK
    jmp .type_done
.type_fifo:
    mov dword [r13 + 36], FC_TYPE_FIFO
    jmp .type_done
.type_socket:
    mov dword [r13 + 36], FC_TYPE_SOCKET
    jmp .type_done
.type_device:
    mov dword [r13 + 36], FC_TYPE_DEVICE
.type_done:
    cmp byte [r14], '.'
    jne .next_entry
    or dword [r13 + 32], FC_ATTR_HIDDEN
.next_entry:
    mov eax, 1
    jmp .next_done

.next_eof:
    xor eax, eax
    jmp .next_done
.next_too_small:
    mov dword [os_error_code], FC_ERR_TOO_SMALL
    mov dword [os_error_domain], FC_ERROR_DOMAIN_LINUX
    mov qword [os_native_error], 0
    mov rax, -1
    jmp .next_done
.next_malformed:
    mov eax, 5
    call fc_linux_set_error
    jmp .next_done
.next_syscall_error:
    neg eax
    call fc_linux_set_error
    jmp .next_done
.next_invalid:
    mov eax, 22
    call fc_linux_set_error
.next_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; os_dir_close(handle)
os_dir_close:
    push r12
    cmp rdi, 1
    jb .close_invalid
    cmp rdi, LINUX_DIR_SLOT_COUNT
    ja .close_invalid
    dec rdi
    imul r12, rdi, LINUX_DIR_SLOT_SIZE
    lea r12, [linux_dir_slots + r12]
    cmp dword [r12], 1
    jne .close_invalid
    mov edi, [r12 + 4]
    mov eax, 3
    syscall
    test rax, rax
    js .close_error
    mov dword [r12], 0
    mov dword [r12 + 4], 0
    mov dword [r12 + 8], 0
    mov dword [r12 + 12], 0
    xor eax, eax
    jmp .close_done
.close_error:
    neg eax
    call fc_linux_set_error
    jmp .close_done
.close_invalid:
    mov eax, 22
    call fc_linux_set_error
.close_done:
    pop r12
    ret
