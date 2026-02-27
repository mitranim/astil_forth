import' ./lang.f
import' ./time.f \ Defines `Timespec`.

\ ## Libc IO
\
\ Translated from C definitions on MacOS 15.3 (24D60).
\
\ The interface is somewhat incomplete,
\ and has NOT been fully tested.
\
\ Some basic extern IO stuff is defined in `lang.f`.
\ This file defines a bunch of missing declarations
\ and some wrappers which throw descriptive errors.
\
\ Procedures which return -1 on error should usually
\ be followed by the test `int_err` or `try_errno"`:
\
\   some_word try_errno" unable to blah"

\ `struct stat`. Used by reference.
struct: Fstat
  S32      1 field: Fstat_dev           \ [XSI] ID of device containing file
  U16      1 field: Fstat_mode          \ [XSI] Mode of file (see below)
  U16      1 field: Fstat_nlink         \ [XSI] Number of hard links
  U64      1 field: Fstat_ino           \ [XSI] File serial number
  U32      1 field: Fstat_uid           \ [XSI] User ID of the file
  U32      1 field: Fstat_gid           \ [XSI] Group ID of the file
  S32      1 field: Fstat_rdev          \ [XSI] Device ID
  Timespec 1 field: Fstat_atimespec     \ time of last access
  Timespec 1 field: Fstat_mtimespec     \ time of last data modification
  Timespec 1 field: Fstat_ctimespec     \ time of last status change
  Timespec 1 field: Fstat_birthtimespec \ time of file creation(birth)
  S64      1 field: Fstat_size          \ [XSI] file size, in bytes
  S64      1 field: Fstat_blocks        \ [XSI] blocks allocated for file
  S32      1 field: Fstat_blksize       \ [XSI] optimal blocksize for I/O
  U32      1 field: Fstat_flags         \ user defined flags for file
  U32      1 field: Fstat_gen           \ file generation number
  S32      1 field: Fstat_lspare        \ RESERVED: DO NOT USE!
  S64      2 field: Fstat_qspare        \ RESERVED: DO NOT USE!
end

\ `readv` and `preadv` take an array of these.
struct: Iov
  Adr  1 field: Iov_base \ [XSI] Base address of I/O memory region
  Cell 1 field: Iov_len  \ [XSI] Size of region iov_base points to
end

\ When C _thinks_ it returns `-1`, the actual register content
\ sometimes contains zeros in upper bits, depending on how the
\ result was produced. Sometimes we may have to compare with a
\ type-specific value or sign-extend. TODO consistent solution.
-1         let: EOF
0xff       let: EOF8
0xffffffff let: EOF32

0x00000004 let: O_NONBLOCK        \ do not block on open or for data to become available
0x00000008 let: O_APPEND          \ append on each write
0x00000200 let: O_CREAT           \ create file if it does not exist
0x00000400 let: O_TRUNC           \ truncate size to 0
0x00000800 let: O_EXCL            \ error if O_CREAT and the file exists
0x00000010 let: O_SHLOCK          \ atomically obtain a shared lock
0x00000020 let: O_EXLOCK          \ atomically obtain an exclusive lock
0x00000040 let: O_DIRECTORY       \ restrict open to a directory
0x0080     let: O_SYNC            \ synch I/O file integrity
0x00000100 let: O_ASYNC           \ signal pgrp when data ready
0x00100000 let: O_NOFOLLOW        \ do not follow symlinks
0x00200000 let: O_SYMLINK         \ allow open of symlinks
0x00008000 let: O_EVTONLY         \ descriptor requested for event notifications only
0x01000000 let: O_CLOEXEC         \ mark as close-on-exec
0x20000000 let: O_NOFOLLOW_ANY    \ do not follow symlinks in the entire path
0x00001000 let: O_RESOLVE_BENEATH \ path must reside in the hierarchy beneath the starting directory

\ Exactly one of these must be `or`d into flags in `open`.
\ Note that `fopen` and `fdopen` take a STRING as mode!
0                     let: O_RDONLY  \ open for reading only
1                     let: O_WRONLY  \ open for writing only
2                     let: O_RDWR    \ open for reading and writing
0x40000000            let: O_EXEC    \ open file for execute only
O_EXEC O_DIRECTORY or let: O_SEARCH  \ open directory for search only

\ In all definitions below, `code` means `(Cint)-1` on failure;
\ should be tested with `int_err` or `try_errno`.
2 1 extern: open  ( path flag -- fdes|code )
1 1 extern: close ( fdes      -- code      )
2 1 extern: fstat ( fdes stat -- code      )
2 1 extern: lstat ( path stat -- code      )
2 1 extern: stat  ( path stat -- code      )

1 1 extern: fgetc            ( file -- char|eof32 )
1 1 extern: getc_unlocked    ( file -- char|eof32 )
1 1 extern: getchar          ( file -- char|eof32 )
1 1 extern: getchar_unlocked ( file -- char|eof32 )
1 1 extern: getw             ( file -- char|eof32 )

3 1 extern: read   ( fdes buf len         -- len )
4 1 extern: pread  ( fdes buf len off     -- len )
3 1 extern: readv  ( fdes iov iov_len     -- len )
4 1 extern: preadv ( fdes iov iov_len off -- len )

3 1 extern: write   ( fdes buf len         -- len )
4 1 extern: pwrite  ( fdes buf len off     -- len )
3 1 extern: writev  ( fdes iov iov_len     -- len )
4 1 extern: pwritev ( fdes iov iov_len off -- len )

\ Note: `fread`, `fwrite` and some other procs are defined in `lang.f`.
2 1 extern: fopen    ( path mode      -- file )
2 1 extern: fdopen   ( fdes mode      -- file )
3 1 extern: freopen  ( path mode file -- file )
3 1 extern: fmemopen ( buf cap mode   -- file )
1 1 extern: fclose   ( file           -- code )

2 1 extern: fgetpos ( file pos_adr    -- code     )
2 1 extern: fsetpos ( file pos_adr    -- code     )
3 1 extern: fseek   ( file off whence -- code     )
3 1 extern: fseeko  ( file off whence -- code     )
1 1 extern: ftell   ( file            -- off|code )
1 1 extern: ftello  ( file            -- off|code )
1 0 extern: rewind  ( file            --          )

1 0 extern: clearerr          ( file --      )
1 0 extern: clearerr_unlocked ( file --      )
1 1 extern: feof              ( file -- code )
1 1 extern: feof_unlocked     ( file -- code )
1 1 extern: ferror            ( file -- code )
1 1 extern: ferror_unlocked   ( file -- code )
1 1 extern: fileno            ( file -- code )
1 1 extern: fileno_unlocked   ( file -- code )

: io_err { action path }
  errno { code }
  code strerror { msg }
  msg if
    action path code msg throwf" unable to %s `%s`; code: %d; msg: %s"
  else
    action path code throwf" unable to %s `%s`; code: %s"
  end
;

: fd_stat { fd path stat_adr }
  fd stat_adr fstat
  if c" stat" path io_err end
;

: path_stat { path stat_adr }
  path stat_adr stat
  if c" stat" path io_err end
;

: fd_open { path mode -- fd }
  path mode open { out }
  out int_err if c" open" path io_err end
  out
;

: file_open { path mode -- file }
  path mode fopen { file }
  file ifn c" open" path io_err end
  file
;
