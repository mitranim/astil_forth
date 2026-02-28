import' std:lang.f
import' std:io.f

\ The paths assume the script is being run from repo root:
\
\   ./astil.exe examples/io.f

\ ## Stat

: example_file_stat
  c" examples/io.f" { path }
  Fstat alloca      { stat }

  path stat path_stat

  stat Fstat_size @    { size }
  stat Fstat_mtimespec { mtime }
  mtime Timespec_sec @ { secs }

  path logf" [example0] path:        %s"  lf
  size logf" [example0] file size:   %zd" lf
  secs logf" [example0] last change: %zd" lf
;
example_file_stat

\ [example0] path:        examples/io.f
\ [example0] file size:   1348
\ [example0] last change: 1772280472

\ ## Seek

128 mem: BUF

: example_seek_and_thou_shall_find
  c" examples/io.f"    { path }
  path c" r" file_open { file }

  \ Disable exceptions from here, so we can close the file reliably.
  [ true catches ]

  37 91 { off cap }

  file off 0 fseek is_err if
    file fclose { -- }

    \ Explicit inline `throw` is perfectly fine. It's just like `ret`,
    \ but doesn't zero-out the exception register.
    c" seek file" path io_err throw
  end

  BUF U8 cap file fread { len }
  file ferror           { code }
  file fclose           { -- }

  len cap <> if
    code c" unable to read file" os_err throw
  end

  lf
  path logf" [example1] partial content of `%s`:" lf lf
  BUF puts lf
;
example_seek_and_thou_shall_find

\ [example1] partial content of `examples/io.f`:
\
\ \ The paths assume the script is being run from repo root:
\ \
\ \   ./astil.exe examples/io.f
