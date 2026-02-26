import' std:lang.f
import' std:pthread.f
import' std:internals.f

\ ## Posix thread usage
\
\ Basic example without cancellation etc.

: thread_spawn { fun inp -- thread }
  nil { attr }
  nil { thread }

  ref' thread attr fun inp pthread_create { code }

  code if
    code throwf" [main] unable to spawn a thread; error code: %zd" lf
  end

  thread
;

: thread_join { thread -- out }
  nil { out }

  thread ref' out pthread_join { code }

  code if
    code throwf" [main] unable to join with child thread; error code: %zd" lf
  end

  out
;

\ Note on errors and exceptions.
\
\ In callbacks passed to libc, it's good practice to use `[ true catches ]`
\ to disable implicit rethrowing and reveal exceptions as explicit values.
\ Without this, an exception would be returned implicitly as an additional
\ output and ignored by libc.
\
\ These example procedures don't actually have any exceptions.
\ Using `[ true catches ]` ensures that they will not be added
\ accidentally when modifying code in the future.

: run0 { inp -- out } [ true catches ]
  inp logf" [child0] running child thread 0 with input %zd" lf
  inp 2 *
;

: run1 { inp -- out } [ true catches ]
  inp logf" [child1] running child thread 1 with input %zd" lf
  inp 345 +
;

: main
  123         { inp  }
  instr' run0 { fun0 }
  instr' run1 { fun1 }
  nil         { thread0 }
  nil         { thread1 }

  log" [main]   spawning child thread 0..." lf

  fun0 123 thread_spawn { thread0 }

  log" [main]   spawning child thread 1..." lf

  fun1 234 thread_spawn { thread1 }

  log" [main]   waiting on child thread 0..." lf

  thread0 thread_join { out }

  out logf" [main]   joined with child thread 0; output: %zd" lf

  log" [main]   waiting on child thread 1..." lf

  thread1 thread_join { out }

  out logf" [main]   joined with child thread 1; output: %zd" lf
;
main
