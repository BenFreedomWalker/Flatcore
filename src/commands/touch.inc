; Shared FC64 touch command.
; touch [-a] [-m] [-c] [--no-create] [--] FILE...
; -a updates access time, -m updates modification time. With neither (or
; both), both timestamps are updated. New files are created unless -c is used.

fc_cmd_touch_write:
    mov rcx, os_write
    jmp fc_write_all

fc_cmd_touch_write_cstr:
    push rbx
    push r12
    sub rsp, 8
    mov r12, rdi
    mov rbx, rsi
    mov rdi, rbx
    call fc_strlen
    mov rdx, rax
    mov rdi, r12
    mov rsi, rbx
    call fc_cmd_touch_write
    add rsp, 8
    pop r12
    pop rbx
    ret

; Print one path-specific failure. RDI=path.
fc_cmd_touch_error:
    push rbx
    push r12
    sub rsp, 8
    mov r12, rdi
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_touch_error_prefix
    mov edx, fc_cmd_touch_error_prefix_length
    call fc_cmd_touch_write
    mov rdi, r12
    call fc_strlen
    mov rbx, rax
    mov rdi, [os_stderr]
    mov rsi, r12
    mov rdx, rbx
    call fc_cmd_touch_write
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_touch_error_suffix
    mov edx, fc_cmd_touch_error_suffix_length
    call fc_cmd_touch_write
    add rsp, 8
    pop r12
    pop rbx
    ret

fc_cmd_touch:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov r13, rsi
    mov r14, 1
    xor r15d, r15d             ; bit 0: atime, bit 1: mtime, bit 2: no-create

.parse:
    cmp r14, r12
    jae .parsed
    mov rsi, [r13 + r14 * 8]
    cmp byte [rsi], '-'
    jne .parsed
    cmp byte [rsi + 1], 0
    je .parsed
    cmp byte [rsi + 1], '-'
    jne .short_option
    cmp byte [rsi + 2], 0
    je .end_options
    mov rdi, fc_cmd_touch_opt_no_create
    call fc_streq
    test eax, eax
    jnz .set_no_create
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_touch_opt_help
    call fc_streq
    test eax, eax
    jnz .help
    mov rsi, [r13 + r14 * 8]
    mov rdi, fc_cmd_touch_opt_version
    call fc_streq
    test eax, eax
    jnz .version
    jmp .bad_option

.short_option:
    ; -r FILE copies both reference timestamps (or the selected -a/-m one).
    mov rdi, rsi
    mov rsi, fc_cmd_touch_opt_reference
    call fc_streq
    test eax, eax
    jnz .reference_option
    mov rsi, [r13 + r14 * 8]
    ; Accept clustered -a/-m/-c options.
    mov rbx, rsi
    inc rbx
.short_loop:
    mov al, [rbx]
    test al, al
    jz .short_done
    cmp al, 'a'
    je .short_atime
    cmp al, 'm'
    je .short_mtime
    cmp al, 'c'
    je .short_no_create
    jmp .bad_option
.short_atime:
    or r15d, 1
    jmp .short_next
.short_mtime:
    or r15d, 2
    jmp .short_next
.short_no_create:
    or r15d, 4
.short_next:
    inc rbx
    jmp .short_loop
.short_done:
    inc r14
    jmp .parse

.reference_option:
    lea rax, [r14 + 1]
    cmp rax, r12
    jae .usage
    mov rax, [r13 + rax * 8]
    mov [fc_cmd_touch_reference], rax
    or r15d, 16
    add r14, 2
    jmp .parse

.set_no_create:
    or r15d, 4
    inc r14
    jmp .parse
.end_options:
    inc r14
    jmp .parsed

.parsed:
    cmp r14, r12
    jae .usage
    ; No explicit timestamp selector means update both.
    test r15d, 3
    jnz .timestamps_selected
    or r15d, 3
.timestamps_selected:
    ; Resolve the reference once before modifying any destination.
    test r15d, 16
    jz .files
    mov rdi, qword [fc_cmd_touch_reference]
    mov rsi, fc_cmd_touch_ref_info
    call os_stat_follow
    test eax, eax
    jnz .reference_error

.files:
    cmp r14, r12
    jae .success
    mov rbx, [r13 + r14 * 8]

    ; Resolve symbolic links before reading timestamps. os_set_times follows
    ; links too, so both operations must refer to the same target object.
    mov rdi, rbx
    mov rsi, fc_cmd_touch_info
    call os_stat_follow
    test eax, eax
    jz .have_info
    cmp dword [os_error_code], FC_ERR_NOT_FOUND
    jne .file_error
    test r15d, 4
    jnz .next_file

    ; Create a missing file without truncating anything (CREATE + WRITE).
    ; For a dangling symlink this creates its missing target, as touch normally does.
    mov rdi, rbx
    mov esi, FC_OPEN_WRITE or FC_OPEN_CREATE
    mov edx, 0644o
    mov rcx, fc_cmd_touch_handle
    call os_open
    test eax, eax
    jnz .file_error
    mov rdi, qword [fc_cmd_touch_handle]
    call os_close
    test eax, eax
    jnz .file_error

    ; Fetch the timestamps established by file creation so -a/-m can
    ; preserve the unselected timestamp.
    mov rdi, rbx
    mov rsi, fc_cmd_touch_info
    call os_stat_follow
    test eax, eax
    jnz .file_error

.have_info:
    test r15d, 16
    jz .current_time
    ; Reference mode: preserve unselected target timestamps and copy the
    ; corresponding reference timestamp for each selected field.
    mov rsi, qword [fc_cmd_touch_info + 24]
    mov rdx, qword [fc_cmd_touch_info + 8]
    test r15d, 1
    jz .reference_keep_atime
    mov rsi, qword [fc_cmd_touch_ref_info + 24]
.reference_keep_atime:
    test r15d, 2
    jz .reference_keep_mtime
    mov rdx, qword [fc_cmd_touch_ref_info + 8]
.reference_keep_mtime:
    jmp .timestamps_ready

.current_time:
    mov rdi, fc_cmd_touch_now
    call os_get_time
    test eax, eax
    jnz .file_error
    mov rsi, qword [fc_cmd_touch_info + 24] ; preserve atime by default
    mov rdx, qword [fc_cmd_touch_info + 8]  ; preserve mtime by default
    test r15d, 1
    jz .current_keep_atime
    mov rsi, qword [fc_cmd_touch_now]
.current_keep_atime:
    test r15d, 2
    jz .current_keep_mtime
    mov rdx, qword [fc_cmd_touch_now]
.current_keep_mtime:
.timestamps_ready:
    mov rdi, rbx
    call os_set_times
    test eax, eax
    jnz .file_error

.next_file:
    inc r14
    jmp .files

.reference_error:
    mov rdi, qword [fc_cmd_touch_reference]
    call fc_cmd_touch_error
    mov eax, 1
    jmp .return

.file_error:
    mov rdi, rbx
    call fc_cmd_touch_error
    or r15d, 8
    inc r14
    jmp .files

.help:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_touch_help_text
    mov edx, fc_cmd_touch_help_length
    call fc_cmd_touch_write
    xor eax, eax
    jmp .return

.version:
    mov rdi, [os_stdout]
    mov rsi, fc_cmd_touch_version_text
    mov edx, fc_cmd_touch_version_length
    call fc_cmd_touch_write
    xor eax, eax
    jmp .return

.bad_option:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_touch_bad_option_text
    mov edx, fc_cmd_touch_bad_option_length
    call fc_cmd_touch_write
    mov eax, 2
    jmp .return

.usage:
    mov rdi, [os_stderr]
    mov rsi, fc_cmd_touch_usage_text
    mov edx, fc_cmd_touch_usage_length
    call fc_cmd_touch_write
    mov eax, 2
.return:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

.success:
    test r15d, 8
    jnz .failed_status
    xor eax, eax
    jmp .return
.failed_status:
    mov eax, 1
    jmp .return
