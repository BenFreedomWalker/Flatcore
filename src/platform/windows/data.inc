; Windows backend state and scratch storage.
align 8
os_stdin dq 0
os_stdout dq 0
os_stderr dq 0
os_error_code dd FC_ERR_NONE
os_error_domain dd FC_ERROR_DOMAIN_NONE
os_native_error dq 0

align 16
fc_cmd_path_buffer rb 4096
fc_cmd_path_length dq 0
align 16
fc_cmd_cat_buffer rb 4096
align 16
fc_cmd_ls_flags dq 0
fc_cmd_ls_dir_handle dq 0
fc_cmd_ls_name_length dq 0
fc_cmd_ls_info rb 64
fc_cmd_ls_entry_name rb 1024
fc_cmd_ls_full_path rb 8192
fc_cmd_ls_mode rb 10
fc_cmd_ls_number rb 32
align 16
fc_cmd_mkdir_flags dq 0
fc_cmd_mkdir_mode dq 0777o
fc_cmd_mkdir_info rb 64
fc_cmd_mkdir_work_path rb 8192
align 16
fc_cmd_rmdir_flags dq 0
fc_cmd_rmdir_work_path rb 8192
align 16
fc_cmd_rm_flags dq 0
fc_cmd_rm_info rb 64
align 16
windows_path_buffer rb 65536
windows_path_buffer2 rb 65536

align 8
windows_file_data rb 40
windows_stat_find_data rb 592
align 8
fc_cmd_stat_info rb 64
fc_cmd_stat_number rb 32
align 8
fc_cmd_touch_info rb 64
fc_cmd_touch_ref_info rb 64
fc_cmd_touch_reference dq 0
fc_cmd_touch_handle dq 0
fc_cmd_touch_now dq 0

; Independent FindFirstFileW iterators (active, pending, HANDLE, FIND_DATA).
align 16
windows_dir_slots rb 8 * (16 + 592)
align 16
windows_dir_name_buffer rb 1024
