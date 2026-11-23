; Shared command registry and immutable data.
fc_basic_registry:
    dq fc_cmd_true_name, fc_cmd_true
    dq fc_cmd_false_name, fc_cmd_false
    dq fc_cmd_echo_name, fc_cmd_echo
fc_basic_registry_count = 3

fc_cmd_true_name  db 'true', 0
fc_cmd_false_name db 'false', 0
fc_cmd_echo_name  db 'echo', 0
fc_cmd_echo_no_newline db '-n', 0
fc_cmd_space db ' '
fc_cmd_newline db 10
