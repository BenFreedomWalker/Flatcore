; Flatcore shared command dispatcher (FC64).
;
; Registry layout: repeated pairs of dq command_name_pointer, dq handler_pointer.
; Handler contract: handler(argc, argv) -> RAX=process exit status.
; argv[0] is the command name; argv[1..] are its operands/options.
; The platform entry point is responsible for normalizing argv before calling
; this routine. A missing command returns 127; no process exit occurs here.

; fc_dispatch(argc, argv, registry, registry_count) -> RAX=exit status.
fc_dispatch:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 8

    mov r15, rdi                ; argc
    mov rbx, rsi                ; argv
    mov r12, rdx                ; registry
    mov r13, rcx                ; registry count
    xor r14d, r14d

    test r15, r15
    jz .unknown
    test rbx, rbx
    jz .unknown
    mov rdi, [rbx]              ; argv[0]
    test rdi, rdi
    jz .unknown

.search:
    cmp r14, r13
    jae .unknown
    mov rsi, [r12]              ; registry entry name
    call fc_streq
    test eax, eax
    jnz .found
    add r12, 16
    inc r14
    mov rdi, [rbx]
    jmp .search

.found:
    mov rax, [r12 + 8]          ; command handler
    mov rdi, r15
    mov rsi, rbx
    call rax
    jmp .return

.unknown:
    mov eax, 127

.return:
    add rsp, 8
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
