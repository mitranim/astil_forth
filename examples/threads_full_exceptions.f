import' std:lang.f
import' std:io.f
import' std:pthread.f
import' std:internals.f

\ ## Posix thread usage
\
\ Using the C thread APIs from Forth, natively.
\
\ This version of the example uses Forth exceptions to full extent
\ while maintaining full ABI compatibility with libc.
\
\ See adjacent example files for different error handling.

: thread_spawn { fun inp -- thread }
  nil { attr }
  nil { thread }

  ref' thread attr fun inp pthread_create
  try_errno_posix" unable to spawn thread"
  thread
;

: thread_join { thread -- out }
  nil { out }
  thread ref' out pthread_join
  try_errno_posix" unable to join with thread"
  out
;

\ Passed to `pthread_spawn` by raw instruction address.
\ There are no hidden adapters between Forth and libc.
\
\ This word may throw. If it does, the exception is implicitly returned
\ in `x0` which is understood by `pthread_spawn` to hold thread output.
\ The error value is returned to `main` when joining with this thread.
\
\ Our "exceptions" DON'T do any of:
\ - Unwinding the stack.
\ - Randomly clobbering callee-saved registers.
\ - Otherwise interfering with the caller in any way.
: callback { inout -- }
  inout @ { path }
  path logf" [child] checking size of `%s`..." lf

  Fstat alloca { stat }
  path stat path_stat \ Throws if file is missing.

  \ Store the output back.
  stat Fstat_size @ inout !
;

: main
  instr' callback  { instr } \ Raw instruction address.
  c" missing/none" { path0 } \ Input for `thread0`: missing file.
  c" forth/lang.f" { path1 } \ Input for `thread1`: existing file.
  path0            { val0  } \ Input-output for `thread0`.
  path1            { val1  } \ Input-output for `thread1`.

  log" [main] spawning thread 0..." lf
  instr ref' val0 thread_spawn { thread0 }

  log" [main] spawning thread 1..." lf
  instr ref' val1 thread_spawn { thread1 }

  log" [main] waiting on thread 0..." lf
  thread0 thread_join try
  path0 val0 logf" [main] result from thread 0: size of `%s`: %zd" lf

  log" [main] waiting on thread 1..." lf
  thread1 thread_join try
  path1 val1 logf" [main] result from thread 1: size of `%s`: %zd" lf
;
main
