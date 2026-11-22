; Flatcore common numeric formatting (FC64).
; No locale or platform APIs are used.

; fc_u64toa(value, destination, capacity) -> RAX=length, or -1.
; Writes an unsigned decimal integer and a terminating NUL.
; The buffer must have room for the actual digits plus the terminator.
fc_u64toa:
    mov r8, rsi
    mov r9, rdx
    xor ecx, ecx
    mov rax, rdi
    test rax, rax
    jnz .convert
    cmp r9, 2
    jb .too_small
    mov byte [r8], '0'
    mov byte [r8 + 1], 0
    mov eax, 1
    ret
.convert:
    xor edx, edx
    mov r10d, 10
    div r10
    add dl, '0'
    mov [r8 + rcx], dl
    inc rcx
    mov rax, rdi
    ; Advance to the quotient for the next digit.
    mov rdi, rax
    cmp rdi, 0
    jne .convert
    ; Reverse the generated digits in place.
    xor r10d, r10d
    lea r11, [rcx - 1]
.reverse:
    cmp r10, r11
    jae .terminate
    mov al, [r8 + r10]
    mov dl, [r8 + r11]
    mov [r8 + r10], dl
    mov [r8 + r11], al
    inc r10
    dec r11
    jmp .reverse
.terminate:
    cmp r9, rcx
    jbe .too_small
    mov byte [r8 + rcx], 0
    mov rax, rcx
    ret
.too_small:
    mov rax, -1
    ret
