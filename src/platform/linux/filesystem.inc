; Linux x86-64 FC64 filesystem backend.
; Path arguments are NUL-terminated UTF-8 byte strings.

; os_open(path_utf8, flags, mode, out_handle)
os_open:
    mov r9, rcx
    mov r10, rdx
    mov r11d, esi
    and r11d, FC_OPEN_READ or FC_OPEN_WRITE
    mov eax, 2
    cmp r11d, FC_OPEN_READ
    je .access_ready
    cmp r11d, FC_OPEN_WRITE
    je .write_only
    cmp r11d, FC_OPEN_READ or FC_OPEN_WRITE
    je .read_write
    jmp .open_bad_flags
.write_only:
    mov eax, 1
    jmp .access_ready
.read_write:
    mov eax, 2
.access_ready:
    test esi, FC_OPEN_CREATE
    jz .no_create
    or eax, 100o
.no_create:
    test esi, FC_OPEN_TRUNCATE
    jz .no_truncate
    or eax, 1000o
.no_truncate:
    test esi, FC_OPEN_APPEND
    jz .no_append
    or eax, 2000o
.no_append:
    test esi, FC_OPEN_EXCLUSIVE
    jz .no_exclusive
    or eax, 200o
.no_exclusive:
    test esi, FC_OPEN_DIRECTORY
    jz .no_directory
    or eax, 200000o
.no_directory:
    mov rsi, rax
    mov rax, 2
    syscall
    test rax, rax
    js .open_error
    mov [r9], rax
    xor eax, eax
    ret
.open_bad_flags:
    mov eax, 22
    jmp fc_linux_set_error
.open_error:
    neg eax
    jmp fc_linux_set_error

; os_mkdir(path_utf8, mode)
os_mkdir:
    mov eax, 83
    syscall
    test rax, rax
    js .mkdir_error
    xor eax, eax
    ret
.mkdir_error:
    neg eax
    jmp fc_linux_set_error

; os_rmdir(path_utf8)
os_rmdir:
    mov eax, 84
    syscall
    test rax, rax
    js .rmdir_error
    xor eax, eax
    ret
.rmdir_error:
    neg eax
    jmp fc_linux_set_error

; os_unlink(path_utf8)
os_unlink:
    mov eax, 87
    syscall
    test rax, rax
    js .unlink_error
    xor eax, eax
    ret
.unlink_error:
    neg eax
    jmp fc_linux_set_error

; os_rename(old_path_utf8, new_path_utf8)
os_rename:
    mov eax, 82
    syscall
    test rax, rax
    js .rename_error
    xor eax, eax
    ret
.rename_error:
    neg eax
    jmp fc_linux_set_error

; os_getcwd(buffer, capacity, out_length)
os_getcwd:
    mov r8, rdx
    mov rdx, rsi
    mov eax, 79
    syscall
    test rax, rax
    js .getcwd_error
    ; Linux getcwd returns the byte count including the NUL terminator.
    dec rax
    mov [r8], rax
    xor eax, eax
    ret
.getcwd_error:
    neg eax
    jmp fc_linux_set_error

; os_chdir(path_utf8)
os_chdir:
    mov eax, 80
    syscall
    test rax, rax
    js .chdir_error
    xor eax, eax
    ret
.chdir_error:
    neg eax
    jmp fc_linux_set_error

; os_stat(path_utf8, out_fc_file_info)
; Linux x86-64 struct stat is collected with newfstatat(AT_FDCWD, ...).
os_stat:
    mov r8, rsi
    mov rsi, rdi
    mov edi, -100
    lea rdx, [linux_stat_buffer]
    mov r9, rdx
    mov r10d, 100h             ; AT_SYMLINK_NOFOLLOW (lstat semantics)
    mov eax, 262
    syscall
    test rax, rax
    js .stat_error

    ; Clear the complete public structure before populating supported fields.
    mov rdi, r8
    xor eax, eax
    mov ecx, 8
    rep stosq

    mov rax, [r9 + 48]
    mov [r8], rax

    ; st_mtime seconds/nanoseconds -> Unix epoch nanoseconds.
    mov rax, [r9 + 88]
    imul rax, 1000000000
    add rax, [r9 + 96]
    mov [r8 + 8], rax

    ; st_atime seconds/nanoseconds -> Unix epoch nanoseconds.
    mov rax, [r9 + 72]
    imul rax, 1000000000
    add rax, [r9 + 80]
    mov [r8 + 24], rax

    mov eax, [r9 + 24]
    mov [r8 + 40], eax
    mov eax, [r9 + 16]
    mov [r8 + 44], eax

    ; Translate the POSIX file type from st_mode.
    mov eax, [r9 + 24]
    and eax, 170000o
    cmp eax, 100000o
    je .stat_regular
    cmp eax, 040000o
    je .stat_directory
    cmp eax, 120000o
    je .stat_symlink
    cmp eax, 010000o
    je .stat_fifo
    cmp eax, 140000o
    je .stat_socket
    cmp eax, 020000o
    je .stat_device
    cmp eax, 060000o
    je .stat_device
    jmp .stat_attributes
.stat_regular:
    mov dword [r8 + 36], FC_TYPE_REGULAR
    jmp .stat_attributes
.stat_directory:
    mov dword [r8 + 36], FC_TYPE_DIRECTORY
    jmp .stat_attributes
.stat_symlink:
    mov dword [r8 + 36], FC_TYPE_SYMLINK
    jmp .stat_attributes
.stat_fifo:
    mov dword [r8 + 36], FC_TYPE_FIFO
    jmp .stat_attributes
.stat_socket:
    mov dword [r8 + 36], FC_TYPE_SOCKET
    jmp .stat_attributes
.stat_device:
    mov dword [r8 + 36], FC_TYPE_DEVICE
.stat_attributes:
    ; Hidden is a shared convention: the final path component begins with '.'.
    ; A trailing slash does not create an empty basename.
    mov rdx, rsi
    mov rax, rsi
.stat_hidden_scan:
    mov cl, [rdx]
    test cl, cl
    jz .stat_hidden_check
    cmp cl, '/'
    jne .stat_hidden_next
    cmp byte [rdx + 1], 0
    je .stat_hidden_next
    lea rax, [rdx + 1]
.stat_hidden_next:
    inc rdx
    jmp .stat_hidden_scan
.stat_hidden_check:
    cmp byte [rax], '.'
    jne .stat_readonly
    or dword [r8 + 32], FC_ATTR_HIDDEN
.stat_readonly:
    mov eax, [r9 + 24]
    test eax, 0222o
    jnz .stat_done
    or dword [r8 + 32], FC_ATTR_READ_ONLY
.stat_done:
    xor eax, eax
    ret
.stat_error:
    neg eax
    jmp fc_linux_set_error


; os_set_times(path_utf8, accessed_ns, modified_ns)
; Set access and modification times using UTC Unix-epoch nanoseconds.
; Convert each signed nanosecond value to a normalized Linux timespec.
os_set_times:
    push r12
    push r13
    push r14
    sub rsp, 32
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx

    ; atime: signed nanoseconds -> seconds + nonnegative nanosecond remainder.
    mov rax, r13
    cqo
    mov rcx, 1000000000
    idiv rcx
    test rdx, rdx
    jns .atime_normalized
    dec rax
    add rdx, 1000000000
.atime_normalized:
    mov [rsp], rax
    mov [rsp + 8], rdx

    ; mtime: signed nanoseconds -> seconds + nonnegative nanosecond remainder.
    mov rax, r14
    cqo
    mov rcx, 1000000000
    idiv rcx
    test rdx, rdx
    jns .mtime_normalized
    dec rax
    add rdx, 1000000000
.mtime_normalized:
    mov [rsp + 16], rax
    mov [rsp + 24], rdx

    ; utimensat(AT_FDCWD, path, times, 0).
    mov edi, -100
    mov rsi, r12
    mov rdx, rsp
    xor r10d, r10d
    mov eax, 280
    syscall
    test rax, rax
    js .set_times_error
    xor eax, eax
    jmp .set_times_done
.set_times_error:
    neg eax
    call fc_linux_set_error
.set_times_done:
    add rsp, 32
    pop r14
    pop r13
    pop r12
    ret

; os_get_capabilities()
; Linux exposes no-follow symlink metadata, complete POSIX st_mode, and
; nanosecond file timestamp updates through the operations in this backend.
os_get_capabilities:
    mov eax, FC_CAP_SYMLINKS or FC_CAP_POSIX_MODES or FC_CAP_FILE_TIMES
    ret
