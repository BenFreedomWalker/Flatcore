; Windows backend test fixtures.
windows_test_path db 'build\windows\flatcore-测试.tmp', 0
windows_test_renamed_path db 'build\windows\flatcore-改名.tmp', 0
windows_test_dir db 'build\windows\flatcore-测试-dir', 0
windows_test_payload db 'OK'
align 8
windows_test_length rq 1
windows_test_handle rq 1
windows_test_cwd rb 4096
windows_test_readback rb 8

align 8
windows_test_info rb 64
