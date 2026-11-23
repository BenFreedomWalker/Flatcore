; Shared path-command registry and immutable diagnostics.
fc_basic_registry:
    dq fc_cmd_true_name, fc_cmd_true
    dq fc_cmd_false_name, fc_cmd_false
    dq fc_cmd_echo_name, fc_cmd_echo
    dq fc_cmd_pwd_name, fc_cmd_pwd
    dq fc_cmd_basename_name, fc_cmd_basename
    dq fc_cmd_dirname_name, fc_cmd_dirname
fc_basic_registry_count = 6

fc_cmd_true_name     db 'true', 0
fc_cmd_false_name    db 'false', 0
fc_cmd_echo_name     db 'echo', 0
fc_cmd_pwd_name      db 'pwd', 0
fc_cmd_basename_name db 'basename', 0
fc_cmd_dirname_name  db 'dirname', 0
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
