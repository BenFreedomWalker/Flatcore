; Flatcore common output helpers (FC64).
; Output is routed through a backend writer callback, keeping OS handles and
; native I/O conventions out of shared code.
;
; Writer callback signature: writer(handle, buffer, byte_count, out_written)
; It returns RAX=0 on success, -1 on failure. out_written is a 64-bit count.

; fc_write_all(handle, buffer, byte_count, writer_pointer) -> RAX=0/-1.
; Retries short writes until all bytes are written. A zero-byte successful
; write while data remains is treated as an I/O failure.
fc_write_all:
    push rbx
    push r12
    push r13
    push r14
    sub rsp, 8

    mov r12, rdi                ; opaque output handle
    mov r13, rsi                ; current buffer pointer
    mov r14, rdx                ; bytes remaining
    mov rbx, rcx                ; backend writer function
    mov qword [rsp], 0

.loop:
    test r14, r14
    jz .success
    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    lea rcx, [rsp]
    call rbx
    test rax, rax
    jnz .failure
    mov rax, [rsp]
    test rax, rax
    jz .failure
    cmp rax, r14
    ja .failure
    add r13, rax
    sub r14, rax
    jmp .loop
.success:
    xor eax, eax
    jmp .return
.failure:
    mov rax, -1
.return:
    add rsp, 8
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; fc_write_cstr(handle, string, writer_pointer) -> RAX=0/-1.
fc_write_cstr:
    push rbx
    push r12
    push r13

    mov r12, rdi
    mov r13, rsi
    mov rbx, rdx
    mov rdi, r13
    call fc_strlen
    mov rdx, rax
    mov rdi, r12
    mov rsi, r13
    mov rcx, rbx
    call fc_write_all

    pop r13
    pop r12
    pop rbx
    ret
