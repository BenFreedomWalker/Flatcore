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
    lea rdx, [r15 + r15]
    mov ax, [windows_path_buffer + rdx - 2]
    cmp ax, '\'
    je .dir_open_append_star
    cmp ax, '/'
    je .dir_open_append_star
    mov word [windows_path_buffer + rdx], '\'
    add rdx, 2
    inc r15
.dir_open_append_star:
    mov word [windows_path_buffer + rdx], '*'
    add rdx, 2
    inc r15
    mov word [windows_path_buffer + rdx], 0

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
    test eax, 2
    jz .dir_next_not_hidden
    or dword [r13 + 32], FC_ATTR_HIDDEN
.dir_next_not_hidden:
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
    jnz .dir_next_consume       ; preserve UNKNOWN for unclassified reparse tags
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
    add rsp, 40
    pop r12
    ret

; Capability reporting remains conservative until the optional operations land.
os_get_capabilities:
    xor eax, eax
    ret
