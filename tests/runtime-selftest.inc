; Shared Flatcore runtime helper self-tests.
; fc_runtime_selftest() -> RAX=0 success, 1 failure.
fc_runtime_selftest:
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; String length and equality.
    mov rdi, test_text_hello
    call fc_strlen
    cmp rax, 5
    jne .failed
    mov rdi, test_text_hello
    mov rsi, test_text_hello_copy
    call fc_streq
    test eax, eax
    jz .failed

    ; Bounded string copy.
    mov rdi, test_copy_buffer
    mov rsi, test_text_hello
    mov edx, 16
    call fc_strcopy
    cmp rax, 5
    jne .failed
    mov rdi, test_copy_buffer
    mov rsi, test_text_hello
    call fc_streq
    test eax, eax
    jz .failed

    ; Unsigned 64-bit decimal conversion, including UINT64_MAX.
    mov rdi, 0FFFFFFFFFFFFFFFFh
    mov rsi, test_number_buffer
    mov edx, 21
    call fc_u64toa
    cmp rax, 20
    jne .failed
    mov rdi, test_number_buffer
    mov rsi, test_uint64_max
    call fc_streq
    test eax, eax
    jz .failed
    mov rdi, test_uint64_max
    call fc_atou64
    test rdx, rdx
    jz .failed
    cmp rax, 0FFFFFFFFFFFFFFFFh
    jne .failed

    ; UTF-16 -> UTF-8: ASCII, 2-byte, 3-byte and surrogate-pair code points.
    mov rdi, test_utf16
    mov rsi, test_utf8_buffer
    mov edx, 16
    call fc_utf16_to_utf8
    cmp rax, 10
    jne .failed
    mov rdi, test_utf8_buffer
    mov rsi, test_utf8_expected
    call fc_streq
    test eax, eax
    jz .failed

    ; Windows-style command-line tokenization with a quoted argument.
    mov rdi, test_command_line
    mov rsi, test_argv
    mov edx, 8
    mov rcx, test_arg_storage
    mov r8d, 128
    call fc_parse_argv
    cmp rax, 4
    jne .failed
    mov rdi, [test_argv + 16]
    mov rsi, test_quoted_argument
    call fc_streq
    test eax, eax
    jz .failed
    cmp qword [test_argv + 32], 0
    jne .failed

    ; Explicit flatcore COMMAND form normalizes argv to COMMAND.
    mov edi, 4
    mov rsi, test_argv
    mov rdx, test_executable_name
    mov rcx, test_normalized_argc
    mov r8, test_normalized_argv
    call fc_prepare_command
    test eax, eax
    jnz .failed
    cmp qword [test_normalized_argc], 3
    jne .failed
    mov rax, [test_normalized_argv]
    cmp rax, [test_argv + 8]
    jne .failed

    ; Multicall invocation uses argv[0] as the command.
    mov rax, test_multicall_path
    mov [test_argv], rax
    mov rax, test_text_hello
    mov [test_argv + 8], rax
    mov edi, 2
    mov rsi, test_argv
    mov rdx, test_executable_name
    mov rcx, test_normalized_argc
    mov r8, test_normalized_argv
    call fc_prepare_command
    test eax, eax
    jnz .failed
    cmp qword [test_normalized_argc], 2
    jne .failed
    mov rax, [test_normalized_argv]
    mov rdx, test_multicall_command
    cmp rax, rdx
    jne .failed

    ; Table-driven dispatcher invokes the registered handler and returns status.
    mov rax, test_dispatch_name
    mov [test_dispatch_argv], rax
    mov edi, 1
    mov rsi, test_dispatch_argv
    mov rdx, test_registry
    mov ecx, 1
    call fc_dispatch
    cmp rax, 42
    jne .failed

    ; Normalized error code maps to a stable message.
    mov edi, FC_ERR_ACCESS
    call fc_error_message
    mov rdi, rax
    mov rsi, test_error_access
    call fc_streq
    test eax, eax
    jz .failed

    xor eax, eax
    jmp .return
.failed:
    mov eax, 1
.return:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

test_dispatch_handler:
    mov eax, 42
    ret

