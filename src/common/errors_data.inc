; Static UTF-8 diagnostics for fc_error_message.
fc_error_none         db 'no error', 0
fc_error_not_found    db 'not found', 0
fc_error_exists       db 'already exists', 0
fc_error_access       db 'permission denied', 0
fc_error_not_dir      db 'not a directory', 0
fc_error_is_dir       db 'is a directory', 0
fc_error_not_empty    db 'directory not empty', 0
fc_error_invalid      db 'invalid argument', 0
fc_error_no_memory    db 'out of memory', 0
fc_error_no_space     db 'no space left', 0
fc_error_read_only    db 'read-only filesystem', 0
fc_error_io           db 'I/O error', 0
fc_error_encoding     db 'text encoding error', 0
fc_error_unsupported  db 'operation not supported', 0
fc_error_too_small    db 'buffer too small', 0
fc_error_interrupted  db 'operation interrupted', 0
fc_error_unknown      db 'unknown error', 0
