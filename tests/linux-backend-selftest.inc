; Linux FC64 backend integration checks.
; Returns zero on success; nonzero values identify the failing operation.
fc_linux_backend_selftest:
    push rbx
    push r12
    push r13
    sub rsp, 16

    ; getcwd must return a NUL-terminated path and its byte length.
    mov rdi, linux_test_cwd
    mov esi, 4096
    mov rdx, linux_test_length
    call os_getcwd
    test eax, eax
    jnz .fail1
    cmp qword [linux_test_length], 0
    je .fail1

    ; Create/truncate a file, write a short payload, and close it.
    mov rdi, linux_test_path
    mov esi, FC_OPEN_WRITE or FC_OPEN_CREATE or FC_OPEN_TRUNCATE
    mov edx, 600o
    mov rcx, linux_test_handle
    call os_open
    test eax, eax
    jnz .fail2
    mov rdi, [linux_test_handle]
    mov rsi, linux_test_payload
    mov edx, 2
    mov rcx, linux_test_length
    call os_write
    test eax, eax
    jnz .fail3
    cmp qword [linux_test_length], 2
    jne .fail3
    mov rdi, [linux_test_handle]
    call os_close
    test eax, eax
    jnz .fail4


    ; Stat must report the file size, type, and POSIX mode.
    mov rdi, linux_test_path
    mov rsi, linux_test_info
    call os_stat
    test eax, eax
    jnz .fail10
    cmp qword [linux_test_info], 2
    jne .fail11
    cmp dword [linux_test_info + 36], FC_TYPE_REGULAR
    jne .fail12
    ; FC_FILE_INFO.mode retains the complete POSIX st_mode, including S_IFREG.
    cmp dword [linux_test_info + 40], 100600o
    jne .fail13

    ; Set and read back distinct nanosecond timestamps.
    mov rdi, linux_test_path
    mov rsi, 1700000000123456789
    mov rdx, 1800000000987654321
    call os_set_times
    test eax, eax
    jnz .fail23
    mov rdi, linux_test_path
    mov rsi, linux_test_info
    call os_stat
    test eax, eax
    jnz .fail24
    cmp qword [linux_test_info + 24], 1700000000123456789
    jne .fail25
    cmp qword [linux_test_info + 8], 1800000000987654321
    jne .fail26

    ; Use a dedicated empty directory so entry/EOF behavior is deterministic.
    mov rdi, linux_test_dir_path
    call os_rmdir
    mov rdi, linux_test_dir_path
    mov esi, 700o
    call os_mkdir
    test eax, eax
    jnz .fail14

    ; Two simultaneous directory handles must keep independent iteration state.
    mov rdi, linux_test_dir_path
    mov rsi, linux_test_dir_handle
    call os_dir_open
    test eax, eax
    jnz .fail14
    mov rdi, linux_test_dir_path
    mov rsi, linux_test_dir_handle2
    call os_dir_open
    test eax, eax
    jnz .fail15

    ; A too-small name buffer must fail without consuming the current entry.
    mov rdi, [linux_test_dir_handle]
    mov rsi, linux_test_info
    mov rdx, linux_test_name
    xor ecx, ecx
    mov r8, linux_test_name_length
    call os_dir_next
    cmp eax, -1
    jne .fail16
    cmp dword [os_error_code], FC_ERR_TOO_SMALL
    jne .fail16

    ; Retrying with sufficient capacity must return that same entry.
    mov rdi, [linux_test_dir_handle]
    mov rsi, linux_test_info
    mov rdx, linux_test_name
    mov ecx, 256
    mov r9, linux_test_name_length
    call os_dir_next
    cmp eax, 1
    jne .fail16
    cmp qword [linux_test_name_length], 0
    je .fail17

    mov rdi, [linux_test_dir_handle2]
    mov rsi, linux_test_info2
    mov rdx, linux_test_name2
    mov ecx, 256
    mov r8, linux_test_name_length2
    call os_dir_next
    cmp eax, 1
    jne .fail18
    cmp qword [linux_test_name_length2], 0
    je .fail19

    ; Drain the first iterator and require an explicit EOF result.
    mov ebx, 8
.drain_directory:
    mov rdi, [linux_test_dir_handle]
    mov rsi, linux_test_info
    mov rdx, linux_test_name
    mov ecx, 256
    mov r9, linux_test_name_length
    call os_dir_next
    test eax, eax
    jz .directory_eof
    cmp eax, 1
    jne .fail20
    dec ebx
    jnz .drain_directory
    jmp .fail20
.directory_eof:
    mov rdi, [linux_test_dir_handle]
    call os_dir_close
    test eax, eax
    jnz .fail20
    mov rdi, [linux_test_dir_handle2]
    call os_dir_close
    test eax, eax
    jnz .fail21
    mov rdi, linux_test_dir_path
    call os_rmdir
    test eax, eax
    jnz .fail22

    ; Reopen and read back the exact payload.
    mov rdi, linux_test_path
    mov esi, FC_OPEN_READ
    xor edx, edx
    mov rcx, linux_test_handle
    call os_open
    test eax, eax
    jnz .fail5
    mov rdi, [linux_test_handle]
    mov rsi, linux_test_readback
    mov edx, 2
    mov rcx, linux_test_length
    call os_read
    test eax, eax
    jnz .fail6
    cmp qword [linux_test_length], 2
    jne .fail6
    mov rdi, [linux_test_handle]
    call os_close
    test eax, eax
    jnz .fail7
    mov al, [linux_test_readback]
    cmp al, 'O'
    jne .fail8
    mov al, [linux_test_readback + 1]
    cmp al, 'K'
    jne .fail8

    ; Remove the fixture and verify the operation succeeds.
    mov rdi, linux_test_path
    call os_unlink
    test eax, eax
    jnz .fail9

    xor eax, eax
    jmp .done
.fail1: mov eax, 31
    jmp .done
.fail2: mov eax, 32
    jmp .done
.fail3:
    mov rdi, [linux_test_handle]
    call os_close
    mov eax, 33
    jmp .done
.fail4: mov eax, 34
    jmp .done
.fail5: mov eax, 35
    jmp .done
.fail6:
    mov rdi, [linux_test_handle]
    call os_close
    mov eax, 36
    jmp .done
.fail7: mov eax, 37
    jmp .done
.fail8: mov eax, 38
    jmp .done
.fail9: mov eax, 39
    jmp .done
.fail10: mov eax, 40
    jmp .done
.fail11: mov eax, 41
    jmp .done
.fail12: mov eax, 42
    jmp .done
.fail13: mov eax, 43
    jmp .done
.fail14: mov eax, 44
    jmp .done
.fail15:
    mov rdi, [linux_test_dir_handle]
    call os_dir_close
    mov eax, 45
    jmp .done
.fail16:
    mov rdi, [linux_test_dir_handle]
    call os_dir_close
    mov rdi, [linux_test_dir_handle2]
    call os_dir_close
    mov eax, 46
    jmp .done
.fail17:
    mov rdi, [linux_test_dir_handle]
    call os_dir_close
    mov rdi, [linux_test_dir_handle2]
    call os_dir_close
    mov eax, 47
    jmp .done
.fail18:
    mov rdi, [linux_test_dir_handle]
    call os_dir_close
    mov rdi, [linux_test_dir_handle2]
    call os_dir_close
    mov eax, 48
    jmp .done
.fail19:
    mov rdi, [linux_test_dir_handle]
    call os_dir_close
    mov rdi, [linux_test_dir_handle2]
    call os_dir_close
    mov eax, 49
    jmp .done
.fail20:
    mov rdi, [linux_test_dir_handle2]
    call os_dir_close
    mov eax, 50
    jmp .done
.fail21: mov eax, 51
    jmp .done
.fail22: mov eax, 52
    jmp .done
.fail23: mov eax, 53
    jmp .done
.fail24: mov eax, 54
    jmp .done
.fail25: mov eax, 55
    jmp .done
.fail26: mov eax, 56
.done:
    add rsp, 16
    pop r13
    pop r12
    pop rbx
    ret
