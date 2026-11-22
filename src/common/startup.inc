; Flatcore shared command selection and argv normalization (FC64).
; All strings are UTF-8. For multicall invocation, argv[0] is normalized to point
; at its basename; the argument strings themselves are never modified.
;
; fc_prepare_command(argc, argv, executable_name, out_argc, out_argv)
;   -> RAX=0 on success, 1 when no explicit command was supplied, -1 invalid.
;
; executable_name is the expected multicall executable basename (normally
; "flatcore"). If argv[0]'s basename matches it, argv[1] is the command and
; the returned vector starts at argv[1]. Otherwise argv[0]'s basename is the
; command (symlink/multicall invocation) and the original vector is retained.
; In either successful case, returned argv[0] is the command name.

fc_prepare_command:
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r12, rdi                ; original argc
    mov r13, rsi                ; original argv
    mov r14, rdx                ; expected executable basename
    mov r15, rcx                ; output argc pointer
    mov rbx, r8                 ; output argv pointer

    test r13, r13
    jz .invalid
    test r15, r15
    jz .invalid
    test rbx, rbx
    jz .invalid
    test r12, r12
    jz .invalid

    mov rdi, [r13]              ; argv[0]
    test rdi, rdi
    jz .invalid
    mov rsi, rdi
.find_basename:
    mov al, [rsi]
    test al, al
    jz .basename_ready
    cmp al, '/'
    je .basename_after_separator
    cmp al, '\'
    jne .basename_next
.basename_after_separator:
    lea rdi, [rsi + 1]
.basename_next:
    inc rsi
    jmp .find_basename
.basename_ready:
    mov rsi, r14
    call fc_streq
    test eax, eax
    jz .multicall_name

    ; Explicit form: flatcore COMMAND [ARG...].
    cmp r12, 2
    jb .no_command
    lea rax, [r13 + 8]
    mov [rbx], rax
    lea rax, [r12 - 1]
    mov [r15], rax
    xor eax, eax
    jmp .return

.multicall_name:
    mov [r13], rdi             ; normalize argv[0] to the executable basename
    mov [rbx], r13
    mov [r15], r12
    xor eax, eax
    jmp .return

.no_command:
    mov qword [r15], 0
    mov qword [rbx], 0
    mov eax, 1
    jmp .return

.invalid:
    mov eax, -1
.return:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
