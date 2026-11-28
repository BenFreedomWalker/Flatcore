include 'src/include/flatcore.inc'

include 'win64a.inc'

format PE64 console
entry start

section '.text' code readable executable

; Flatcore's first native Windows x86-64 entry point.
; This bootstrap deliberately uses Win32 APIs rather than Linux syscalls.

start:
    sub rsp, 72
    call os_init

    ; Capture the native UTF-16 command line, convert it to the shared UTF-8
    ; representation, tokenize it, and normalize explicit/multicall argv.
    call [GetCommandLineW]
    mov r12, rax
    mov rdi, r12
    mov rsi, windows_utf8_command_line
    mov edx, 131072
    call fc_utf16_to_utf8
    cmp rax, -1
    je .startup_error

    mov rdi, windows_utf8_command_line
    mov rsi, windows_argv
    mov edx, 16384
    mov rcx, windows_arg_storage
    mov r8d, 131072
    call fc_parse_argv
    cmp rax, -1
    je .startup_error
    mov [windows_argc], rax

    mov rdi, rax
    mov rsi, windows_argv
    mov rdx, windows_executable_name
    mov rcx, windows_normalized_argc
    mov r8, windows_normalized_argv
    call fc_prepare_command
    cmp rax, -1
    je .startup_error

    ; Route migrated commands through the same shared registry as Linux.
    mov rdi, [windows_normalized_argc]
    mov rsi, [windows_normalized_argv]
    mov rdx, fc_basic_registry
    mov rcx, fc_basic_registry_count
    call fc_dispatch
    cmp eax, 127
    jne .shared_command_exit

    ; The legacy command bodies remain active during migration. They continue
    ; consuming the original command-line tail until the shared commands land.
    call [GetCommandLineA]
    test rax, rax
    jz .startup_error

    ; GetCommandLineA returns a system-managed buffer. Copy it into bounded
    ; writable storage before the legacy parser edits it in place.
    mov rsi, rax
    mov rdi, windows_legacy_command_line
    mov ecx, 131072
.legacy_command_line_copy:
    mov al, [rsi]
    mov [rdi], al
    inc rsi
    inc rdi
    test al, al
    jz .legacy_command_line_copied
    dec ecx
    jnz .legacy_command_line_copy
    jmp .startup_error
.legacy_command_line_copied:
    mov r12, windows_legacy_command_line

    ; Skip argv[0], honoring a quoted executable path.
    mov rsi, r12
    cmp byte [rsi], '"'
    jne .skip_unquoted_exe
    inc rsi
.skip_quoted_exe:
    cmp byte [rsi], 0
    je .no_command
    cmp byte [rsi], '"'
    je .quoted_exe_end
    inc rsi
    jmp .skip_quoted_exe
.quoted_exe_end:
    inc rsi
    jmp .after_exe
.skip_unquoted_exe:
    cmp byte [rsi], 0
    je .no_command
    cmp byte [rsi], ' '
    je .after_exe
    cmp byte [rsi], 9
    je .after_exe
    inc rsi
    jmp .skip_unquoted_exe
.after_exe:
    ; Skip whitespace before command.
.skip_space:
    cmp byte [rsi], ' '
    je .space_next
    cmp byte [rsi], 9
    jne .command_start
.space_next:
    inc rsi
    jmp .skip_space
.command_start:
    mov r13, rsi
    xor ecx, ecx
.command_scan:
    mov al, [rsi + rcx]
    test al, al
    jz .command_end
    cmp al, ' '
    je .command_end
    cmp al, 9
    je .command_end
    inc rcx
    jmp .command_scan
.command_end:
    mov r14, rcx
    cmp byte [r13 + r14], 0
    je .args_at_end
    mov byte [r13 + r14], 0
    lea rsi, [r13 + r14 + 1]
    jmp .args_space
.args_at_end:
    lea rsi, [r13 + r14]
    ; Skip whitespace before command arguments.
.args_space:
    cmp byte [rsi], ' '
    je .args_next
    cmp byte [rsi], 9
    jne .dispatch
.args_next:
    inc rsi
    jmp .args_space

.dispatch:
    mov rdi, r13
    mov r10, help_name
    call string_equal
    test eax, eax
    jnz .help

    mov rdi, r13
    mov r10, true_name
    call string_equal
    test eax, eax
    jnz .success

    mov rdi, r13
    mov r10, false_name
    call string_equal
    test eax, eax
    jnz .failure

    mov rdi, r13
    mov r10, echo_name
    call string_equal
    test eax, eax
    jnz .echo

    mov rdi, r13
    mov r10, pwd_name
    call string_equal
    test eax, eax
    jnz .pwd

    mov rdi, r13
    mov r10, hostname_name
    call string_equal
    test eax, eax
    jnz .hostname

    mov rdi, r13
    mov r10, mkdir_name
    call string_equal
    test eax, eax
    jnz .mkdir

    mov rdi, r13
    mov r10, rmdir_name
    call string_equal
    test eax, eax
    jnz .rmdir

    mov rdi, r13
    mov r10, rm_name
    call string_equal
    test eax, eax
    jnz .rm

    mov rdi, r13
    mov r10, touch_name
    call string_equal
    test eax, eax
    jnz .touch

    mov rdi, r13
    mov r10, cat_name
    call string_equal
    test eax, eax
    jnz .cat

    mov rdi, r13
    mov r10, ls_name
    call string_equal
    test eax, eax
    jnz .ls

    mov rdi, r13
    mov r10, cp_name
    call string_equal
    test eax, eax
    jnz .cp

    mov rdi, r13
    mov r10, mv_name
    call string_equal
    test eax, eax
    jnz .mv

    mov rdi, r13
    mov r10, basename_name
    call string_equal
    test eax, eax
    jnz .basename

    mov rdi, r13
    mov r10, dirname_name
    call string_equal
    test eax, eax
    jnz .dirname

    mov rdi, r13
    mov r10, head_name
    call string_equal
    test eax, eax
    jnz .head

    mov rsi, unknown_text
    call write_stdout
    mov ecx, 127
    jmp .exit

.help:
    mov rsi, help_text
    call write_stdout
    jmp .success

.pwd:
    mov ecx, 4096
    mov rdx, cwd_buffer
    call [GetCurrentDirectoryA]
    test eax, eax
    jz .api_error
    mov edx, eax
    mov rsi, cwd_buffer
    call write_stdout_n
    mov rsi, newline_text
    mov edx, 2
    call write_stdout_n
    jmp .success

.hostname:
    mov dword [computer_name_length], 256
    mov rcx, computer_name_buffer
    mov rdx, computer_name_length
    call [GetComputerNameA]
    test eax, eax
    jz .api_error
    mov edx, [computer_name_length]
    mov rsi, computer_name_buffer
    call write_stdout_n
    mov rsi, newline_text
    mov edx, 2
    call write_stdout_n
    jmp .success

.ls:
    call first_argument
    test rax, rax
    jnz .ls_copy_path
    mov rax, dot_path
.ls_copy_path:
    mov rsi, rax
    mov rdi, ls_pattern
.ls_copy_loop:
    mov al, [rsi]
    test al, al
    jz .ls_pattern_end
    mov [rdi], al
    inc rsi
    inc rdi
    jmp .ls_copy_loop
.ls_pattern_end:
    mov byte [rdi], '\'
    inc rdi
    mov byte [rdi], '*'
    inc rdi
    mov byte [rdi], 0
    mov rcx, ls_pattern
    mov rdx, find_data
    call [FindFirstFileA]
    cmp rax, -1
    je .api_error
    mov r15, rax
.ls_entry:
    lea rdi, [find_data + 44]
    call string_length
    test rax, rax
    jz .ls_next
    mov rsi, find_data + 44
    mov rdx, rax
    call write_stdout_n
    mov rsi, newline_text
    mov edx, 2
    call write_stdout_n
.ls_next:
    mov rcx, r15
    mov rdx, find_data
    call [FindNextFileA]
    test eax, eax
    jnz .ls_entry
    mov rcx, r15
    call [FindClose]
    jmp .success

.cp:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rbx, rax
    call first_argument
    test rax, rax
    jz .usage_error
    mov rcx, rbx
    mov rdx, rax
    xor r8d, r8d
    call [CopyFileA]
    test eax, eax
    jz .api_error
    jmp .success

.mv:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rbx, rax
    call first_argument
    test rax, rax
    jz .usage_error
    mov rcx, rbx
    mov rdx, rax
    call [MoveFileA]
    test eax, eax
    jz .api_error
    jmp .success

.basename:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rdi, rax
    call string_length
    mov r8, rax
    test r8, r8
    jz .basename_output
.basename_trim:
    cmp r8, 1
    jbe .basename_check_root
    mov al, [rdi + r8 - 1]
    cmp al, '\'
    je .basename_trim_one
    cmp al, '/'
    jne .basename_check_root
.basename_trim_one:
    dec r8
    jmp .basename_trim
.basename_check_root:
    cmp r8, 1
    jne .basename_terminate
    mov al, [rdi]
    cmp al, '\'
    je .basename_root
    cmp al, '/'
    je .basename_root
.basename_terminate:
    mov byte [rdi + r8], 0
    mov r9, r8
.basename_scan:
    test r9, r9
    jz .basename_output
    dec r9
    mov al, [rdi + r9]
    cmp al, '\'
    je .basename_found
    cmp al, '/'
    jne .basename_scan
.basename_root:
    mov rsi, rdi
    jmp .basename_print
.basename_found:
    lea rsi, [rdi + r9 + 1]
    jmp .basename_print
.basename_output:
    mov rsi, rdi
.basename_print:
    mov rdi, rsi
    call string_length
    mov rdx, rax
    call write_stdout_n
    mov rsi, newline_text
    mov edx, 2
    call write_stdout_n
    jmp .success

.dirname:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rdi, rax
    call string_length
    mov r8, rax
    mov r9, r8
.dirname_scan:
    test r9, r9
    jz .dirname_none
    dec r9
    mov al, [rdi + r9]
    cmp al, '\'
    je .dirname_found
    cmp al, '/'
    jne .dirname_scan
.dirname_found:
    test r9, r9
    jnz .dirname_drive_root
    mov byte [rdi + 1], 0
    jmp .dirname_print
.dirname_drive_root:
    cmp r9, 2
    jne .dirname_cut
    cmp byte [rdi + 1], ':'
    jne .dirname_cut
    mov byte [rdi + 3], 0
    jmp .dirname_print
.dirname_cut:
    mov byte [rdi + r9], 0
    jmp .dirname_print
.dirname_none:
    mov rsi, dot_path
    jmp .dirname_output
.dirname_print:
    mov rsi, rdi
.dirname_output:
    mov rdi, rsi
    call string_length
    mov rdx, rax
    call write_stdout_n
    mov rsi, newline_text
    mov edx, 2
    call write_stdout_n
    jmp .success

.head:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rcx, rax
    mov edx, 80000000h
    mov r8d, 1
    xor r9d, r9d
    mov qword [rsp + 32], 3
    mov qword [rsp + 40], 80h
    mov qword [rsp + 48], 0
    call [CreateFileA]
    cmp rax, -1
    je .api_error
    mov rbx, rax
    xor r14d, r14d
.head_read:
    mov rcx, rbx
    mov rdx, io_buffer
    mov r8d, 4096
    lea r9, [rsp + 56]
    mov qword [rsp + 32], 0
    mov dword [rsp + 56], 0
    call [ReadFile]
    test eax, eax
    jz .head_close_error
    mov r10d, [rsp + 56]
    test r10d, r10d
    jz .head_close
    xor r11d, r11d
.head_scan:
    cmp r11d, r10d
    jae .head_write_all
    cmp byte [io_buffer + r11], 10
    jne .head_scan_next
    inc r14d
    cmp r14d, 10
    je .head_write_ten
.head_scan_next:
    inc r11d
    jmp .head_scan
.head_write_ten:
    lea rsi, [io_buffer]
    lea rdx, [r11 + 1]
    call write_stdout_n
    jmp .head_close
.head_write_all:
    mov rsi, io_buffer
    mov edx, r10d
    call write_stdout_n
    jmp .head_read
.head_close_error:
    mov rcx, rbx
    call [CloseHandle]
    jmp .api_error
.head_close:
    mov rcx, rbx
    call [CloseHandle]
    jmp .success

.mkdir:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rcx, rax
    xor edx, edx
    call [CreateDirectoryA]
    test eax, eax
    jz .api_error
    jmp .success

.rmdir:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rcx, rax
    call [RemoveDirectoryA]
    test eax, eax
    jz .api_error
    jmp .success

.rm:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rcx, rax
    call [DeleteFileA]
    test eax, eax
    jz .api_error
    jmp .success

.touch:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rcx, rax
    mov edx, 40000000h           ; GENERIC_WRITE
    xor r8d, r8d                 ; no sharing
    xor r9d, r9d                 ; default security
    mov qword [rsp + 32], 4      ; OPEN_ALWAYS
    mov qword [rsp + 40], 80h    ; FILE_ATTRIBUTE_NORMAL
    mov qword [rsp + 48], 0
    call [CreateFileA]
    cmp rax, -1
    je .api_error
    mov r15, rax
    lea rcx, [current_file_time]
    call [GetSystemTimeAsFileTime]
    mov rcx, r15
    xor edx, edx
    xor r8d, r8d
    lea r9, [current_file_time]
    call [SetFileTime]
    test eax, eax
    jz .touch_close_error
    mov rcx, r15
    call [CloseHandle]
    jmp .success
.touch_close_error:
    mov rcx, r15
    call [CloseHandle]
    jmp .api_error

.cat:
    call first_argument
    test rax, rax
    jz .usage_error
    mov rcx, rax
    mov edx, 80000000h           ; GENERIC_READ
    mov r8d, 1                   ; FILE_SHARE_READ
    xor r9d, r9d
    mov qword [rsp + 32], 3      ; OPEN_EXISTING
    mov qword [rsp + 40], 80h
    mov qword [rsp + 48], 0
    call [CreateFileA]
    cmp rax, -1
    je .api_error
    mov r15, rax
.cat_read:
    mov rcx, r15
    mov rdx, io_buffer
    mov r8d, 4096
    lea r9, [rsp + 56]
    mov qword [rsp + 32], 0
    mov dword [rsp + 56], 0
    call [ReadFile]
    test eax, eax
    jz .cat_close_error
    mov edx, [rsp + 56]
    test edx, edx
    jz .cat_close
    mov rsi, io_buffer
    call write_stdout_n
    jmp .cat_read
.cat_close_error:
    mov rcx, r15
    call [CloseHandle]
    jmp .api_error
.cat_close:
    mov rcx, r15
    call [CloseHandle]
    jmp .success

.usage_error:
    mov rsi, usage_error_text
    call write_stdout
    mov ecx, 2
    jmp .exit

.api_error:
    mov rsi, error_text
    call write_stdout
    mov ecx, 1
    jmp .exit

.echo:
    ; Write the remaining raw command-line text, followed by a newline.
    mov rdi, rsi
    call string_length
    mov r8, rax
.echo_trim_trailing:
    test r8, r8
    jz .echo_newline
    mov al, [rsi + r8 - 1]
    cmp al, ' '
    je .echo_trim_one
    cmp al, 9
    jne .echo_write
.echo_trim_one:
    dec r8
    jmp .echo_trim_trailing
.echo_write:
    test r8, r8
    jz .echo_newline
    mov rdx, r8
    call write_stdout_n
.echo_newline:
    mov rsi, newline_text
    mov edx, 2
    call write_stdout_n
    jmp .success

.shared_command_exit:
    mov ecx, eax
    call [ExitProcess]

.startup_error:
    mov rsi, startup_error_text
    call write_stdout
    mov ecx, 2
    jmp .exit

.no_command:
    mov rsi, help_text
    call write_stdout
.success:
    xor ecx, ecx
    jmp .exit
.failure:
    mov ecx, 1
    jmp .exit

.exit:
    call [ExitProcess]

; Return the next argument in RAX, advancing RSI. Quoted paths are unwrapped.
first_argument:
.arg_skip:
    cmp byte [rsi], ' '
    je .arg_skip_one
    cmp byte [rsi], 9
    jne .arg_begin
.arg_skip_one:
    inc rsi
    jmp .arg_skip
.arg_begin:
    cmp byte [rsi], 0
    je .arg_missing
    mov rax, rsi
    mov r8, rsi
    mov r9, rsi
    cmp byte [r8], '"'
    jne .arg_plain
    inc r8
.arg_quoted:
    mov dl, [r8]
    test dl, dl
    jz .arg_finish
    cmp dl, '"'
    je .arg_quote_end
    mov [r9], dl
    inc r8
    inc r9
    jmp .arg_quoted
.arg_quote_end:
    inc r8
    jmp .arg_finish
.arg_plain:
    mov dl, [r8]
    test dl, dl
    jz .arg_finish
    cmp dl, ' '
    je .arg_delimiter
    cmp dl, 9
    je .arg_delimiter
    mov [r9], dl
    inc r8
    inc r9
    jmp .arg_plain
.arg_delimiter:
    inc r8
.arg_finish:
    mov byte [r9], 0
.arg_skip_next:
    cmp byte [r8], ' '
    je .arg_skip_next_one
    cmp byte [r8], 9
    jne .arg_return
.arg_skip_next_one:
    inc r8
    jmp .arg_skip_next
.arg_return:
    mov rsi, r8
    ret
.arg_missing:
    xor eax, eax
    ret

; Compare zero-terminated strings: RDI and RSI. EAX=1 if equal.
string_equal:
.loop:
    mov al, [rdi]
    cmp al, [r10]
    jne .no
    test al, al
    jz .yes
    inc rdi
    inc r10
    jmp .loop
.yes:
    mov eax, 1
    ret
.no:
    xor eax, eax
    ret

; Return the length of the zero-terminated string in RDI.
string_length:
    xor eax, eax
.loop:
    cmp byte [rdi + rax], 0
    je .done
    inc rax
    jmp .loop
.done:
    ret

; Write a zero-terminated string in RSI to standard output.
write_stdout:
    sub rsp, 40
    mov rdi, rsi
    call string_length
    mov rdx, rax
    mov rsi, rdi
    add rsp, 40
    jmp write_stdout_n

; Write RDX bytes from RSI to standard output.
write_stdout_n:
    ; Keep the WriteFile byte-count output outside the Win64 shadow space.
    sub rsp, 72
    mov [rsp + 40], rsi
    mov [rsp + 48], rdx
    mov ecx, -11                 ; STD_OUTPUT_HANDLE
    call [GetStdHandle]
    mov rcx, rax
    mov rdx, [rsp + 40]
    mov r8, [rsp + 48]
    lea r9, [rsp + 56]
    mov qword [rsp + 56], 0
    mov qword [rsp + 32], 0      ; lpOverlapped = NULL
    call [WriteFile]
    add rsp, 72
    ret

include 'src/common/memory.inc'
include 'src/common/strings.inc'
include 'src/common/arguments.inc'
include 'src/common/utf.inc'
include 'src/common/formatting.inc'
include 'src/common/output.inc'
include 'src/common/startup.inc'
include 'src/common/dispatcher.inc'
include 'src/commands/basic.inc'
include 'src/commands/path.inc'
include 'src/commands/cat.inc'
include 'src/commands/ls.inc'
include 'src/commands/mkdir.inc'
include 'src/commands/rmdir.inc'
include 'src/commands/rm.inc'
include 'src/commands/stat.inc'
include 'src/commands/touch.inc'
include 'src/commands/cp.inc'
include 'src/commands/mv.inc'
include 'src/common/errors.inc'
include 'src/platform/windows/io.inc'
include 'src/platform/windows/filesystem.inc'

section '.rdata' data readable
include 'src/common/errors_data.inc'
include 'src/commands/basic_data.inc'
include 'src/commands/path_data.inc'

help_name db 'help', 0
echo_name db 'echo', 0
true_name db 'true', 0
false_name db 'false', 0
pwd_name db 'pwd', 0
hostname_name db 'hostname', 0
ls_name db 'ls', 0
cp_name db 'cp', 0
mv_name db 'mv', 0
basename_name db 'basename', 0
dirname_name db 'dirname', 0
head_name db 'head', 0
mkdir_name db 'mkdir', 0
rmdir_name db 'rmdir', 0
rm_name db 'rm', 0
touch_name db 'touch', 0
cat_name db 'cat', 0
help_text db 'Flatcore - native Windows x86-64 multicall userspace', 13, 10
          db 'Usage: flatcore.exe COMMAND [ARGUMENTS...]', 13, 10
          db 'Windows commands: help, echo, true, false, pwd, hostname, ls, cp, mv, basename, dirname, head, mkdir, rmdir, rm, touch, cat, stat', 13, 10, 0
error_text db 'flatcore: Windows API operation failed', 13, 10, 0
usage_error_text db 'flatcore: command requires a path argument', 13, 10, 0
unknown_text db 'flatcore: unknown command in this Windows build', 13, 10, 0
newline_text db 13, 10
dot_path db '.', 0

section '.data' data readable writeable
include 'src/platform/windows/data.inc'
windows_executable_name db 'flatcore.exe', 0
windows_argc rq 1
windows_normalized_argc rq 1
windows_normalized_argv rq 1
windows_utf8_command_line rb 131072
windows_arg_storage rb 131072
windows_legacy_command_line rb 131072
align 8
windows_argv rq 16385
startup_error_text db 'flatcore: invalid or oversized Windows command line', 13, 10, 0
cwd_buffer rb 4096
computer_name_buffer rb 256
computer_name_length dd 256
ls_pattern rb 4096
find_data rb 592
current_file_time dq 0
io_buffer rb 4096

section '.idata' import data readable writeable

library kernel32, 'KERNEL32.DLL'

import kernel32, \
    GetCommandLineA, 'GetCommandLineA', \
    GetCommandLineW, 'GetCommandLineW', \
    GetLastError, 'GetLastError', \
    CreateFileW, 'CreateFileW', \
    CreateDirectoryW, 'CreateDirectoryW', \
    RemoveDirectoryW, 'RemoveDirectoryW', \
    DeleteFileW, 'DeleteFileW', \
    MoveFileExW, 'MoveFileExW', \
    GetCurrentDirectoryW, 'GetCurrentDirectoryW', \
    GetFileAttributesExW, 'GetFileAttributesExW', \
    FindFirstFileW, 'FindFirstFileW', \
    FindNextFileW, 'FindNextFileW', \
    SetCurrentDirectoryW, 'SetCurrentDirectoryW', \
    GetCurrentDirectoryA, 'GetCurrentDirectoryA', \
    GetComputerNameA, 'GetComputerNameA', \
    CreateDirectoryA, 'CreateDirectoryA', \
    RemoveDirectoryA, 'RemoveDirectoryA', \
    DeleteFileA, 'DeleteFileA', \
    CreateFileA, 'CreateFileA', \
    ReadFile, 'ReadFile', \
    CloseHandle, 'CloseHandle', \
    GetSystemTimeAsFileTime, 'GetSystemTimeAsFileTime', \
    SetFileTime, 'SetFileTime', \
    FindFirstFileA, 'FindFirstFileA', \
    FindNextFileA, 'FindNextFileA', \
    FindClose, 'FindClose', \
    CopyFileA, 'CopyFileA', \
    MoveFileA, 'MoveFileA', \
    GetStdHandle, 'GetStdHandle', \
    WriteFile, 'WriteFile', \
    ExitProcess, 'ExitProcess', \
    GetFileInfoByHandle, 'GetFileInformationByHandle', \
    SetEndOfFileAPI, 'SetEndOfFile'
