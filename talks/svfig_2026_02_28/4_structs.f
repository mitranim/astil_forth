import' std:lang.f

\ ## Structs and C interop
\
\ The system supports structs which match the C ABI.
\ The implementation is extremely simple and done
\ entirely inside Forth.
\
\ See `forth/time.f` and `forth/io.f` in repo root for more.
\
\ Struct fields return pointers which can be loaded and stored
\ with `@` and `!`. This generates suboptimal machine code, but
\ is good enough for IO-related code.
\
\ A struct "type" is a size. A struct "field" is an offset.

\ C: `struct timespec`
struct: Timespec
  S64 1 field: Timespec_sec
  S64 1 field: Timespec_nsec
end

2 0 extern: clock_gettime

: example_time_struct
  Timespec alloca { inst }

  \ A `libc` API which takes a struct pointer.
  0 inst clock_gettime

  inst Timespec_sec  @ { secs }
  inst Timespec_nsec @ { nano }

  secs logf" real seconds: %zd" lf
  nano logf" real nanos:   %zd" lf
;
example_time_struct
