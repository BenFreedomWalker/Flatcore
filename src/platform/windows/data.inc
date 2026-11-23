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
windows_stat_find_data rb 592

; Independent FindFirstFileW iterators (active, pending, HANDLE, FIND_DATA).
align 16
windows_dir_slots rb 8 * (16 + 592)
align 16
windows_dir_name_buffer rb 1024
