; Shared runtime helpers. Linux x86-64 syscall ABI.

; Compare zero-terminated strings. RSI and RDI are inputs; EAX is the result.
streq:
.loop:
    mov al, [rsi]
    cmp al, [rdi]
    jne .no
    test al, al
    jz .yes
    inc rsi
    inc rdi
    jmp .loop
.yes:
    mov eax, 1
    ret
.no:
    xor eax, eax
    ret

; Write RDX bytes from RSI to descriptor RDI. EAX=0 success, -1 error.
write_all:
    mov r8, rsi
    mov r9, rdx
.loop:
    test r9, r9
    jz .success
    mov eax, 1                  ; SYS_write
    mov rsi, r8
    mov rdx, r9
    syscall
    test rax, rax
    jle .failure
    add r8, rax
    sub r9, rax
    jmp .loop
.success:
    xor eax, eax
    ret
.failure:
    mov eax, -1
    ret

; Write a zero-terminated string to stdout. Input: RSI.
write_out:
    mov rdi, 1
    jmp write_string

; Write a zero-terminated string to stderr. Input: RSI.
write_err:
    mov rdi, 2

; Write a zero-terminated string to descriptor RDI. Input: RSI.
write_string:
    xor edx, edx
.length:
    cmp byte [rsi + rdx], 0
    je .write
    inc rdx
    jmp .length
.write:
    jmp write_all

; Copy all bytes from descriptor RDI to descriptor RSI.
; EAX=0 on EOF/success, -1 on read/write error.
copy_fd:
    push r12
    push r13
    mov r12, rdi
    mov r13, rsi
.read:
    xor eax, eax                ; SYS_read
    mov edi, r12d
    mov rsi, io_buffer
    mov edx, 4096
    syscall
    test rax, rax
    jz .success
    js .failure
    mov rdx, rax
    mov rdi, r13
    mov rsi, io_buffer
    call write_all
    test eax, eax
    jnz .failure
    jmp .read
.success:
    xor eax, eax
    pop r13
    pop r12
    ret
.failure:
    mov eax, -1
    pop r13
    pop r12
    ret

; Exit with status in EDI.
process_exit:
    mov eax, 60                 ; SYS_exit
    syscall
    ud2
