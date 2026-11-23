; Flatcore shared runtime / OS backend contract
; Stage 1 ABI specification. Included by shared code and both native backends.
;
; INTERNAL CALLING CONVENTION (FC64)
; ----------------------------------
; This is a private convention for Flatcore functions, not the Linux syscall
; ABI and not the Microsoft x64 ABI.
;
; Arguments 1..6: RDI, RSI, RDX, RCX, R8, R9.
; Integer/pointer result: RAX. Additional results use caller-owned pointers.
; Caller-saved: RAX, RCX, RDX, RSI, RDI, R8, R9, R10, R11, and RFLAGS.
; Callee-saved: RBX, RBP, R12, R13, R14, R15, and RSP.
; A callee must restore every callee-saved register it changes.
; The caller aligns RSP to 16 bytes immediately before CALL. Thus RSP is
; 8 modulo 16 at function entry. There are no stack-passed arguments in v1;
; pass a pointer to a parameter block when more than six arguments are needed.
; A function may use caller-saved registers freely. Do not assume flags survive
; a call. Leaf functions follow the same preservation rules.
;
; Native boundaries:
; - Linux entry/backend adapts FC64 calls to the Linux x86-64 syscall ABI.
; - Windows entry/backend adapts FC64 calls to the Microsoft x64 ABI, including
;   32-byte shadow space, required stack alignment, and nonvolatile registers.
; - Native API/syscall register conventions must never leak into shared code.
;
; RETURN AND ERROR CONTRACT
; -------------------------
; Backend functions return RAX=0 on success and RAX=-1 on failure unless their
; individual contract explicitly defines a different result (directory next).
; Output values are written through caller-provided pointers.
; On failure, os_error_code contains a normalized FC_ERR_* value and
; os_native_error contains the unmodified native OS error value. Both are
; process-global diagnostic state; callers must inspect them before another
; backend call can replace them. os_error_domain identifies the native source.
; Commands return an exit status in EAX to the common dispatcher. Commands
; never invoke the platform process-exit primitive.
;
; HANDLE AND STANDARD STREAM CONTRACT
; -----------------------------------
; OS_HANDLE is an opaque unsigned 64-bit token owned by the backend.
; FC_INVALID_HANDLE is reserved and must never identify a valid handle.
; Standard input/output/error handles are initialized by the native entry
; point in os_stdin, os_stdout, and os_stderr. Shared code must not assume
; that these values are Unix descriptors or Windows HANDLE values.
; Backend close operations must reject standard handles unless explicitly
; documented otherwise.
;
; PATH / STRING CONTRACT
; ----------------------
; All shared strings and path arguments are NUL-terminated UTF-8.
; Lengths are byte counts, not character counts. No implicit normalization
; or case folding is performed by the common runtime.
; Linux passes UTF-8 path bytes to native syscalls.
; Windows converts UTF-8 paths to UTF-16 and uses wide (W-suffixed) APIs.
; Conversion failures are reported as FC_ERR_ENCODING.
; A backend must not silently replace invalid input sequences.
;
; FILE METADATA CONTRACT (FC_FILE_INFO)
; -------------------------------------
; Fixed 64-byte layout, all integer fields little-endian:
;   +0  dq size_bytes       (unsigned file size)
;   +8  dq modified_ns      (nanoseconds since Unix epoch, if available)
;   +16 dq created_ns       (0 when unavailable)
;   +24 dq accessed_ns      (0 when unavailable)
;   +32 dd attributes       (FC_ATTR_* bitmask)
;   +36 dd file_type        (FC_TYPE_*)
;   +40 dd mode             (POSIX mode when meaningful, else 0)
;   +44 dd link_count       (0 when unavailable)
;   +48 dq reserved[0]
;   +56 dq reserved[1]
; Backends zero the whole structure before populating it. Unsupported values
; are zero, not guessed. Timestamps use UTC Unix-epoch nanoseconds.
;
; DIRECTORY ITERATION CONTRACT
; ----------------------------
; os_dir_open(path, out_dir_handle): 0 success, -1 failure.
; os_dir_next(dir_handle, out_info, name_buffer, name_capacity, out_name_len):
;   RAX=1 entry returned; RAX=0 end of directory; RAX=-1 error.
;   Name is UTF-8, not NUL-terminated unless capacity permits; out_name_len
;   reports bytes excluding any terminator. A too-small buffer is an error.
; os_dir_close(dir_handle): 0 success, -1 failure.
; "." and ".." are returned when the native API exposes them; shared ls logic
; decides whether to display them.
;
; BACKEND CAPABILITIES
; --------------------
; os_get_capabilities() returns a bitmask in RAX. A clear bit means the
; operation is unavailable or cannot be represented faithfully on this OS.
; Shared commands must test the relevant bit before requesting optional
; behavior and report an explicit unsupported-operation diagnostic.
;
; PUBLIC BACKEND ENTRY POINT CONTRACTS (FC64)
; -------------------------------------------
; os_read(handle, buffer, byte_count, out_bytes_read)
; os_write(handle, buffer, byte_count, out_bytes_written)
; os_open(path_utf8, flags, mode, out_handle)
; os_close(handle)
; os_stat(path_utf8, out_fc_file_info)
; os_dir_open(path_utf8, out_dir_handle)
; os_dir_next(dir_handle, out_fc_file_info, name_buffer, name_capacity,
;             out_name_len) -- special RAX result documented above.
; os_dir_close(dir_handle)
; os_mkdir(path_utf8, mode)
; os_rmdir(path_utf8)
; os_unlink(path_utf8)
; os_rename(old_path_utf8, new_path_utf8)
; os_getcwd(buffer, capacity, out_length)
; os_chdir(path_utf8)
; os_set_times(path_utf8, accessed_ns, modified_ns)
; os_get_capabilities() returns FC_CAP_* flags. FC_CAP_SYMLINKS means
; os_stat/os_dir_next can identify symbolic links without following them;
; it does not promise link creation or readlink APIs. FC_CAP_HARDLINKS means
; the backend exposes hard-link operations. FC_CAP_POSIX_MODES means mode
; metadata is meaningful. FC_CAP_FILE_TIMES means os_set_times is supported.
; FC_CAP_PROCESS_SIGNALS and FC_CAP_OWNER_IDS are reserved until corresponding
; public backend operations/metadata are exposed.
; os_process_exit(exit_status) -- does not return.
;
; Error-domain and normalized error constants.
FC_ERROR_DOMAIN_NONE    = 0
FC_ERROR_DOMAIN_LINUX   = 1
FC_ERROR_DOMAIN_WINDOWS = 2

FC_ERR_NONE       = 0
FC_ERR_NOT_FOUND  = 1
FC_ERR_EXISTS     = 2
FC_ERR_ACCESS     = 3
FC_ERR_NOT_DIR    = 4
FC_ERR_IS_DIR     = 5
FC_ERR_NOT_EMPTY  = 6
FC_ERR_INVALID    = 7
FC_ERR_NO_MEMORY  = 8
FC_ERR_NO_SPACE   = 9
FC_ERR_READ_ONLY  = 10
FC_ERR_IO         = 11
FC_ERR_ENCODING   = 12
FC_ERR_UNSUPPORTED = 13
FC_ERR_TOO_SMALL  = 14
FC_ERR_INTERRUPTED = 15
FC_ERR_UNKNOWN    = 255

; File open flags (combinable).
FC_OPEN_READ       = 0001b
FC_OPEN_WRITE      = 0010b
FC_OPEN_CREATE     = 0100b
FC_OPEN_TRUNCATE   = 1000b
FC_OPEN_APPEND     = 10000b
FC_OPEN_EXCLUSIVE  = 100000b
FC_OPEN_DIRECTORY  = 1000000b

; File types.
FC_TYPE_UNKNOWN = 0
FC_TYPE_REGULAR = 1
FC_TYPE_DIRECTORY = 2
FC_TYPE_SYMLINK = 3
FC_TYPE_FIFO = 4
FC_TYPE_SOCKET = 5
FC_TYPE_DEVICE = 6

; Attribute bits.
FC_ATTR_HIDDEN    = 0001b
FC_ATTR_READ_ONLY = 0010b
FC_ATTR_SYSTEM    = 0100b
FC_ATTR_ARCHIVE   = 1000b

; Optional backend capabilities.
FC_CAP_SYMLINKS       = 0001b
FC_CAP_HARDLINKS      = 0010b
FC_CAP_POSIX_MODES    = 0100b
FC_CAP_FILE_TIMES     = 1000b
FC_CAP_PROCESS_SIGNALS = 10000b
FC_CAP_OWNER_IDS      = 100000b

; Backend diagnostic state, allocated by the common runtime.
; os_error_code dd FC_ERR_*; os_error_domain dd FC_ERROR_DOMAIN_*;
; os_native_error dq native errno / GetLastError() value.
; os_stdin, os_stdout, os_stderr: OS_HANDLE values initialized at startup.
; FC_INVALID_HANDLE is an opaque sentinel shared by both backends.
FC_INVALID_HANDLE = -1
