; Flatcore UTF-16LE to UTF-8 conversion (FC64).
; This routine is platform-neutral; Windows startup uses it after obtaining the
; native UTF-16 command line. Invalid surrogate sequences are rejected.
;
; fc_utf16_to_utf8(source_utf16, destination_utf8, destination_capacity)
;   -> RAX=number of UTF-8 bytes written, or -1 on invalid input/short buffer.
; The destination capacity includes room for the terminating NUL.

fc_utf16_to_utf8:
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r12, rdi                ; UTF-16 source
    mov r13, rsi                ; UTF-8 destination
    mov r14, rdx                ; destination capacity
    xor r15d, r15d              ; bytes written

    test r12, r12
    jz .invalid
    test r13, r13
    jz .invalid
    test r14, r14
    jz .invalid

.next_codepoint:
    movzx ebx, word [r12]
    test ebx, ebx
    jz .terminate
    add r12, 2

    cmp ebx, 0D800h
    jb .encode
    cmp ebx, 0DBFFh
    jbe .surrogate_pair
    cmp ebx, 0DFFFh
    jbe .invalid

.encode:
    cmp ebx, 7Fh
    jbe .one_byte
    cmp ebx, 7FFh
    jbe .two_bytes
    cmp ebx, 0FFFFh
    jbe .three_bytes
    jmp .four_bytes

.surrogate_pair:
    movzx eax, word [r12]
    cmp eax, 0DC00h
    jb .invalid
    cmp eax, 0DFFFh
    ja .invalid
    add r12, 2
    sub ebx, 0D800h
    shl ebx, 10
    sub eax, 0DC00h
    add ebx, eax
    add ebx, 10000h
    jmp .encode

.one_byte:
    lea rax, [r15 + 1]
    cmp rax, r14
    jae .invalid
    mov [r13 + r15], bl
    inc r15
    jmp .next_codepoint

.two_bytes:
    lea rax, [r15 + 2]
    cmp rax, r14
    jae .invalid
    mov eax, ebx
    shr eax, 6
    or al, 0C0h
    mov [r13 + r15], al
    mov eax, ebx
    and al, 3Fh
    or al, 80h
    mov [r13 + r15 + 1], al
    add r15, 2
    jmp .next_codepoint

.three_bytes:
    lea rax, [r15 + 3]
    cmp rax, r14
    jae .invalid
    mov eax, ebx
    shr eax, 12
    or al, 0E0h
    mov [r13 + r15], al
    mov eax, ebx
    shr eax, 6
    and al, 3Fh
    or al, 80h
    mov [r13 + r15 + 1], al
    mov eax, ebx
    and al, 3Fh
    or al, 80h
    mov [r13 + r15 + 2], al
    add r15, 3
    jmp .next_codepoint

.four_bytes:
    lea rax, [r15 + 4]
    cmp rax, r14
    jae .invalid
    mov eax, ebx
    shr eax, 18
    or al, 0F0h
    mov [r13 + r15], al
    mov eax, ebx
    shr eax, 12
    and al, 3Fh
    or al, 80h
    mov [r13 + r15 + 1], al
    mov eax, ebx
    shr eax, 6
    and al, 3Fh
    or al, 80h
    mov [r13 + r15 + 2], al
    mov eax, ebx
    and al, 3Fh
    or al, 80h
    mov [r13 + r15 + 3], al
    add r15, 4
    jmp .next_codepoint

.terminate:
    cmp r15, r14
    jae .invalid
    mov byte [r13 + r15], 0
    mov rax, r15
    jmp .return
.invalid:
    mov rax, -1
.return:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
