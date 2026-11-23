; Windows x86-64 FC64 filesystem backend.
; All public paths are UTF-8. Convert to UTF-16 and call only W-suffixed APIs.

; os_open(path_utf8, flags, mode, out_handle)
os_open:
    push r12
    push r13
    push r14
    sub rsp, 64
    mov r12, rdi
    mov r13, rcx
    mov r14d, esi
    mov rdi, r12
    mov rsi, windows_path_buffer
    mov edx, 32768
    call fc_utf8_to_utf16
    cmp rax, -1
    je .open_encoding
    mov eax, r14d
    and eax, FC_OPEN_READ or FC_OPEN_WRITE
    test eax, eax
    jz .open_invalid

    ; Map Flatcore access and append flags to Win32 desired access.
    xor r10d, r10d
    test r14d, FC_OPEN_READ
    jz .open_no_read
    or r10d, 80000000h
.open_no_read:
    test r14d, FC_OPEN_WRITE
    jz .open_no_write
    or r10d, 40000000h
.open_no_write:
    test r14d, FC_OPEN_APPEND
    jz .open_access_ready
    and r10d, 0BFFFFFFFh
    or r10d, 00000004h       ; FILE_APPEND_DATA
.open_access_ready:
    mov rcx, windows_path_buffer
    mov edx, r10d
    mov r8d, 7               ; share read/write/delete
    xor r9d, r9d             ; default security
    mov eax, 3               ; OPEN_EXISTING
    test r14d, FC_OPEN_CREATE
    jz .open_no_create
    test r14d, FC_OPEN_EXCLUSIVE
    jnz .open_create_new
    test r14d, FC_OPEN_TRUNCATE
    jnz .open_create_always
    mov eax, 4               ; OPEN_ALWAYS
    jmp .open_no_create
.open_create_new:
    mov eax, 1               ; CREATE_NEW
    jmp .open_no_create
.open_create_always:
    mov eax, 2               ; CREATE_ALWAYS
.open_no_create:
    test r14d, FC_OPEN_TRUNCATE
    jz .open_flags
    test r14d, FC_OPEN_CREATE
    jnz .open_flags
    mov eax, 5               ; TRUNCATE_EXISTING
.open_flags:
    mov [rsp + 32], rax
    mov eax, 80h             ; FILE_ATTRIBUTE_NORMAL
    test r14d, FC_OPEN_DIRECTORY
    jz .open_store_flags
    or eax, 02000000h        ; FILE_FLAG_BACKUP_SEMANTICS
.open_store_flags:
    mov [rsp + 40], rax
    mov qword [rsp + 48], 0
    call [CreateFileW]
    cmp rax, -1
    je .open_error
    mov [r13], rax
    xor eax, eax
    jmp .open_done
.open_encoding:
    mov eax, FC_ERR_ENCODING
    call fc_windows_set_normalized_error
    jmp .open_done
.open_invalid:
    mov eax, FC_ERR_INVALID
    call fc_windows_set_normalized_error
    jmp .open_done
.open_error:
    call fc_windows_set_error
.open_done:
    add rsp, 64
    pop r14
    pop r13
    pop r12
    ret

; os_mkdir(path_utf8, mode). Windows does not expose POSIX creation modes.
os_mkdir:
    push r12
    sub rsp, 32
    mov r12, rdi
    mov rdi, r12
    mov rsi, windows_path_buffer
    mov edx, 32768
    call fc_utf8_to_utf16
    cmp rax, -1
    je .mkdir_encoding
    mov rcx, windows_path_buffer
    xor edx, edx
    call [CreateDirectoryW]
    test eax, eax
    jz .mkdir_error
    xor eax, eax
    jmp .mkdir_done
.mkdir_encoding:
    mov eax, FC_ERR_ENCODING
    call fc_windows_set_normalized_error
    jmp .mkdir_done
.mkdir_error:
    call fc_windows_set_error
.mkdir_done:
    add rsp, 32
    pop r12
    ret

; os_rmdir(path_utf8)
os_rmdir:
    push r12
    sub rsp, 32
    mov r12, rdi
    mov rdi, r12
    mov rsi, windows_path_buffer
    mov edx, 32768
    call fc_utf8_to_utf16
    cmp rax, -1
    je .rmdir_encoding
    mov rcx, windows_path_buffer
    call [RemoveDirectoryW]
    test eax, eax
    jz .rmdir_error
    xor eax, eax
    jmp .rmdir_done
.rmdir_encoding:
    mov eax, FC_ERR_ENCODING
    call fc_windows_set_normalized_error
    jmp .rmdir_done
.rmdir_error:
    call fc_windows_set_error
.rmdir_done:
    add rsp, 32
    pop r12
    ret

; os_unlink(path_utf8)
os_unlink:
    push r12
    sub rsp, 32
    mov r12, rdi
    mov rdi, r12
    mov rsi, windows_path_buffer
    mov edx, 32768
    call fc_utf8_to_utf16
    cmp rax, -1
    je .unlink_encoding
    mov rcx, windows_path_buffer
    call [DeleteFileW]
    test eax, eax
    jz .unlink_error
    xor eax, eax
    jmp .unlink_done
.unlink_encoding:
    mov eax, FC_ERR_ENCODING
    call fc_windows_set_normalized_error
    jmp .unlink_done
.unlink_error:
    call fc_windows_set_error
.unlink_done:
    add rsp, 32
    pop r12
    ret

; os_rename(old_path_utf8, new_path_utf8)
os_rename:
    push r12
    push r13
    sub rsp, 72
    mov r12, rdi
    mov r13, rsi
    mov rdi, r12
    mov rsi, windows_path_buffer
    mov edx, 32768
    call fc_utf8_to_utf16
    cmp rax, -1
    je .rename_encoding
    mov rdi, r13
    mov rsi, windows_path_buffer2
    mov edx, 32768
    call fc_utf8_to_utf16
    cmp rax, -1
    je .rename_encoding
    mov rcx, windows_path_buffer
    mov rdx, windows_path_buffer2
    call [MoveFileW]
    test eax, eax
    jz .rename_error
    xor eax, eax
    jmp .rename_done
.rename_encoding:
    mov eax, FC_ERR_ENCODING
    call fc_windows_set_normalized_error
    jmp .rename_done
.rename_error:
    call fc_windows_set_error
.rename_done:
    add rsp, 72
    pop r13
    pop r12
    ret

; os_getcwd(buffer_utf8, capacity, out_length)
os_getcwd:
    push r12
    push r13
    push r14
    sub rsp, 32
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
    mov ecx, 32768
    mov rdx, windows_path_buffer
    call [GetCurrentDirectoryW]
    test eax, eax
    jz .getcwd_error
    cmp eax, 32768
    jae .getcwd_too_small
    mov rdi, windows_path_buffer
    mov rsi, r12
    mov rdx, r13
    call fc_utf16_to_utf8
    cmp rax, -1
    je .getcwd_too_small
    mov [r14], rax
    xor eax, eax
    jmp .getcwd_done
.getcwd_error:
    call fc_windows_set_error
    jmp .getcwd_done
.getcwd_too_small:
    mov eax, FC_ERR_TOO_SMALL
    call fc_windows_set_normalized_error
.getcwd_done:
    add rsp, 32
    pop r14
    pop r13
    pop r12
    ret

; os_chdir(path_utf8)
os_chdir:
    push r12
    sub rsp, 32
    mov r12, rdi
    mov rdi, r12
    mov rsi, windows_path_buffer
    mov edx, 32768
    call fc_utf8_to_utf16
    cmp rax, -1
    je .chdir_encoding
    mov rcx, windows_path_buffer
    call [SetCurrentDirectoryW]
    test eax, eax
    jz .chdir_error
    xor eax, eax
    jmp .chdir_done
.chdir_encoding:
    mov eax, FC_ERR_ENCODING
    call fc_windows_set_normalized_error
    jmp .chdir_done
.chdir_error:
    call fc_windows_set_error
.chdir_done:
    add rsp, 32
    pop r12
    ret

; os_stat(path_utf8, out_fc_file_info)
; Query Win32 file metadata and translate FILETIME to Unix-epoch nanoseconds.
; Windows does not expose POSIX mode bits or a portable link count here, so
; those FC_FILE_INFO fields remain zero. Reparse points are left UNKNOWN
; rather than incorrectly classifying every reparse tag as a symbolic link.
os_stat:
    push r12
    push r13
    sub rsp, 40
    mov r12, rsi
    mov rdi, rdi
    mov rsi, windows_path_buffer
    mov edx, 32768
    call fc_utf8_to_utf16
    cmp rax, -1
    je .stat_encoding

    mov rcx, windows_path_buffer
    xor edx, edx                 ; GetFileExInfoStandard
    mov r8, windows_file_data
    call [GetFileAttributesExW]
    test eax, eax
    jz .stat_error

    ; Clear all fields so unavailable POSIX metadata is deterministically zero.
    mov rdi, r12
    xor eax, eax
    mov ecx, 8
    rep stosq

    mov r13, windows_file_data
    ; 64-bit file size = (high << 32) | low.
    mov eax, [r13 + 28]
    shl rax, 32
    mov eax, [r13 + 32]
    mov [r12], rax

    ; Creation FILETIME (100 ns since 1601) -> Unix nanoseconds.
    mov rax, [r13 + 4]
    sub rax, 116444736000000000
    imul rax, 100
    mov [r12 + 16], rax

    ; Last-write FILETIME -> modified_ns.
    mov rax, [r13 + 20]
    sub rax, 116444736000000000
    imul rax, 100
    mov [r12 + 8], rax

    ; Last-access FILETIME -> accessed_ns.
    mov rax, [r13 + 12]
    sub rax, 116444736000000000
    imul rax, 100
    mov [r12 + 24], rax

    mov eax, [r13]
    ; Translate the common DOS attribute bits.
    test eax, 2                  ; FILE_ATTRIBUTE_HIDDEN
    jz .stat_not_hidden
    or dword [r12 + 32], FC_ATTR_HIDDEN
.stat_not_hidden:
    test eax, 1                  ; FILE_ATTRIBUTE_READONLY
    jz .stat_not_readonly
    or dword [r12 + 32], FC_ATTR_READ_ONLY
.stat_not_readonly:
    test eax, 4                  ; FILE_ATTRIBUTE_SYSTEM
    jz .stat_not_system
    or dword [r12 + 32], FC_ATTR_SYSTEM
.stat_not_system:
    test eax, 20h                ; FILE_ATTRIBUTE_ARCHIVE
    jz .stat_type
    or dword [r12 + 32], FC_ATTR_ARCHIVE

.stat_type:
    test eax, 400h               ; FILE_ATTRIBUTE_REPARSE_POINT
    jnz .stat_done               ; tag-specific classification is not available
    test eax, 10h                ; FILE_ATTRIBUTE_DIRECTORY
    jz .stat_regular
    mov dword [r12 + 36], FC_TYPE_DIRECTORY
    jmp .stat_done
.stat_regular:
    mov dword [r12 + 36], FC_TYPE_REGULAR
.stat_done:
    xor eax, eax
    jmp .stat_return
.stat_encoding:
    mov eax, FC_ERR_ENCODING
    call fc_windows_set_normalized_error
    jmp .stat_return
.stat_error:
    call fc_windows_set_error
.stat_return:
    add rsp, 40
    pop r13
    pop r12
    ret

; Capability reporting remains conservative until the optional operations land.
os_get_capabilities:
    xor eax, eax
    ret
