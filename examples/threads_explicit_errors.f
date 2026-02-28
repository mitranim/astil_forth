import' std:lang.f
import' std:io.f
import' std:pthread.f
import' std:internals.f

\ ## Posix thread usage
\
\ Using the C thread APIs from Forth, natively.
\
\ This version of the example takes advantage of opting out of exceptions
\ where it makes sense: in callbacks passed to `libc` and in `main` which
\ contains many non-fatal error conditions. Annotation `[ true catches ]`
\ automatically catches all exceptions and guarantees local control flow.
\
\ See adjacent example files for different approaches to error handling.

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
: callback { inout -- err }

  \ No implicit exceptions here.
  [ true catches ]

  inout @ { path }
  path logf" [child] checking size of `%s`..." lf

  Fstat alloca        { stat }
  path stat path_stat { err } \ Automatic catch; zero-cost.

  err ifn
    \ Store the output back.
    stat Fstat_size @ inout !
  end

  \ `main` will check this.
  err
;

: main
  \ No implicit exceptions here either.
  [ true catches ]

  instr' callback  { instr } \ Raw instruction address.
  c" missing/none" { path0 } \ Input for `thread0`: missing file.
  c" forth/lang.f" { path1 } \ Input for `thread1`: existing file.
  path0            { val0  } \ Input-output for `thread0`.
  path1            { val1  } \ Input-output for `thread1`.

  log" [main] spawning thread 0..." lf
  instr ref' val0 thread_spawn { thread0 err0 }

  log" [main] spawning thread 1..." lf
  instr ref' val1 thread_spawn { thread1 err1 }

  err0 if
    err0 logf" [main] when spawning thread 0: `%s`"
  end

  err1 if
    err1 logf" [main] when spawning thread 1: `%s`"
  end

  err0 ifn
    log" [main] waiting on thread 0..." lf
    thread0 thread_join { err0 join_err }

    join_err if
      join_err logf" [main] when joining with thread 0: %s" lf
    else err0 elif
      err0 logf" [main] error from thread 0: %s" lf
    else
      path0 val0 logf" [main] result from thread 0: size of `%s`: %zd" lf
    end
  end

  err1 ifn
    log" [main] waiting on thread 1..." lf
    thread1 thread_join { err1 join_err }

    join_err if
      join_err logf" [main] when joining with thread 1: %s" lf
    else err1 elif
      err1 logf" [main] error from thread 1: %s" lf
    else
      path1 val1 logf" [main] result from thread 1: size of `%s`: %zd" lf
    end
  end
;
main
