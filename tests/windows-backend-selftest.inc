; Windows Unicode backend integration checks.
; Exercises UTF-8 paths through the Win32 W API boundary.
fc_windows_backend_selftest:
    push rbx
    push r12
    push r13
    sub rsp, 16

    ; Remove leftovers from an interrupted earlier run; these failures are
    ; intentionally ignored so the fixture remains repeatable.
    mov rdi, windows_test_path
    call os_unlink
    mov rdi, windows_test_renamed_path
    call os_unlink
    mov rdi, windows_test_dir
    call os_rmdir

    ; Preserve the original working directory for the chdir round-trip.
    mov rdi, windows_test_cwd
    mov esi, 4096
    mov rdx, windows_test_length
    call os_getcwd
    test eax, eax
    jnz .fail1
    cmp qword [windows_test_length], 0
    je .fail1

    ; Create a file whose path contains non-ASCII UTF-8 characters.
    mov rdi, windows_test_path
    mov esi, FC_OPEN_WRITE or FC_OPEN_CREATE or FC_OPEN_TRUNCATE
    mov edx, 600o
    mov rcx, windows_test_handle
    call os_open
    test eax, eax
    jnz .fail2
    mov rdi, [windows_test_handle]
    mov rsi, windows_test_payload
    mov edx, 2
    mov rcx, windows_test_length
    call os_write
    test eax, eax
    jnz .fail3
    cmp qword [windows_test_length], 2
    jne .fail3
    mov rdi, [windows_test_handle]
    call os_close
    test eax, eax
    jnz .fail4

    ; Read the same bytes back through the Unicode path.
    mov rdi, windows_test_path
    mov esi, FC_OPEN_READ
    xor edx, edx
    mov rcx, windows_test_handle
    call os_open
    test eax, eax
    jnz .fail5
    mov rdi, [windows_test_handle]
    mov rsi, windows_test_readback
    mov edx, 2
    mov rcx, windows_test_length
    call os_read
    test eax, eax
    jnz .fail6
    cmp qword [windows_test_length], 2
    jne .fail6
    mov rdi, [windows_test_handle]
    call os_close
    test eax, eax
    jnz .fail7
    cmp byte [windows_test_readback], 'O'
    jne .fail8
    cmp byte [windows_test_readback + 1], 'K'
    jne .fail8

    ; Rename and remove the Unicode filename.
    mov rdi, windows_test_path
    mov rsi, windows_test_renamed_path
    call os_rename
    test eax, eax
    jnz .fail9
    mov rdi, windows_test_renamed_path
    call os_unlink
    test eax, eax
    jnz .fail10

    ; Exercise Unicode directory creation, chdir, restoration, and removal.
    mov rdi, windows_test_dir
    mov esi, 700o
    call os_mkdir
    test eax, eax
    jnz .fail11
    mov rdi, windows_test_dir
    call os_chdir
    test eax, eax
    jnz .fail12
    mov rdi, windows_test_cwd
    call os_chdir
    test eax, eax
    jnz .fail13
    mov rdi, windows_test_dir
    call os_rmdir
    test eax, eax
    jnz .fail14

    xor eax, eax
    jmp .done
.fail1: mov eax, 61
    jmp .done
.fail2: mov eax, 62
    jmp .done
.fail3:
    mov rdi, [windows_test_handle]
    call os_close
    mov eax, [os_native_error]
    and eax, 0FFFFh
    add eax, 10000h
    jmp .done
.fail4: mov eax, 64
    jmp .done
.fail5: mov eax, 65
    jmp .done
.fail6:
    mov rdi, [windows_test_handle]
    call os_close
    mov eax, 66
    jmp .done
.fail7: mov eax, 67
    jmp .done
.fail8: mov eax, 68
    jmp .done
.fail9: mov eax, 69
    jmp .done
.fail10: mov eax, 70
    jmp .done
.fail11: mov eax, 71
    jmp .done
.fail12: mov eax, 72
    jmp .done
.fail13: mov eax, 73
    jmp .done
.fail14: mov eax, 74
.done:
    add rsp, 16
    pop r13
    pop r12
    pop rbx
    ret
