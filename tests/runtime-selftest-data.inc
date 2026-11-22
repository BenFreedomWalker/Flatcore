; Runtime self-test fixtures and writable output buffers.
test_text_hello db 'hello', 0
test_text_hello_copy db 'hello', 0
test_uint64_max db '18446744073709551615', 0
test_quoted_argument db 'hello world', 0
test_executable_name db 'flatcore', 0
test_multicall_path db '/usr/bin/echo', 0
test_multicall_command db 'echo', 0
test_dispatch_name db 'probe', 0
test_error_access db 'permission denied', 0
test_utf16 dw 0041h, 07FFh, 0800h, 0D83Dh, 0DE00h, 0
test_utf8_expected db 041h, 0DFh, 0BFh, 0E0h, 0A0h, 080h, 0F0h, 09Fh, 098h, 080h, 0
test_command_line db 'flatcore echo "hello world" plain', 0
test_registry dq test_dispatch_name, test_dispatch_handler

align 8
test_copy_buffer rb 16
test_number_buffer rb 24
test_utf8_buffer rb 16
test_arg_storage rb 128
test_argv rq 9
test_normalized_argc rq 1
test_normalized_argv rq 1
test_dispatch_argv rq 1
