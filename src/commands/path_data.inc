; Shared path-command registry and immutable diagnostics.
fc_basic_registry:
    dq fc_cmd_true_name, fc_cmd_true
    dq fc_cmd_false_name, fc_cmd_false
    dq fc_cmd_echo_name, fc_cmd_echo
    dq fc_cmd_pwd_name, fc_cmd_pwd
    dq fc_cmd_basename_name, fc_cmd_basename
    dq fc_cmd_dirname_name, fc_cmd_dirname
    dq fc_cmd_cat_name, fc_cmd_cat
    dq fc_cmd_ls_name, fc_cmd_ls
    dq fc_cmd_mkdir_name, fc_cmd_mkdir
    dq fc_cmd_rmdir_name, fc_cmd_rmdir
    dq fc_cmd_rm_name, fc_cmd_rm
    dq fc_cmd_stat_name, fc_cmd_stat
    dq fc_cmd_touch_name, fc_cmd_touch
    dq fc_cmd_cp_name, fc_cmd_cp
    dq fc_cmd_mv_name, fc_cmd_mv
fc_basic_registry_count = 15

fc_cmd_true_name     db 'true', 0
fc_cmd_false_name    db 'false', 0
fc_cmd_echo_name     db 'echo', 0
fc_cmd_pwd_name      db 'pwd', 0
fc_cmd_basename_name db 'basename', 0
fc_cmd_dirname_name  db 'dirname', 0
fc_cmd_cat_name      db 'cat', 0
fc_cmd_ls_name       db 'ls', 0
fc_cmd_mkdir_name    db 'mkdir', 0
fc_cmd_rmdir_name    db 'rmdir', 0
fc_cmd_rm_name       db 'rm', 0
fc_cmd_stat_name     db 'stat', 0
fc_cmd_touch_name    db 'touch', 0
fc_cmd_cp_name       db 'cp', 0
fc_cmd_mv_name       db 'mv', 0
fc_cmd_cat_stdin_name db '-', 0
fc_cmd_cat_error_text db 'flatcore: cat: input/output error', 10
fc_cmd_cat_error_length = $ - fc_cmd_cat_error_text
fc_cmd_echo_no_newline db '-n', 0
fc_cmd_space db ' '
fc_cmd_newline db 10

fc_cmd_path_newline db 10
fc_cmd_dot db '.', 0
fc_cmd_pwd_usage_text db 'usage: pwd', 10, 0
fc_cmd_basename_usage_text db 'usage: basename PATH', 10, 0
fc_cmd_dirname_usage_text db 'usage: dirname PATH', 10, 0
fc_cmd_pwd_error db 'flatcore: pwd: cannot get current directory', 10
fc_cmd_pwd_error_length = $ - fc_cmd_pwd_error

fc_cmd_ls_dot db '.', 0
fc_cmd_ls_opt_all db '--all', 0
fc_cmd_ls_opt_almost db '--almost-all', 0
fc_cmd_ls_opt_directory db '--directory', 0
fc_cmd_ls_opt_help db '--help', 0
fc_cmd_ls_opt_version db '--version', 0
fc_cmd_ls_error_text db 'flatcore: ls: operation failed', 10
fc_cmd_ls_error_length = $ - fc_cmd_ls_error_text
fc_cmd_ls_bad_option_text db 'flatcore: ls: invalid option', 10
fc_cmd_ls_bad_option_length = $ - fc_cmd_ls_bad_option_text
fc_cmd_ls_help_text db 'Usage: ls [OPTION]... [FILE]...', 10
                  db '  -a, --all          include . and ..', 10
                  db '  -A, --almost-all   include hidden entries except . and ..', 10
                  db '  -l                 use long listing format', 10
                  db '  -d, --directory    list directories themselves', 10
                  db '  -1                 one entry per line', 10
                  db '  --                 end option parsing', 10
fc_cmd_ls_help_length = $ - fc_cmd_ls_help_text
fc_cmd_ls_version_text db 'Flatcore ls 0.1', 10
fc_cmd_ls_version_length = $ - fc_cmd_ls_version_text
fc_cmd_ls_perm_bits dw 256, 128, 64, 32, 16, 8, 4, 2, 1
fc_cmd_ls_perm_chars db 'rwxrwxrwx'

; Shared mkdir command options, diagnostics, and immutable output strings.
fc_cmd_mkdir_opt_parents db '--parents', 0
fc_cmd_mkdir_opt_verbose db '--verbose', 0
fc_cmd_mkdir_opt_help db '--help', 0
fc_cmd_mkdir_opt_version db '--version', 0
fc_cmd_mkdir_opt_mode_prefix db '--mode=', 0
fc_cmd_mkdir_usage_text db 'usage: mkdir [-pv] [-m MODE] [--] DIRECTORY...', 10
fc_cmd_mkdir_usage_length = $ - fc_cmd_mkdir_usage_text
fc_cmd_mkdir_bad_option_text db 'flatcore: mkdir: invalid option', 10
fc_cmd_mkdir_bad_option_length = $ - fc_cmd_mkdir_bad_option_text
fc_cmd_mkdir_bad_mode_text db 'flatcore: mkdir: invalid mode', 10
fc_cmd_mkdir_bad_mode_length = $ - fc_cmd_mkdir_bad_mode_text
fc_cmd_mkdir_error_prefix db 'flatcore: mkdir: cannot create directory ', 39
fc_cmd_mkdir_error_prefix_length = $ - fc_cmd_mkdir_error_prefix
fc_cmd_mkdir_error_suffix db 39, ': operation failed', 10
fc_cmd_mkdir_error_suffix_length = $ - fc_cmd_mkdir_error_suffix
fc_cmd_mkdir_verbose_prefix db 'mkdir: created directory ', 39
fc_cmd_mkdir_verbose_prefix_length = $ - fc_cmd_mkdir_verbose_prefix
fc_cmd_mkdir_quote_newline db 39, 10
fc_cmd_mkdir_help_text db 'Usage: mkdir [OPTION]... DIRECTORY...', 10
                    db '  -p, --parents  create parent directories as needed', 10
                    db '  -m, --mode=MODE set file mode (octal; ignored by Windows)', 10
                    db '  -v, --verbose  print a message for each created directory', 10
                    db '  --             end option parsing', 10
fc_cmd_mkdir_help_length = $ - fc_cmd_mkdir_help_text
fc_cmd_mkdir_version_text db 'Flatcore mkdir 0.1', 10
fc_cmd_mkdir_version_length = $ - fc_cmd_mkdir_version_text

; Shared rmdir command options, diagnostics, and immutable output strings.
fc_cmd_rmdir_opt_parents db '--parents', 0
fc_cmd_rmdir_opt_verbose db '--verbose', 0
fc_cmd_rmdir_opt_help db '--help', 0
fc_cmd_rmdir_opt_version db '--version', 0
fc_cmd_rmdir_usage_text db 'usage: rmdir [-pv] [--] DIRECTORY...', 10
fc_cmd_rmdir_usage_length = $ - fc_cmd_rmdir_usage_text
fc_cmd_rmdir_bad_option_text db 'flatcore: rmdir: invalid option', 10
fc_cmd_rmdir_bad_option_length = $ - fc_cmd_rmdir_bad_option_text
fc_cmd_rmdir_error_prefix db 'flatcore: rmdir: failed to remove ', 39
fc_cmd_rmdir_error_prefix_length = $ - fc_cmd_rmdir_error_prefix
fc_cmd_rmdir_error_suffix db 39, ': operation failed', 10
fc_cmd_rmdir_error_suffix_length = $ - fc_cmd_rmdir_error_suffix
fc_cmd_rmdir_verbose_prefix db 'rmdir: removed ', 39
fc_cmd_rmdir_verbose_prefix_length = $ - fc_cmd_rmdir_verbose_prefix
fc_cmd_rmdir_quote_newline db 39, 10
fc_cmd_rmdir_help_text db 'Usage: rmdir [OPTION]... DIRECTORY...', 10
                    db '  -p, --parents  remove DIRECTORY and its empty parents', 10
                    db '  -v, --verbose  output a diagnostic for every directory processed', 10
                    db '  --             end option parsing', 10
fc_cmd_rmdir_help_length = $ - fc_cmd_rmdir_help_text
fc_cmd_rmdir_version_text db 'Flatcore rmdir 0.1', 10
fc_cmd_rmdir_version_length = $ - fc_cmd_rmdir_version_text

; Shared rm command options, diagnostics, and immutable output strings.
fc_cmd_rm_opt_force db '--force', 0
fc_cmd_rm_opt_dir db '--dir', 0
fc_cmd_rm_opt_verbose db '--verbose', 0
fc_cmd_rm_opt_help db '--help', 0
fc_cmd_rm_opt_version db '--version', 0
fc_cmd_rm_usage_text db 'usage: rm [-dfv] [--] FILE...', 10
fc_cmd_rm_usage_length = $ - fc_cmd_rm_usage_text
fc_cmd_rm_bad_option_text db 'flatcore: rm: invalid option', 10
fc_cmd_rm_bad_option_length = $ - fc_cmd_rm_bad_option_text
fc_cmd_rm_error_prefix db 'flatcore: rm: failed to remove ', 39
fc_cmd_rm_error_prefix_length = $ - fc_cmd_rm_error_prefix
fc_cmd_rm_error_suffix db 39, ': operation failed', 10
fc_cmd_rm_error_suffix_length = $ - fc_cmd_rm_error_suffix
fc_cmd_rm_verbose_prefix db 'rm: removed ', 39
fc_cmd_rm_verbose_prefix_length = $ - fc_cmd_rm_verbose_prefix
fc_cmd_rm_quote_newline db 39, 10
fc_cmd_rm_help_text db 'Usage: rm [OPTION]... FILE...', 10
                 db '  -f, --force    ignore nonexistent files', 10
                 db '  -d, --dir      remove empty directories', 10
                 db '  -v, --verbose  explain what is being done', 10
                 db '  --             end option parsing', 10
fc_cmd_rm_help_length = $ - fc_cmd_rm_help_text
fc_cmd_rm_version_text db 'Flatcore rm 0.1', 10
fc_cmd_rm_version_length = $ - fc_cmd_rm_version_text

; Shared stat command output strings.
fc_cmd_stat_file_prefix db '  File: ', 0
fc_cmd_stat_size_prefix db '  Size: ', 0
fc_cmd_stat_bytes_newline db ' bytes', 10, 0
fc_cmd_stat_type_prefix db '  Type: ', 0
fc_cmd_stat_hidden_prefix db '  Hidden: ', 0
fc_cmd_stat_native_hidden_prefix db '  Native hidden: ', 0
fc_cmd_stat_readonly_prefix db '  Read-only: ', 0
fc_cmd_stat_yes_newline db 'yes', 10, 0
fc_cmd_stat_no_newline db 'no', 10, 0
fc_cmd_stat_newline db 10, 0
fc_cmd_stat_type_regular db 'regular file', 0
fc_cmd_stat_type_directory db 'directory', 0
fc_cmd_stat_type_symlink db 'symbolic link', 0
fc_cmd_stat_type_fifo db 'FIFO', 0
fc_cmd_stat_type_socket db 'socket', 0
fc_cmd_stat_type_device db 'device', 0
fc_cmd_stat_type_unknown db 'unknown', 0
fc_cmd_stat_usage_text db 'Usage: stat FILE', 10, 0
fc_cmd_stat_error_prefix db 'flatcore: stat: cannot stat ', 39, 0
fc_cmd_stat_error_suffix db 39, ': operation failed', 10, 0

; Shared touch command options and diagnostics.
fc_cmd_touch_opt_no_create db '--no-create', 0
fc_cmd_touch_opt_reference db '-r', 0
fc_cmd_touch_opt_help db '--help', 0
fc_cmd_touch_opt_version db '--version', 0
fc_cmd_touch_usage_text db 'Usage: touch [-acm] [-r FILE] [--no-create] [--] FILE...', 10
fc_cmd_touch_usage_length = $ - fc_cmd_touch_usage_text
fc_cmd_touch_bad_option_text db 'flatcore: touch: invalid option', 10
fc_cmd_touch_bad_option_length = $ - fc_cmd_touch_bad_option_text
fc_cmd_touch_error_prefix db 'flatcore: touch: cannot touch ', 39
fc_cmd_touch_error_prefix_length = $ - fc_cmd_touch_error_prefix
fc_cmd_touch_error_suffix db 39, ': operation failed', 10
fc_cmd_touch_error_suffix_length = $ - fc_cmd_touch_error_suffix
fc_cmd_touch_help_text db 'Usage: touch [OPTION]... FILE...', 10
                      db '  -r FILE        use FILE timestamps instead of current time', 10
                      db '  -a             change only the access time', 10
                      db '  -m             change only the modification time', 10
                      db '  -c, --no-create do not create missing files', 10
                      db '  --             end option parsing', 10
fc_cmd_touch_help_length = $ - fc_cmd_touch_help_text
fc_cmd_touch_version_text db 'Flatcore touch 0.1', 10
fc_cmd_touch_version_length = $ - fc_cmd_touch_version_text

; Shared cp/mv command diagnostics and scratch state.
fc_cmd_cp_usage_text db 'usage: cp [--] SOURCE DEST', 10
fc_cmd_cp_usage_length = $ - fc_cmd_cp_usage_text
fc_cmd_cp_bad_option_text db 'flatcore: cp: invalid option', 10
fc_cmd_cp_bad_option_length = $ - fc_cmd_cp_bad_option_text
fc_cmd_cp_error_text db 'flatcore: cp: copy failed', 10
fc_cmd_cp_error_length = $ - fc_cmd_cp_error_text
fc_cmd_cp_same_text db 'flatcore: cp: source and destination are the same file', 10
fc_cmd_cp_same_length = $ - fc_cmd_cp_same_text
fc_cmd_cp_source_handle dq 0
fc_cmd_cp_destination_handle dq 0
fc_cmd_cp_read_count dq 0
fc_cmd_cp_buffer rb 4096

fc_cmd_mv_usage_text db 'usage: mv [--] SOURCE DEST', 10
fc_cmd_mv_usage_length = $ - fc_cmd_mv_usage_text
fc_cmd_mv_bad_option_text db 'flatcore: mv: invalid option', 10
fc_cmd_mv_bad_option_length = $ - fc_cmd_mv_bad_option_text
fc_cmd_mv_error_text db 'flatcore: mv: move failed', 10
fc_cmd_mv_error_length = $ - fc_cmd_mv_error_text

fc_cmd_cp_opt_help db '--help', 0
fc_cmd_cp_opt_version db '--version', 0
fc_cmd_cp_help_text db 'Usage: cp [--] SOURCE DEST', 10
                    db '  --       end option parsing', 10
fc_cmd_cp_help_length = $ - fc_cmd_cp_help_text
fc_cmd_cp_version_text db 'Flatcore cp 0.1', 10
fc_cmd_cp_version_length = $ - fc_cmd_cp_version_text

fc_cmd_mv_opt_help db '--help', 0
fc_cmd_mv_opt_version db '--version', 0
fc_cmd_mv_help_text db 'Usage: mv [--] SOURCE DEST', 10
                    db '  --       end option parsing', 10
fc_cmd_mv_help_length = $ - fc_cmd_mv_help_text
fc_cmd_mv_version_text db 'Flatcore mv 0.1', 10
fc_cmd_mv_version_length = $ - fc_cmd_mv_version_text
