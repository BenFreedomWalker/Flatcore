segment readable writeable

argc         rq 1
argv         rq 1
io_buffer    rb 4096
cwd_buffer   rb 4096
uname_buffer rb 390
num_buffer   rb 32

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

symlink_option db '-s', 0
uid_text       db 'uid=', 0
gid_text       db ' gid=', 0
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
          db 'Commands can also be invoked through symlinks.', 10, 0
