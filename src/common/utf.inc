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


; fc_utf8_to_utf16(source_utf8, destination_utf16, destination_capacity_units)
; Strict UTF-8 decoder. Returns UTF-16 code units written (excluding NUL),
; or -1 for malformed UTF-8 / insufficient capacity. Capacity includes NUL.
fc_utf8_to_utf16:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
    xor r15d, r15d
    test r12, r12
    jz .bad
    test r13, r13
    jz .bad
    test r14, r14
    jz .bad
.decode:
    movzx ebx, byte [r12]
    test ebx, ebx
    jz .finish
    inc r12
    cmp ebx, 80h
    jb .emit_bmp
    cmp ebx, 0C2h
    jb .bad
    cmp ebx, 0DFh
    jbe .decode_two
    cmp ebx, 0EFh
    jbe .decode_three
    cmp ebx, 0F4h
    jbe .decode_four
    jmp .bad
.decode_two:
    movzx eax, byte [r12]
    test al, 0C0h
    cmp al, 80h
    jne .bad
    inc r12
    and ebx, 1Fh
    shl ebx, 6
    and eax, 3Fh
    or ebx, eax
    jmp .emit_bmp
.decode_three:
    movzx eax, byte [r12]
    test al, al
    jz .bad
    cmp al, 80h
    jb .bad
    cmp al, 0BFh
    ja .bad
    movzx ecx, byte [r12 + 1]
    test cl, cl
    jz .bad
    cmp cl, 80h
    jb .bad
    cmp cl, 0BFh
    ja .bad
    cmp bl, 0E0h
    jne .three_not_e0
    cmp al, 0A0h
    jb .bad
.three_not_e0:
    cmp bl, 0EDh
    jne .three_not_ed
    cmp al, 9Fh
    ja .bad
.three_not_ed:
    add r12, 2
    and ebx, 0Fh
    shl ebx, 12
    and eax, 3Fh
    shl eax, 6
    or ebx, eax
    and ecx, 3Fh
    or ebx, ecx
    jmp .emit_bmp
.decode_four:
    movzx eax, byte [r12]
    test al, al
    jz .bad
    cmp al, 80h
    jb .bad
    cmp al, 0BFh
    ja .bad
    movzx ecx, byte [r12 + 1]
    test cl, cl
    jz .bad
    cmp cl, 80h
    jb .bad
    cmp cl, 0BFh
    ja .bad
    movzx edx, byte [r12 + 2]
    test dl, dl
    jz .bad
    cmp dl, 80h
    jb .bad
    cmp dl, 0BFh
    ja .bad
    cmp bl, 0F0h
    jne .four_not_f0
    cmp al, 90h
    jb .bad
.four_not_f0:
    cmp bl, 0F4h
    jne .four_range_ok
    cmp al, 8Fh
    ja .bad
.four_range_ok:
    lea rax, [r15 + 3]       ; two UTF-16 units plus the terminating NUL
    cmp rax, r14
    ja .bad
    add r12, 3
    and ebx, 7
    shl ebx, 18
    and eax, 3Fh
    shl eax, 12
    or ebx, eax
    and ecx, 3Fh
    shl ecx, 6
    or ebx, ecx
    and edx, 3Fh
    or ebx, edx
    sub ebx, 10000h
    mov eax, ebx
    shr eax, 10
    add eax, 0D800h
    mov [r13 + r15*2], ax
    and ebx, 3FFh
    add ebx, 0DC00h
    mov [r13 + r15*2 + 2], bx
    add r15, 2
    jmp .decode
.emit_bmp:
    cmp r15, r14
    jae .bad
    mov [r13 + r15*2], bx
    inc r15
    jmp .decode
.finish:
    cmp r15, r14
    jae .bad
    mov word [r13 + r15*2], 0
    mov rax, r15
    jmp .return
.bad:
    mov rax, -1
.return:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
