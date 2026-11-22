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
