; Flatcore common memory primitives (FC64).
; These routines are platform-neutral and use no OS APIs or syscalls.

; fc_memcpy(destination, source, byte_count) -> RAX=destination.
; Copies exactly byte_count bytes. Overlapping ranges are not supported.
fc_memcpy:
    mov rax, rdi
    test rdx, rdx
    jz .done
    mov rcx, rdx
.loop:
    mov r8b, [rsi]
    mov [rdi], r8b
    inc rsi
    inc rdi
    dec rcx
    jnz .loop
.done:
    ret

; fc_memset(destination, byte_value, byte_count) -> RAX=destination.
fc_memset:
    mov rax, rdi
    test rdx, rdx
    jz .done
    mov rcx, rdx
    mov r8b, sil
.loop:
    mov [rdi], r8b
    inc rdi
    dec rcx
    jnz .loop
.done:
    ret
