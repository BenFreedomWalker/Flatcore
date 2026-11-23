segment readable writeable

argc         rq 1
argv         rq 1
normalized_argc rq 1
normalized_argv rq 1
io_buffer    rb 4096
cwd_buffer   rb 4096
uname_buffer rb 390
num_buffer   rb 32
sysinfo_buffer rb 112
sleep_request rq 2
sleep_remaining rq 2

binary_name db 'flatcore', 0

name_help   db 'help', 0
name_echo   db 'echo', 0
name_true   db 'true', 0
name_false  db 'false', 0
name_pwd    db 'pwd', 0
name_cat    db 'cat', 0
name_ls     db 'ls', 0
name_mkdir  db 'mkdir', 0
name_rmdir  db 'rmdir', 0
name_rm     db 'rm', 0
name_touch  db 'touch', 0
name_cp     db 'cp', 0
name_mv     db 'mv', 0
name_clear  db 'clear', 0
name_uname    db 'uname', 0
name_basename db 'basename', 0
name_dirname  db 'dirname', 0
name_head     db 'head', 0
name_wc       db 'wc', 0
name_ln       db 'ln', 0
name_readlink db 'readlink', 0
name_chmod    db 'chmod', 0
name_kill     db 'kill', 0
name_sync     db 'sync', 0
name_id       db 'id', 0
name_hostname db 'hostname', 0
name_uptime   db 'uptime', 0
name_sleep    db 'sleep', 0

symlink_option db '-s', 0
uid_text       db 'uid=', 0
gid_text       db ' gid=', 0
uptime_suffix  db ' seconds', 10, 0
slash_text     db '/', 0

dot_text        db '.', 0
space_text      db ' ', 0
newline_text    db 10, 0
clear_text      db 27, '[2J', 27, '[H', 0
operation_error db 'flatcore: operation failed', 10, 0
unknown_message db 'flatcore: unknown command; run flatcore help', 10, 0
usage_text      db 'flatcore: missing or invalid arguments', 10, 0

help_text db 'Flatcore - native Linux x86-64 userspace', 10
          db 'Usage: flatcore COMMAND [ARGUMENTS...]', 10
          db 'Commands:', 10
          db '  echo  true  false  pwd  cat  ls', 10
          db '  mkdir  rmdir  rm  touch  cp  mv', 10
          db '  clear  uname  help', 10
          db '  basename  dirname  head  wc', 10
          db '  ln  readlink  chmod  kill  sync  id', 10
          db '  hostname  uptime  sleep', 10
          db 'Commands can also be invoked through symlinks.', 10, 0

; ls implementation state and Linux stat layout.
ls_flags       rq 1
ls_name_ptr    rq 1
ls_stat        rb 144
ls_mode_text   rb 11
ls_perm_bits   dw 256,128,64,32,16,8,4,2,1
ls_perm_chars  db 'rwxrwxrwx'
ls_arg_index rq 1
ls_opt_all db '--all',0
ls_opt_almost db '--almost-all',0
ls_opt_directory db '--directory',0
ls_opt_help db '--help',0
ls_opt_version db '--version',0
ls_bad_option_prefix db 'ls: invalid option: ',0
ls_help_text db 'Usage: ls [OPTION]... [FILE]...',10
             db '  -a, --all             do not ignore entries starting with .',10
             db '  -A, --almost-all      do not list implied . and ..',10
             db '  -l                    use a long listing format',10
             db '  -d, --directory       list directories themselves',10
             db '  -1                    list one file per line',10
             db '      --help            display this help and exit',10
             db '      --version         output version information and exit',10,0
ls_version_text db 'Flatcore ls 0.1',10,0
