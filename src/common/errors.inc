; Flatcore shared normalized error descriptions (FC64).
; fc_error_message(error_code) -> RAX=pointer to a static UTF-8 message.
; This maps normalized codes only; native details remain in os_native_error.

fc_error_message:
    cmp edi, FC_ERR_NONE
    je .none
    cmp edi, FC_ERR_NOT_FOUND
    je .not_found
    cmp edi, FC_ERR_EXISTS
    je .exists
    cmp edi, FC_ERR_ACCESS
    je .access
    cmp edi, FC_ERR_NOT_DIR
    je .not_dir
    cmp edi, FC_ERR_IS_DIR
    je .is_dir
    cmp edi, FC_ERR_NOT_EMPTY
    je .not_empty
    cmp edi, FC_ERR_INVALID
    je .invalid
    cmp edi, FC_ERR_NO_MEMORY
    je .no_memory
    cmp edi, FC_ERR_NO_SPACE
    je .no_space
    cmp edi, FC_ERR_READ_ONLY
    je .read_only
    cmp edi, FC_ERR_IO
    je .io
    cmp edi, FC_ERR_ENCODING
    je .encoding
    cmp edi, FC_ERR_UNSUPPORTED
    je .unsupported
    cmp edi, FC_ERR_TOO_SMALL
    je .too_small
    cmp edi, FC_ERR_INTERRUPTED
    je .interrupted
    mov rax, fc_error_unknown
    ret
.none:
    mov rax, fc_error_none
    ret
.not_found:
    mov rax, fc_error_not_found
    ret
.exists:
    mov rax, fc_error_exists
    ret
.access:
    mov rax, fc_error_access
    ret
.not_dir:
    mov rax, fc_error_not_dir
    ret
.is_dir:
    mov rax, fc_error_is_dir
    ret
.not_empty:
    mov rax, fc_error_not_empty
    ret
.invalid:
    mov rax, fc_error_invalid
    ret
.no_memory:
    mov rax, fc_error_no_memory
    ret
.no_space:
    mov rax, fc_error_no_space
    ret
.read_only:
    mov rax, fc_error_read_only
    ret
.io:
    mov rax, fc_error_io
    ret
.encoding:
    mov rax, fc_error_encoding
    ret
.unsupported:
    mov rax, fc_error_unsupported
    ret
.too_small:
    mov rax, fc_error_too_small
    ret
.interrupted:
    mov rax, fc_error_interrupted
    ret

