; Shared cat implementation (FC64).
; cat [FILE...]
; With no operands, or for an operand named "-", read standard input.
; File data is copied byte-for-byte; no newline or encoding conversion is added.

; fc_cmd_cat_stream(handle) -> RAX=0 on EOF, 1 on read/write failure.
fc_cmd_cat_stream:
    push rbx
    push r12
    sub rsp, 8
    mov r12, rdi
    mov qword [rsp], 0
.cat_stream_read:
    mov rdi, r12
    mov rsi, fc_cmd_cat_buffer
    mov edx, 4096
    lea rcx, [rsp]
    call os_read
    test rax, rax
    jnz .cat_stream_failed
    mov rbx, [rsp]
    test rbx, rbx
    jz .cat_stream_success
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_cat_buffer
    mov rdx, rbx
    mov rcx, os_write
    call fc_write_all
    test rax, rax
    jnz .cat_stream_failed
    jmp .cat_stream_read
.cat_stream_success:
    xor eax, eax
    jmp .cat_stream_done
.cat_stream_failed:
    mov eax, 1
.cat_stream_done:
    add rsp, 8
    pop r12
    pop rbx
    ret

; Emit one consistent diagnostic for an input, output, or close failure.
fc_cmd_cat_report_error:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_cat_error_text
    mov edx, fc_cmd_cat_error_length
    mov rcx, os_write
    jmp fc_write_all

; cat [FILE...]
fc_cmd_cat:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 16
    mov rbx, rdi                 ; argc
    mov r12, rsi                 ; argv
    mov r14, 1                   ; next operand
    xor r15d, r15d               ; aggregate exit status
    cmp rbx, 1
    jne .cat_operand_loop

    mov rdi, [os_stdin]
    call fc_cmd_cat_stream
    test eax, eax
    jz .cat_return
    mov r15d, 1
    call fc_cmd_cat_report_error
    jmp .cat_return

.cat_operand_loop:
    cmp r14, rbx
    jae .cat_return
    mov rdi, [r12 + r14 * 8]
    mov rsi, fc_cmd_cat_stdin_name
    call fc_streq
    test eax, eax
    jnz .cat_stdin_operand

    ; Open a named file through the backend. The returned handle is local.
    mov rdi, [r12 + r14 * 8]
    mov esi, FC_OPEN_READ
    xor edx, edx
    lea rcx, [rsp]
    call os_open
    test rax, rax
    jnz .cat_operand_error
    mov rdi, [rsp]
    call fc_cmd_cat_stream
    test eax, eax
    jnz .cat_close_after_error

    mov rdi, [rsp]
    call os_close
    test rax, rax
    jnz .cat_operand_error
    jmp .cat_next_operand

.cat_close_after_error:
    mov r15d, 1
    mov rdi, [rsp]
    call os_close
    call fc_cmd_cat_report_error
    jmp .cat_next_operand

.cat_stdin_operand:
    mov rdi, [os_stdin]
    call fc_cmd_cat_stream
    test eax, eax
    jz .cat_next_operand
    mov r15d, 1
    call fc_cmd_cat_report_error
    jmp .cat_next_operand

.cat_operand_error:
    mov r15d, 1
    call fc_cmd_cat_report_error

.cat_next_operand:
    inc r14
    jmp .cat_operand_loop

.cat_return:
    mov eax, r15d
    add rsp, 16
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
