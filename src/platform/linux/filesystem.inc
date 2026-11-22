; Linux x86-64 FC64 filesystem backend.
; Path arguments are NUL-terminated UTF-8 byte strings.

; os_open(path_utf8, flags, mode, out_handle)
os_open:
    mov r10, rdx
    xor eax, eax
    test esi, FC_OPEN_READ or FC_OPEN_WRITE
    jz .open_bad_flags
    mov eax, 2
    cmp esi, FC_OPEN_READ
    je .access_ready
    cmp esi, FC_OPEN_WRITE
    je .write_only
    cmp esi, FC_OPEN_READ or FC_OPEN_WRITE
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
    mov [rcx], rax
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
    mov rdx, rsi
    mov eax, 79
    syscall
    test rax, rax
    js .getcwd_error
    ; Linux getcwd returns the byte count including the NUL terminator.
    dec rax
    mov [rcx], rax
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
