import' std:lang.f
import' std:io.f
import' std:pthread.f
import' std:internals.f

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
\ When this word throws, it returns the exception in the first output
\ register `x0` (because it has no other outputs). Libc takes exactly
\ this one output and returns it to `main` on `thread_join`.
\
\ Our "exceptions" DON'T do any of:
\ - Unwinding the stack.
\ - Randomly clobbering callee-saved registers.
\ - Otherwise interfering with the caller in any way.
: callback { ignored -- }
  ( E: -- err ) \ Real ABI signature.

  logf" [child] running and throwing" lf

  throw" mock_error"
;

: main
  instr' callback { instr } \ Raw instruction address.

  log" [main] spawning..." lf
  instr nil thread_spawn { thread }

  log" [main] joining..." lf
  thread thread_join { err }

  err logf" [main] received error: %s" lf
;
main
