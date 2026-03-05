import' std:lang.f
import' std:io.f
import' std:pthread.f
import' std:internals.f

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
  ( E: val -- err ) \ Real ABI signature.

  " [child] running and throwing" log lf

  " mock_error" throw
;

: main
  instr' callback { instr } \ Raw instruction address.

  " [main] spawning..." log lf
  instr nil thread_spawn { thread }

  " [main] joining..." log lf
  thread thread_join { err }

  " [main] received error: %s" err logf lf
;
main
