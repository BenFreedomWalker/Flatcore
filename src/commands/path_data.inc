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
fc_basic_registry_count = 8

fc_cmd_true_name     db 'true', 0
fc_cmd_false_name    db 'false', 0
fc_cmd_echo_name     db 'echo', 0
fc_cmd_pwd_name      db 'pwd', 0
fc_cmd_basename_name db 'basename', 0
fc_cmd_dirname_name  db 'dirname', 0
fc_cmd_cat_name      db 'cat', 0
fc_cmd_ls_name       db 'ls', 0
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

fc_cmd_ls_name db 'ls', 0
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
