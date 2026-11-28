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
    test r14d, FC_OPEN_DIRECTORY
    jz .open_store_handle
    ; FILE_FLAG_BACKUP_SEMANTICS permits opening directories but does not
    ; reject regular files. Enforce the FC_OPEN_DIRECTORY contract explicitly.
    mov [rsp + 56], rax
    mov rcx, rax
    lea rdx, [windows_stat_find_data]
    call [GetFileInfoByHandle]
    test eax, eax
    jz .open_not_directory
    test dword [windows_stat_find_data], 10h ; FILE_ATTRIBUTE_DIRECTORY
    jz .open_not_directory
    mov rax, [rsp + 56]
.open_store_handle:
    mov [r13], rax
    xor eax, eax
    jmp .open_done
.open_not_directory:
    mov rcx, [rsp + 56]
    call [CloseHandle]
    mov eax, FC_ERR_NOT_DIR
    call fc_windows_set_normalized_error
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

; os_same_file(path1_utf8, path2_utf8)
; Compare file identities using volume serial and file index from native handles.
; Failure to open either path means they are treated as different; cp's normal
; open path handles the actual error.
os_same_file:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 192
    mov r14, rdi
    mov r15, rsi
    mov r12, -1
    mov r13, -1
    xor ebx, ebx

    mov rdi, r14
    mov rsi, windows_path_buffer
    mov edx, 32768
    call fc_utf8_to_utf16
    cmp rax, -1
    je .same_done
    mov rcx, windows_path_buffer
    mov edx, 80000000h          ; GENERIC_READ
    mov r8d, 7                  ; share read/write/delete
    xor r9d, r9d                ; no security attributes
    mov qword [rsp + 32], 3     ; OPEN_EXISTING
    mov qword [rsp + 40], 80h  ; FILE_ATTRIBUTE_NORMAL
    mov qword [rsp + 48], 0     ; no template handle
    call [CreateFileW]
    cmp rax, -1
    je .same_done
    mov r12, rax

    mov rdi, r15
    mov rsi, windows_path_buffer2
    mov edx, 32768
    call fc_utf8_to_utf16
    cmp rax, -1
    je .same_close_first
    mov rcx, windows_path_buffer2
    mov edx, 80000000h
    mov r8d, 7
    xor r9d, r9d
    mov qword [rsp + 32], 3
    mov qword [rsp + 40], 80h
    mov qword [rsp + 48], 0
    call [CreateFileW]
    cmp rax, -1
    je .same_close_first
    mov r13, rax

    mov rcx, r12
    lea rdx, [rsp + 64]
    call [GetFileInfoByHandle]
    test eax, eax
    jz .same_close_both
    mov rcx, r13
    lea rdx, [rsp + 120]
    call [GetFileInfoByHandle]
    test eax, eax
    jz .same_close_both

    ; BY_HANDLE_FILE_INFORMATION: volume serial at +28, file index at +44/+48.
    mov eax, [rsp + 64 + 28]
    cmp eax, [rsp + 120 + 28]
    jne .same_close_both
    mov eax, [rsp + 64 + 44]
    cmp eax, [rsp + 120 + 44]
    jne .same_close_both
    mov eax, [rsp + 64 + 48]
    cmp eax, [rsp + 120 + 48]
    jne .same_close_both
    mov ebx, 1
    jmp .same_close_both

.same_close_first:
    mov rcx, r12
    call [CloseHandle]
    jmp .same_done
.same_close_both:
    mov rcx, r13
    call [CloseHandle]
    mov rcx, r12
    call [CloseHandle]
.same_done:
    mov eax, ebx
    add rsp, 192
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; os_same_handle(handle1, handle2)
; Compare file identities from already-open handles. Return 1 same, 0 different,
; or -1 if either handle cannot be queried.
os_same_handle:
    push r12
    push r13
    sub rsp, 152
    mov r12, rdi
    mov r13, rsi
    mov rcx, r12
    lea rdx, [rsp + 32]
    call [GetFileInfoByHandle]
    test eax, eax
    jz .same_handle_error
    mov rcx, r13
    lea rdx, [rsp + 88]
    call [GetFileInfoByHandle]
    test eax, eax
    jz .same_handle_error
    mov eax, [rsp + 32 + 28]
    cmp eax, [rsp + 88 + 28]
    jne .same_handle_different
    mov eax, [rsp + 32 + 44]
    cmp eax, [rsp + 88 + 44]
    jne .same_handle_different
    mov eax, [rsp + 32 + 48]
    cmp eax, [rsp + 88 + 48]
    jne .same_handle_different
    mov eax, 1
    jmp .same_handle_done
.same_handle_different:
    xor eax, eax
    jmp .same_handle_done
.same_handle_error:
    call fc_windows_set_error
    mov eax, -1
.same_handle_done:
    add rsp, 152
    pop r13
    pop r12
    ret

; os_truncate(handle): reset the file pointer and truncate the open file to zero.
os_truncate:
    sub rsp, 40
    ; CreateFileW returns a new handle positioned at offset zero.
    ; Keep that position and truncate through the same open handle.
    mov rcx, rdi
    call [SetEndOfFileAPI]
    test eax, eax
    jz .truncate_error
    xor eax, eax
    jmp .truncate_done
.truncate_error:
    call fc_windows_set_error
.truncate_done:
    add rsp, 40
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
; Windows requires RemoveDirectoryW for directory symbolic links and
; directory reparse points; DeleteFileW is used for file links and files.
os_unlink:
    push r12
    push r13
    sub rsp, 32
    mov r12, rdi
    mov rdi, r12
    mov rsi, windows_path_buffer
    mov edx, 32768
    call fc_utf8_to_utf16
    cmp rax, -1
    je .unlink_encoding

    ; Inspect the entry itself without traversing a reparse point. This also
    ; prevents the directory fallback from ever targeting a normal directory.
    mov rcx, windows_path_buffer
    mov rdx, windows_stat_find_data
    call [FindFirstFileW]
    cmp rax, -1
    je .unlink_file
    mov r13, rax
    mov eax, dword [windows_stat_find_data]
    and eax, 410h              ; REPARSE_POINT | DIRECTORY
    cmp eax, 410h
    jne .unlink_close_file

    mov rcx, r13
    call [FindClose]
    mov rcx, windows_path_buffer
    call [RemoveDirectoryW]
    test eax, eax
    jz .unlink_error
    xor eax, eax
    jmp .unlink_done

.unlink_close_file:
    mov rcx, r13
    call [FindClose]
.unlink_file:
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
    pop r13
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
    mov r8d, 1                 ; MOVEFILE_REPLACE_EXISTING, matching Linux rename
    call [MoveFileExW]
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
    push r14
    sub rsp, 32
    mov r12, rsi
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
    mov edx, [r13 + 32]
    or rax, rdx
    mov [r12], rax

    ; Creation FILETIME (100 ns since 1601) -> Unix nanoseconds.
    mov rax, [r13 + 4]
    mov rdx, 116444736000000000
    sub rax, rdx
    imul rax, 100
    mov [r12 + 16], rax

    ; Last-write FILETIME -> modified_ns.
    mov rax, [r13 + 20]
    mov rdx, 116444736000000000
    sub rax, rdx
    imul rax, 100
    mov [r12 + 8], rax

    ; Last-access FILETIME -> accessed_ns.
    mov rax, [r13 + 12]
    mov rdx, 116444736000000000
    sub rax, rdx
    imul rax, 100
    mov [r12 + 24], rax

    mov eax, [r13]
    mov r14d, eax
    ; Preserve the native hidden attribute separately from the shared
    ; leading-dot basename convention exposed through FC_ATTR_HIDDEN.
    test eax, 2                  ; FILE_ATTRIBUTE_HIDDEN
    jz .stat_not_native_hidden
    or dword [r12 + 32], FC_ATTR_NATIVE_HIDDEN
.stat_not_native_hidden:
    test eax, 1                  ; FILE_ATTRIBUTE_READONLY
    jz .stat_not_readonly
    or dword [r12 + 32], FC_ATTR_READ_ONLY
.stat_not_readonly:
    test eax, 4                  ; FILE_ATTRIBUTE_SYSTEM
    jz .stat_not_system
    or dword [r12 + 32], FC_ATTR_SYSTEM
.stat_not_system:
    test eax, 20h                ; FILE_ATTRIBUTE_ARCHIVE
    jz .stat_hidden_scan
    or dword [r12 + 32], FC_ATTR_ARCHIVE

.stat_hidden_scan:
    ; Find the last non-empty path component, accepting both Win32 separators.
    lea rdx, [windows_path_buffer]
    mov r8, rdx
.stat_hidden_loop:
    mov cx, [rdx]
    test cx, cx
    jz .stat_hidden_check
    cmp cx, '\'
    je .stat_hidden_separator
    cmp cx, '/'
    jne .stat_hidden_next
.stat_hidden_separator:
    cmp word [rdx + 2], 0
    je .stat_hidden_next
    lea r8, [rdx + 2]
.stat_hidden_next:
    add rdx, 2
    jmp .stat_hidden_loop
.stat_hidden_check:
    cmp word [r8], '.'
    jne .stat_type
    or dword [r12 + 32], FC_ATTR_HIDDEN

.stat_type:
    test eax, 400h               ; FILE_ATTRIBUTE_REPARSE_POINT
    jz .stat_not_reparse
    ; FindFirstFileW exposes dwReserved0, the reparse tag, which distinguishes
    ; symbolic links from junctions and unrelated reparse-point providers.
    mov rcx, windows_path_buffer
    mov rdx, windows_stat_find_data
    call [FindFirstFileW]
    cmp rax, -1
    je .stat_done                ; retain UNKNOWN if tag lookup is unavailable
    mov r13, rax
    cmp dword [windows_stat_find_data + 36], 0A000000Ch ; IO_REPARSE_TAG_SYMLINK
    jne .stat_reparse_close
    mov dword [r12 + 36], FC_TYPE_SYMLINK
.stat_reparse_close:
    mov rcx, r13
    call [FindClose]
    jmp .stat_done
.stat_not_reparse:
    mov eax, r14d
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
    add rsp, 32
    pop r14
    pop r13
    pop r12
    ret

; os_stat_follow(path_utf8, out_fc_file_info)
; Query the resolved target through a normal Win32 handle. Omitting
; FILE_FLAG_OPEN_REPARSE_POINT makes CreateFileW follow symbolic links and
; junctions, matching Linux stat(2) semantics.
os_stat_follow:
    push r12
    push r13
    push r14
    push r15
    sub rsp, 56
    mov r12, rsi
    mov rsi, windows_path_buffer
    mov edx, 32768
    call fc_utf8_to_utf16
    cmp rax, -1
    je .stat_follow_encoding

    ; FILE_READ_ATTRIBUTES, share read/write/delete, OPEN_EXISTING.
    ; BACKUP_SEMANTICS permits directory targets as well as files.
    mov rcx, windows_path_buffer
    mov edx, 80h
    mov r8d, 7
    xor r9d, r9d
    mov qword [rsp + 32], 3
    mov qword [rsp + 40], 02000000h
    mov qword [rsp + 48], 0
    call [CreateFileW]
    cmp rax, -1
    je .stat_follow_error
    mov r13, rax

    mov rcx, r13
    mov rdx, windows_stat_find_data
    call [GetFileInfoByHandle]
    test eax, eax
    jz .stat_follow_info_error

    ; BY_HANDLE_FILE_INFORMATION layout differs from
    ; WIN32_FILE_ATTRIBUTE_DATA: size high/low are at +32/+36.
    mov rdi, r12
    xor eax, eax
    mov ecx, 8
    rep stosq

    mov eax, [windows_stat_find_data + 32]
    shl rax, 32
    mov edx, [windows_stat_find_data + 36]
    or rax, rdx
    mov [r12], rax

    mov rax, [windows_stat_find_data + 4]
    mov rdx, 116444736000000000
    sub rax, rdx
    imul rax, 100
    mov [r12 + 16], rax

    mov rax, [windows_stat_find_data + 20]
    mov rdx, 116444736000000000
    sub rax, rdx
    imul rax, 100
    mov [r12 + 8], rax

    mov rax, [windows_stat_find_data + 12]
    mov rdx, 116444736000000000
    sub rax, rdx
    imul rax, 100
    mov [r12 + 24], rax

    mov r14d, [windows_stat_find_data]
    test r14d, 2                  ; FILE_ATTRIBUTE_HIDDEN
    jz .stat_follow_not_hidden
    or dword [r12 + 32], FC_ATTR_NATIVE_HIDDEN
.stat_follow_not_hidden:
    test r14d, 1                  ; FILE_ATTRIBUTE_READONLY
    jz .stat_follow_not_readonly
    or dword [r12 + 32], FC_ATTR_READ_ONLY
.stat_follow_not_readonly:
    test r14d, 4                  ; FILE_ATTRIBUTE_SYSTEM
    jz .stat_follow_not_system
    or dword [r12 + 32], FC_ATTR_SYSTEM
.stat_follow_not_system:
    test r14d, 20h                ; FILE_ATTRIBUTE_ARCHIVE
    jz .stat_follow_hidden_scan
    or dword [r12 + 32], FC_ATTR_ARCHIVE

.stat_follow_hidden_scan:
    lea rdx, [windows_path_buffer]
    mov r8, rdx
.stat_follow_hidden_loop:
    mov cx, [rdx]
    test cx, cx
    jz .stat_follow_hidden_check
    cmp cx, '\'
    je .stat_follow_hidden_separator
    cmp cx, '/'
    jne .stat_follow_hidden_next
.stat_follow_hidden_separator:
    cmp word [rdx + 2], 0
    je .stat_follow_hidden_next
    lea r8, [rdx + 2]
.stat_follow_hidden_next:
    add rdx, 2
    jmp .stat_follow_hidden_loop
.stat_follow_hidden_check:
    cmp word [r8], '.'
    jne .stat_follow_type
    or dword [r12 + 32], FC_ATTR_HIDDEN
.stat_follow_type:
    test r14d, 10h                ; FILE_ATTRIBUTE_DIRECTORY
    jz .stat_follow_regular
    mov dword [r12 + 36], FC_TYPE_DIRECTORY
    jmp .stat_follow_close
.stat_follow_regular:
    mov dword [r12 + 36], FC_TYPE_REGULAR

.stat_follow_close:
    mov rcx, r13
    call [CloseHandle]
    test eax, eax
    jz .stat_follow_error
    xor eax, eax
    jmp .stat_follow_return

.stat_follow_info_error:
    call fc_windows_set_error
    mov r15, rax
    mov rcx, r13
    call [CloseHandle]
    mov rax, r15
    jmp .stat_follow_return
.stat_follow_encoding:
    mov eax, FC_ERR_ENCODING
    call fc_windows_set_normalized_error
    jmp .stat_follow_return
.stat_follow_error:
    call fc_windows_set_error
.stat_follow_return:
    add rsp, 56
    pop r15
    pop r14
    pop r13
    pop r12
    ret

; os_set_times(path_utf8, accessed_ns, modified_ns)
; Set access and modification times through a temporary FILE_WRITE_ATTRIBUTES
; handle. Windows FILETIME precision is 100 ns, so sub-100 ns input is rounded
; down to the preceding FILETIME tick, including for pre-epoch timestamps.
os_set_times:
    push r12
    push r13
    push r14
    push r15
    sub rsp, 104
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx

    mov rdi, r12
    mov rsi, windows_path_buffer
    mov edx, 32768
    call fc_utf8_to_utf16
    cmp rax, -1
    je .set_times_encoding

    ; Convert signed Unix nanoseconds to FILETIME ticks. IDIV truncates toward
    ; zero, so adjust a negative remainder to obtain mathematical floor(ns/100).
    mov rax, r13
    cqo
    mov rcx, 100
    idiv rcx
    test rdx, rdx
    jns .set_times_atime_floor
    dec rax
.set_times_atime_floor:
    mov rdx, 116444736000000000
    add rax, rdx
    test rax, rax
    js .set_times_range
    mov [rsp + 64], rax

    mov rax, r14
    cqo
    mov rcx, 100
    idiv rcx
    test rdx, rdx
    jns .set_times_mtime_floor
    dec rax
.set_times_mtime_floor:
    mov rdx, 116444736000000000
    add rax, rdx
    test rax, rax
    js .set_times_range
    mov [rsp + 72], rax

    ; FILE_WRITE_ATTRIBUTES, share read/write/delete, OPEN_EXISTING.
    mov rcx, windows_path_buffer
    mov edx, 100h
    mov r8d, 7
    xor r9d, r9d
    mov qword [rsp + 32], 3
    mov qword [rsp + 40], 02000000h ; FILE_FLAG_BACKUP_SEMANTICS
    mov qword [rsp + 48], 0
    call [CreateFileW]
    cmp rax, -1
    je .set_times_error
    mov r15, rax

    ; Creation time is unchanged; set last-access and last-write times.
    mov rcx, r15
    xor edx, edx
    lea r8, [rsp + 64]
    lea r9, [rsp + 72]
    call [SetFileTime]
    test eax, eax
    jz .set_times_api_error

    mov rcx, r15
    call [CloseHandle]
    test eax, eax
    jz .set_times_close_error
    xor eax, eax
    jmp .set_times_done

.set_times_api_error:
    call fc_windows_set_error
    mov r14, rax
    mov rcx, r15
    call [CloseHandle]
    mov rax, r14
    jmp .set_times_done
.set_times_close_error:
    call fc_windows_set_error
    jmp .set_times_done
.set_times_range:
    mov eax, FC_ERR_INVALID
    call fc_windows_set_normalized_error
    jmp .set_times_done
.set_times_encoding:
    mov eax, FC_ERR_ENCODING
    call fc_windows_set_normalized_error
    jmp .set_times_done
.set_times_error:
    call fc_windows_set_error
.set_times_done:
    add rsp, 104
    pop r15
    pop r14
    pop r13
    pop r12
    ret

; Windows directory iterator pool. Each token identifies one independent
; FindFirstFileW/FindNextFileW search and its current WIN32_FIND_DATAW record.
WINDOWS_DIR_SLOT_COUNT = 8
WINDOWS_DIR_DATA_SIZE = 592
WINDOWS_DIR_SLOT_SIZE = 16 + WINDOWS_DIR_DATA_SIZE

; os_dir_open(path_utf8, out_dir_handle)
; Appends a wildcard to the directory path and opens an independent iterator.
os_dir_open:
    push r12
    push r13
    push r14
    push r15
    sub rsp, 40
    mov r13, rsi
    test r13, r13
    jz .dir_open_invalid

    ; Reserve a free iterator slot before doing any native work.
    lea r12, [windows_dir_slots]
    xor r14d, r14d
.dir_open_find_slot:
    cmp r14d, WINDOWS_DIR_SLOT_COUNT
    jae .dir_open_no_slots
    cmp dword [r12], 0
    je .dir_open_slot_found
    add r12, WINDOWS_DIR_SLOT_SIZE
    inc r14d
    jmp .dir_open_find_slot

.dir_open_slot_found:
    mov rsi, windows_path_buffer
    mov edx, 32768
    call fc_utf8_to_utf16
    cmp rax, -1
    je .dir_open_encoding
    test rax, rax
    jz .dir_open_invalid
    mov r15, rax

    ; Ensure room for an optional separator, '*', and UTF-16 NUL.
    cmp r15, 32765
    ja .dir_open_too_long
    lea r11, [windows_path_buffer]
    lea rdx, [r15 + r15]
    mov ax, [r11 + rdx - 2]
    cmp ax, '\'
    je .dir_open_append_star
    cmp ax, '/'
    je .dir_open_append_star
    mov word [r11 + rdx], '\'
    add rdx, 2
    inc r15
.dir_open_append_star:
    mov word [r11 + rdx], '*'
    add rdx, 2
    inc r15
    mov word [r11 + rdx], 0

    mov rcx, windows_path_buffer
    lea rdx, [r12 + 16]
    call [FindFirstFileW]
    cmp rax, -1
    je .dir_open_error

    mov [r12 + 8], rax
    mov dword [r12 + 4], 1
    mov dword [r12], 1
    lea rax, [r14 + 1]
    mov [r13], rax
    xor eax, eax
    jmp .dir_open_done

.dir_open_no_slots:
    mov eax, FC_ERR_NO_MEMORY
    call fc_windows_set_normalized_error
    jmp .dir_open_done
.dir_open_invalid:
    mov eax, FC_ERR_INVALID
    call fc_windows_set_normalized_error
    jmp .dir_open_done
.dir_open_encoding:
    mov eax, FC_ERR_ENCODING
    call fc_windows_set_normalized_error
    jmp .dir_open_done
.dir_open_too_long:
    mov eax, FC_ERR_TOO_SMALL
    call fc_windows_set_normalized_error
    jmp .dir_open_done
.dir_open_error:
    call fc_windows_set_error
.dir_open_done:
    add rsp, 40
    pop r15
    pop r14
    pop r13
    pop r12
    ret

; os_dir_next(handle, out_info, name_buffer, name_capacity, out_name_len)
; Returns 1 for an entry, 0 for EOF, or -1 for an error. A too-small output
; buffer leaves the current native record pending so the caller can retry.
os_dir_next:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 32
    mov rbx, r8
    mov r13, rsi
    mov r14, rdx
    mov r15, rcx

    test rbx, rbx
    jz .dir_next_invalid
    mov qword [rbx], 0
    test r13, r13
    jz .dir_next_invalid
    test r14, r14
    jz .dir_next_invalid
    cmp rdi, 1
    jb .dir_next_invalid
    cmp rdi, WINDOWS_DIR_SLOT_COUNT
    ja .dir_next_invalid
    dec rdi
    imul r12, rdi, WINDOWS_DIR_SLOT_SIZE
    lea r12, [windows_dir_slots + r12]
    cmp dword [r12], 1
    jne .dir_next_invalid

    cmp dword [r12 + 4], 1
    je .dir_next_record_ready

    ; The previous record was consumed; advance this iterator only.
    mov rcx, [r12 + 8]
    lea rdx, [r12 + 16]
    call [FindNextFileW]
    test eax, eax
    jnz .dir_next_mark_pending
    call [GetLastError]
    cmp eax, 18                  ; ERROR_NO_MORE_FILES
    je .dir_next_eof
    mov [os_native_error], rax
    mov dword [os_error_domain], FC_ERROR_DOMAIN_WINDOWS
    mov dword [os_error_code], FC_ERR_IO
    mov rax, -1
    jmp .dir_next_done
.dir_next_mark_pending:
    mov dword [r12 + 4], 1

.dir_next_record_ready:
    ; Convert the native UTF-16 filename into a bounded temporary UTF-8 name.
    lea rdi, [r12 + 16 + 44]
    mov rsi, windows_dir_name_buffer
    mov edx, 1024
    call fc_utf16_to_utf8
    cmp rax, -1
    je .dir_next_encoding
    mov r10, rax
    cmp r10, r15
    ja .dir_next_too_small

    ; Copy bytes first; terminate only if the caller supplied the extra byte.
    mov rdi, r14
    mov rsi, windows_dir_name_buffer
    mov rcx, r10
    rep movsb
    cmp r10, r15
    jae .dir_next_no_terminator
    mov byte [r14 + r10], 0
.dir_next_no_terminator:
    mov [rbx], r10

    ; Populate metadata available in WIN32_FIND_DATAW.
    mov rdi, r13
    xor eax, eax
    mov ecx, 8
    rep stosq
    lea r11, [r12 + 16]
    mov eax, [r11 + 28]
    shl rax, 32
    mov edx, [r11 + 32]
    or rax, rdx
    mov [r13], rax

    mov rax, [r11 + 20]
    mov rdx, 116444736000000000
    sub rax, rdx
    imul rax, 100
    mov [r13 + 8], rax
    mov rax, [r11 + 4]
    mov rdx, 116444736000000000
    sub rax, rdx
    imul rax, 100
    mov [r13 + 16], rax
    mov rax, [r11 + 12]
    mov rdx, 116444736000000000
    sub rax, rdx
    imul rax, 100
    mov [r13 + 24], rax

    mov eax, [r11]
    ; Dot-prefixed names use the shared hidden convention.
    cmp byte [windows_dir_name_buffer], '.'
    jne .dir_next_not_dot_hidden
    or dword [r13 + 32], FC_ATTR_HIDDEN
.dir_next_not_dot_hidden:
    ; Preserve the Win32 hidden attribute independently.
    test eax, 2
    jz .dir_next_not_native_hidden
    or dword [r13 + 32], FC_ATTR_NATIVE_HIDDEN
.dir_next_not_native_hidden:
    test eax, 1
    jz .dir_next_not_readonly
    or dword [r13 + 32], FC_ATTR_READ_ONLY
.dir_next_not_readonly:
    test eax, 4
    jz .dir_next_not_system
    or dword [r13 + 32], FC_ATTR_SYSTEM
.dir_next_not_system:
    test eax, 20h
    jz .dir_next_type
    or dword [r13 + 32], FC_ATTR_ARCHIVE
.dir_next_type:
    test eax, 400h
    jz .dir_next_not_reparse
    cmp dword [r11 + 36], 0A000000Ch ; IO_REPARSE_TAG_SYMLINK
    jne .dir_next_consume
    mov dword [r13 + 36], FC_TYPE_SYMLINK
    jmp .dir_next_consume
.dir_next_not_reparse:
    test eax, 10h
    jz .dir_next_regular
    mov dword [r13 + 36], FC_TYPE_DIRECTORY
    jmp .dir_next_consume
.dir_next_regular:
    mov dword [r13 + 36], FC_TYPE_REGULAR

.dir_next_consume:
    mov dword [r12 + 4], 0
    mov eax, 1
    jmp .dir_next_done
.dir_next_eof:
    xor eax, eax
    jmp .dir_next_done
.dir_next_too_small:
    mov eax, FC_ERR_TOO_SMALL
    call fc_windows_set_normalized_error
    jmp .dir_next_done
.dir_next_encoding:
    mov eax, FC_ERR_ENCODING
    call fc_windows_set_normalized_error
    jmp .dir_next_done
.dir_next_invalid:
    mov eax, FC_ERR_INVALID
    call fc_windows_set_normalized_error
    jmp .dir_next_done
.dir_next_done:
    add rsp, 32
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; os_dir_close(handle)
os_dir_close:
    push r12
    sub rsp, 32
    cmp rdi, 1
    jb .dir_close_invalid
    cmp rdi, WINDOWS_DIR_SLOT_COUNT
    ja .dir_close_invalid
    dec rdi
    imul r12, rdi, WINDOWS_DIR_SLOT_SIZE
    lea r12, [windows_dir_slots + r12]
    cmp dword [r12], 1
    jne .dir_close_invalid
    mov rcx, [r12 + 8]
    call [FindClose]
    test eax, eax
    jz .dir_close_error
    mov dword [r12], 0
    mov dword [r12 + 4], 0
    mov qword [r12 + 8], 0
    xor eax, eax
    jmp .dir_close_done
.dir_close_invalid:
    mov eax, FC_ERR_INVALID
    call fc_windows_set_normalized_error
    jmp .dir_close_done
.dir_close_error:
    call fc_windows_set_error
.dir_close_done:
    add rsp, 32
    pop r12
    ret

; os_get_time(out_unix_ns)
; Convert the current system FILETIME (100 ns ticks since 1601) to Unix ns.
os_get_time:
    sub rsp, 40
    lea rcx, [rsp + 32]
    call [GetSystemTimeAsFileTime]
    mov rax, [rsp + 32]
    mov rdx, 116444736000000000
    sub rax, rdx
    imul rax, 100
    mov [rdi], rax
    xor eax, eax
    add rsp, 40
    ret

; Windows can identify symbolic links by their reparse tag and supports
; FILETIME updates. POSIX modes, hard-link operations, and other optional
; interfaces are not exposed by this backend contract.
os_get_capabilities:
    mov eax, FC_CAP_SYMLINKS or FC_CAP_FILE_TIMES
    ret
