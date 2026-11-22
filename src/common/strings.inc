; Flatcore common string primitives (FC64).
; Strings are NUL-terminated UTF-8; lengths are byte counts.

; fc_strlen(string) -> RAX=length in bytes.
fc_strlen:
    xor eax, eax
.loop:
    cmp byte [rdi + rax], 0
    je .done
    inc rax
    jmp .loop
.done:
    ret

; fc_streq(left, right) -> RAX=1 when equal, otherwise 0.
fc_streq:
.loop:
    mov al, [rdi]
    cmp al, [rsi]
    jne .no
    test al, al
    jz .yes
    inc rdi
    inc rsi
    jmp .loop
.yes:
    mov eax, 1
    ret
.no:
    xor eax, eax
    ret

; fc_strcopy(destination, source, capacity) -> RAX=bytes copied, or -1.
; Capacity includes space for the terminating NUL. No partial copy is made.
fc_strcopy:
    test rdx, rdx
    jz .too_small
    mov r8, rdi
    mov r9, rsi
    mov r10, rdx
    xor ecx, ecx
.measure:
    cmp byte [r9 + rcx], 0
    je .measured
    inc rcx
    cmp rcx, r10
    jae .too_small
    jmp .measure
.measured:
    mov rdx, rcx
    mov rdi, r8
    mov rsi, r9
    call fc_memcpy
    mov byte [r8 + rdx], 0
    mov rax, rdx
    ret
.too_small:
    mov rax, -1
    ret
