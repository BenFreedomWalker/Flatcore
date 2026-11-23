; Linux FC64 backend self-test fixtures.
segment readable writeable
linux_test_path db '/tmp/flatcore-fc64-backend-test', 0
linux_test_payload db 'OK'
linux_test_readback rb 8
linux_test_cwd rb 4096
align 8
linux_test_handle dq 0
linux_test_length dq 0

align 8
linux_test_info rb 64

linux_test_dir_path db '/tmp', 0
align 8
linux_test_dir_handle dq 0
linux_test_dir_handle2 dq 0
linux_test_name_length dq 0
linux_test_name_length2 dq 0
linux_test_name rb 256
linux_test_name2 rb 256
linux_test_info2 rb 64
