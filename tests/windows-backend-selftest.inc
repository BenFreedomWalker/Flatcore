; Windows Unicode backend integration checks.
; Exercises UTF-8 paths through the Win32 W API boundary.
fc_windows_backend_selftest:
    push rbx
    push r12
    push r13
    sub rsp, 16

    ; Verify Windows advertises only implemented backend semantics.
    call os_get_capabilities
    cmp eax, FC_CAP_SYMLINKS or FC_CAP_FILE_TIMES
    jne .fail99

    ; Remove leftovers from an interrupted earlier run; these failures are
    ; intentionally ignored so the fixture remains repeatable.
    mov rdi, windows_test_path
    call os_unlink
    mov rdi, windows_test_renamed_path
    call os_unlink
    mov rdi, windows_test_entry_path
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

    ; Set distinct access and modification timestamps at FILETIME precision.
    mov rdi, windows_test_path
    mov rsi, 1700000000234567800
    mov rdx, 1700000000123456700
    call os_set_times
    test eax, eax
    jnz .fail96

    ; Query metadata through the Unicode Win32 backend while the file exists.
    mov rdi, windows_test_path
    mov rsi, windows_test_info
    call os_stat
    test eax, eax
    jnz .fail15
    cmp qword [windows_test_info], 2
    jne .fail16
    cmp dword [windows_test_info + 36], FC_TYPE_REGULAR
    jne .fail17
    ; POSIX mode bits are not representable by this Windows metadata query.
    cmp dword [windows_test_info + 40], 0
    jne .fail18
    mov rax, 1700000000123456700
    cmp qword [windows_test_info + 8], rax
    jne .fail97
    mov rax, 1700000000234567800
    cmp qword [windows_test_info + 24], rax
    jne .fail98

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
    ; Create a Unicode-named entry inside the test directory.
    mov rdi, windows_test_entry_path
    mov esi, FC_OPEN_WRITE or FC_OPEN_CREATE or FC_OPEN_TRUNCATE
    mov edx, 600o
    mov rcx, windows_test_handle
    call os_open
    test eax, eax
    jnz .fail79
    mov rdi, [windows_test_handle]
    mov rsi, windows_test_payload
    mov edx, 2
    mov rcx, windows_test_length
    call os_write
    test eax, eax
    jnz .fail80
    mov rdi, [windows_test_handle]
    call os_close
    test eax, eax
    jnz .fail81

    ; Open two independent iterators over the same Unicode directory.
    mov rdi, windows_test_dir
    mov rsi, windows_test_dir_handle1
    call os_dir_open
    test eax, eax
    jnz .fail82
    mov rdi, windows_test_dir
    mov rsi, windows_test_dir_handle2
    call os_dir_open
    test eax, eax
    jnz .fail83

    ; A zero-byte name buffer must fail without consuming the current record.
    mov rdi, [windows_test_dir_handle1]
    mov rsi, windows_test_info
    mov rdx, windows_test_dir_name
    xor ecx, ecx
    mov r8, windows_test_dir_name_length
    call os_dir_next
    cmp rax, -1
    jne .fail84
    cmp dword [os_error_code], FC_ERR_TOO_SMALL
    jne .fail84

    ; Retry the same record with sufficient capacity.
    mov rdi, [windows_test_dir_handle1]
    mov rsi, windows_test_info
    mov rdx, windows_test_dir_name
    mov ecx, 4096
    mov r8, windows_test_dir_name_length
    call os_dir_next
    cmp rax, 1
    jne .fail85

    ; The second iterator starts independently at the first directory entry.
    mov rdi, [windows_test_dir_handle2]
    mov rsi, windows_test_info2
    mov rdx, windows_test_dir_name2
    mov ecx, 4096
    mov r8, windows_test_dir_name_length2
    call os_dir_next
    cmp rax, 1
    jne .fail86

    ; Drain iterator one and find the UTF-8 filename created above.
    mov dword [windows_test_dir_found], 0
.dir_drain:
    mov rdi, [windows_test_dir_handle1]
    mov rsi, windows_test_info
    mov rdx, windows_test_dir_name
    mov ecx, 4096
    mov r8, windows_test_dir_name_length
    call os_dir_next
    cmp rax, 1
    jne .dir_drain_end
    mov rdi, windows_test_dir_name
    mov rsi, windows_test_entry_name
    call fc_streq
    test eax, eax
    jz .dir_drain
    cmp qword [windows_test_info], 2
    jne .fail87
    cmp dword [windows_test_info + 36], FC_TYPE_REGULAR
    jne .fail88
    mov dword [windows_test_dir_found], 1
    jmp .dir_drain

.dir_drain_end:
    test rax, rax
    jnz .fail89
    cmp dword [windows_test_dir_found], 1
    jne .fail90

    ; Iterator two must still have another entry after iterator one reached EOF.
    mov rdi, [windows_test_dir_handle2]
    mov rsi, windows_test_info2
    mov rdx, windows_test_dir_name2
    mov ecx, 4096
    mov r8, windows_test_dir_name_length2
    call os_dir_next
    cmp rax, 1
    jne .fail91

    mov rdi, [windows_test_dir_handle1]
    call os_dir_close
    test eax, eax
    jnz .fail92
    mov rdi, [windows_test_dir_handle2]
    call os_dir_close
    test eax, eax
    jnz .fail93

    mov rdi, windows_test_entry_path
    call os_unlink
    test eax, eax
    jnz .fail94
    mov rdi, windows_test_dir
    call os_rmdir
    test eax, eax
    jnz .fail95

    xor eax, eax
    jmp .done
.fail1: mov eax, 61
    jmp .done
.fail2: mov eax, 62
    jmp .done
.fail3:
    mov rdi, [windows_test_handle]
    call os_close
    mov rax, [os_native_error]
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
    jmp .done
.fail15: mov eax, 75
    jmp .done
.fail16: mov eax, 76
    jmp .done
.fail17: mov eax, 77
    jmp .done
.fail18: mov eax, 78
    jmp .done
.fail79: mov eax, 79
    jmp .done
.fail80:
    mov rdi, [windows_test_handle]
    call os_close
    mov eax, 80
    jmp .done
.fail81: mov eax, 81
    jmp .done
.fail82: mov eax, 82
    jmp .done
.fail83: mov eax, 83
    jmp .done
.fail84: mov eax, 84
    jmp .done
.fail85: mov eax, 85
    jmp .done
.fail86: mov eax, 86
    jmp .done
.fail87: mov eax, 87
    jmp .done
.fail88: mov eax, 88
    jmp .done
.fail89: mov eax, 89
    jmp .done
.fail90: mov eax, 90
    jmp .done
.fail91: mov eax, 91
    jmp .done
.fail92: mov eax, 92
    jmp .done
.fail93: mov eax, 93
    jmp .done
.fail94: mov eax, 94
    jmp .done
.fail95: mov eax, 95
    jmp .done
.fail96: mov eax, 96
    jmp .done
.fail97: mov eax, 97
    jmp .done
.fail98: mov eax, 98
    jmp .done
.fail99: mov eax, 99
.done:
    add rsp, 16
    pop r13
    pop r12
    pop rbx
    ret
