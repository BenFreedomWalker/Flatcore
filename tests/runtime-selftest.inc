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
    jne .fail1
    mov rdi, test_text_hello
    mov rsi, test_text_hello_copy
    call fc_streq
    test eax, eax
    jz .fail2

    ; Bounded string copy.
    mov rdi, test_copy_buffer
    mov rsi, test_text_hello
    mov edx, 16
    call fc_strcopy
    cmp rax, 5
    jne .fail3
    mov rdi, test_copy_buffer
    mov rsi, test_text_hello
    call fc_streq
    test eax, eax
    jz .fail4

    ; Unsigned 64-bit decimal conversion, including UINT64_MAX.
    mov rdi, 0FFFFFFFFFFFFFFFFh
    mov rsi, test_number_buffer
    mov edx, 21
    call fc_u64toa
    cmp rax, 20
    jne .fail5
    mov rdi, test_number_buffer
    mov rsi, test_uint64_max
    call fc_streq
    test eax, eax
    jz .fail6
    mov rdi, test_uint64_max
    call fc_atou64
    test rdx, rdx
    jz .fail7
    cmp rax, 0FFFFFFFFFFFFFFFFh
    jne .fail8

    ; UTF-16 -> UTF-8: ASCII, 2-byte, 3-byte and surrogate-pair code points.
    mov rdi, test_utf16
    mov rsi, test_utf8_buffer
    mov edx, 16
    call fc_utf16_to_utf8
    cmp rax, 10
    jne .fail9
    mov rdi, test_utf8_buffer
    mov rsi, test_utf8_expected
    call fc_streq
    test eax, eax
    jz .fail10

    ; Windows-style command-line tokenization with a quoted argument.
    mov rdi, test_command_line
    mov rsi, test_argv
    mov edx, 8
    mov rcx, test_arg_storage
    mov r8d, 128
    call fc_parse_argv
    cmp rax, 4
    jne .fail11
    mov rdi, [test_argv + 16]
    mov rsi, test_quoted_argument
    call fc_streq
    test eax, eax
    jz .fail12
    cmp qword [test_argv + 32], 0
    jne .fail13

    ; Explicit flatcore COMMAND form normalizes argv to COMMAND.
    mov edi, 4
    mov rsi, test_argv
    mov rdx, test_executable_name
    mov rcx, test_normalized_argc
    mov r8, test_normalized_argv
    call fc_prepare_command
    test eax, eax
    jnz .fail20
    cmp qword [test_normalized_argc], 3
    jne .fail14
    mov rax, [test_normalized_argv] ; normalized argv vector
    mov rax, [rax]                  ; normalized argv[0]
    cmp rax, [test_argv + 8]
    jne .fail15

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
    jnz .fail21
    cmp qword [test_normalized_argc], 2
    jne .fail16
    mov rax, [test_normalized_argv] ; normalized argv vector
    mov rdi, [rax]                  ; argv[0] points inside the original path
    mov rsi, test_multicall_command
    call fc_streq
    test eax, eax
    jz .fail17

    ; Table-driven dispatcher invokes the registered handler and returns status.
    mov rax, test_dispatch_name
    mov [test_dispatch_argv], rax
    mov edi, 1
    mov rsi, test_dispatch_argv
    mov rdx, test_registry
    mov ecx, 1
    call fc_dispatch
    cmp rax, 42
    jne .fail18

    ; Normalized error code maps to a stable message.
    mov edi, FC_ERR_ACCESS
    call fc_error_message
    mov rdi, rax
    mov rsi, test_error_access
    call fc_streq
    test eax, eax
    jz .fail19

    ; UTF-8 -> UTF-16 must round-trip ASCII, BMP, and supplementary points.
    mov rdi, test_utf8_expected
    mov rsi, test_utf16_roundtrip
    mov edx, 16
    call fc_utf8_to_utf16
    cmp rax, 5
    jne .fail22
    mov ax, word [test_utf16_roundtrip]
    cmp ax, [test_utf16]
    jne .fail22
    mov ax, word [test_utf16_roundtrip + 2]
    cmp ax, [test_utf16 + 2]
    jne .fail22
    mov ax, word [test_utf16_roundtrip + 4]
    cmp ax, [test_utf16 + 4]
    jne .fail22
    mov ax, word [test_utf16_roundtrip + 6]
    cmp ax, [test_utf16 + 6]
    jne .fail22
    mov ax, word [test_utf16_roundtrip + 8]
    cmp ax, [test_utf16 + 8]
    jne .fail22
    cmp word [test_utf16_roundtrip + 10], 0
    jne .fail22

    ; Overlong UTF-8 encodings must be rejected, not silently accepted.
    mov rdi, test_utf8_invalid
    mov rsi, test_utf16_roundtrip
    mov edx, 16
    call fc_utf8_to_utf16
    cmp rax, -1
    jne .fail23

    ; Truncated multibyte sequences must stop at the terminator safely.
    mov rdi, test_utf8_truncated
    mov rsi, test_utf16_roundtrip
    mov edx, 16
    call fc_utf8_to_utf16
    cmp rax, -1
    jne .fail24

    xor eax, eax
    jmp .return
.fail1:
    mov eax, 1
    jmp .return
.fail2:
    mov eax, 2
    jmp .return
.fail3:
    mov eax, 3
    jmp .return
.fail4:
    mov eax, 4
    jmp .return
.fail5:
    mov eax, 5
    jmp .return
.fail6:
    mov eax, 6
    jmp .return
.fail7:
    mov eax, 7
    jmp .return
.fail8:
    mov eax, 8
    jmp .return
.fail9:
    mov eax, 9
    jmp .return
.fail10:
    mov eax, 10
    jmp .return
.fail11:
    mov eax, 11
    jmp .return
.fail12:
    mov eax, 12
    jmp .return
.fail13:
    mov eax, 13
    jmp .return
.fail14:
    mov eax, 14
    jmp .return
.fail15:
    mov eax, 15
    jmp .return
.fail16:
    mov eax, 16
    jmp .return
.fail17:
    mov eax, 17
    jmp .return
.fail18:
    mov eax, 18
    jmp .return
.fail19:
    mov eax, 19
    jmp .return
.fail20:
    mov eax, 20
    jmp .return
.fail21:
    mov eax, 21
    jmp .return
.fail22:
    mov eax, 22
    jmp .return
.fail23:
    mov eax, 23
    jmp .return
.fail24:
    mov eax, 24
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

