; Windows backend state and scratch storage.
align 8
os_stdin dq 0
os_stdout dq 0
os_stderr dq 0
os_error_code dd FC_ERR_NONE
os_error_domain dd FC_ERROR_DOMAIN_NONE
os_native_error dq 0
align 16
windows_path_buffer rb 65536
windows_path_buffer2 rb 65536

align 8
windows_file_data rb 40

; Independent FindFirstFileW iterators (active, pending, HANDLE, FIND_DATA).
WINDOWS_DIR_SLOT_COUNT = 8
WINDOWS_DIR_DATA_SIZE = 592
WINDOWS_DIR_SLOT_SIZE = 16 + WINDOWS_DIR_DATA_SIZE
align 16
windows_dir_slots rb WINDOWS_DIR_SLOT_COUNT * WINDOWS_DIR_SLOT_SIZE
align 16
windows_dir_name_buffer rb 1024
