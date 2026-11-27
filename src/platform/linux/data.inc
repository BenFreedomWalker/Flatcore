; Linux backend process state.
segment readable writeable
align 8
os_stdin dq 0
os_stdout dq 1
os_stderr dq 2
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
align 8
fc_cmd_cp_source_handle dq 0
fc_cmd_cp_destination_handle dq 0
fc_cmd_cp_read_count dq 0
align 16
fc_cmd_cp_buffer rb 4096

align 8
fc_cmd_stat_info rb 64
fc_cmd_stat_number rb 32
align 8
fc_cmd_touch_info rb 64
fc_cmd_touch_ref_info rb 64
fc_cmd_touch_reference dq 0
fc_cmd_touch_handle dq 0
fc_cmd_touch_now dq 0

align 16
linux_stat_buffer rb 144
linux_stat_buffer2 rb 144
linux_time_buffer rb 16

align 16
; Eight independent directory iterator contexts: state, fd, position, count, buffer.
linux_dir_slots rb 8 * 4112
