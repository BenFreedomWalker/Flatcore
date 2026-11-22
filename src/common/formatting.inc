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
    ; RAX already contains the quotient for the next digit.
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

; fc_atou64(string) -> RAX=value, RDX=1 on success; RDX=0 on invalid input
; or overflow. Only unsigned base-10 digits are accepted; empty input fails.
fc_atou64:
    xor eax, eax
    xor r8d, r8d
    mov r10d, 10
    cmp byte [rdi], 0
    je .invalid
.parse:
    movzx r9d, byte [rdi]
    test r9b, r9b
    jz .valid
    cmp r9b, '0'
    jb .invalid
    cmp r9b, '9'
    ja .invalid
    sub r9b, '0'
    mul r10
    test rdx, rdx
    jnz .invalid
    add rax, r9
    jc .invalid
    inc rdi
    jmp .parse
.valid:
    mov edx, 1
    ret
.invalid:
    xor eax, eax
    xor edx, edx
    ret
