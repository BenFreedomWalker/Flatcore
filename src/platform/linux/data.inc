; Linux backend process state.
segment readable writeable
align 8
os_stdin dq 0
os_stdout dq 1
os_stderr dq 2
os_error_code dd FC_ERR_NONE
os_error_domain dd FC_ERROR_DOMAIN_NONE
os_native_error dq 0
