; Flatcore common Windows-style command-line tokenizer (FC64).
; Input text is UTF-8. ASCII quote, backslash, space, and tab syntax is parsed
; bytewise; all other UTF-8 bytes are copied unchanged.
;
; fc_parse_argv(command_line, argv_array, max_args, storage, storage_capacity)
;   -> RAX=argc, or -1 when either output buffer is too small.
; argv_array must have max_args+1 pointer slots (the last is the NULL sentinel).
; storage_capacity includes space for every argument terminator.
; Implements the Windows CRT backslash-before-quote rules.
;
; Linux startup does not need this parser: it normalizes the kernel argv vector
; directly. Windows startup will convert GetCommandLineW to UTF-8 and call it.

fc_parse_argv:
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r12, rdi                ; input cursor
    mov r13, rsi                ; argv pointer array
    mov r14, rdx                ; maximum argument count
    mov r15, rcx                ; output storage cursor
    lea rbx, [rcx + r8]          ; output storage end
    xor r10d, r10d              ; argc

    test r12, r12
    jz .invalid
    test r13, r13
    jz .invalid
    test r15, r15
    jz .invalid

.skip_outer:
    mov al, [r12]
    cmp al, ' '
    je .skip_outer_one
    cmp al, 9
    jne .begin_arg
.skip_outer_one:
    inc r12
    jmp .skip_outer

.begin_arg:
    cmp byte [r12], 0
    je .finished
    cmp r10, r14
    jae .overflow
    mov [r13 + r10*8], r15
    xor r11d, r11d              ; quote state: 0 outside, 1 inside

.parse_arg:
    mov al, [r12]
    test al, al
    jz .end_arg
    test r11d, r11d
    jnz .parse_special
    cmp al, ' '
    je .end_arg
    cmp al, 9
    je .end_arg

.parse_special:
    cmp al, '\\'
    je .backslashes
    cmp al, '"'
    je .quote
    jmp .copy_byte

.backslashes:
    xor r8d, r8d
.count_backslashes:
    cmp byte [r12], '\\'
    jne .backslashes_counted
    inc r8
    inc r12
    jmp .count_backslashes
.backslashes_counted:
    cmp byte [r12], '"'
    jne .emit_all_backslashes
    mov r9, r8
    shr r9, 1
.emit_half_backslashes:
    test r9, r9
    jz .backslashes_before_quote
    cmp r15, rbx
    jae .overflow
    mov byte [r15], '\\'
    inc r15
    dec r9
    jmp .emit_half_backslashes
.backslashes_before_quote:
    test r8b, 1
    jnz .escaped_quote
    ; An even number of backslashes leaves a syntactic quote.
    cmp r11d, 0
    je .toggle_quote
    cmp byte [r12 + 1], '"'
    jne .toggle_quote
    ; Inside quotes, a doubled quote is one literal quote.
    cmp r15, rbx
    jae .overflow
    mov byte [r15], '"'
    inc r15
    add r12, 2
    jmp .parse_arg
.toggle_quote:
    xor r11d, 1
    inc r12
    jmp .parse_arg
.escaped_quote:
    cmp r15, rbx
    jae .overflow
    mov byte [r15], '"'
    inc r15
    inc r12
    jmp .parse_arg

.emit_all_backslashes:
    mov r9, r8
.emit_backslash_loop:
    test r9, r9
    jz .parse_arg
    cmp r15, rbx
    jae .overflow
    mov byte [r15], '\\'
    inc r15
    dec r9
    jmp .emit_backslash_loop

.quote:
    cmp r11d, 0
    je .open_quote
    cmp byte [r12 + 1], '"'
    jne .close_quote
    ; Two adjacent quotes while quoted represent a literal quote.
    cmp r15, rbx
    jae .overflow
    mov byte [r15], '"'
    inc r15
    add r12, 2
    jmp .parse_arg
.open_quote:
    mov r11d, 1
    inc r12
    jmp .parse_arg
.close_quote:
    xor r11d, 1
    inc r12
    jmp .parse_arg

.copy_byte:
    cmp r15, rbx
    jae .overflow
    mov [r15], al
    inc r15
    inc r12
    jmp .parse_arg

.end_arg:
    cmp r15, rbx
    jae .overflow
    mov byte [r15], 0
    inc r15
    inc r10
    jmp .skip_outer

.finished:
    mov qword [r13 + r10*8], 0
    mov rax, r10
    jmp .return
.invalid:
    mov rax, -1
    jmp .return
.overflow:
    mov rax, -1
.return:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
