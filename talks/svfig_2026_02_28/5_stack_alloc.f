import' std:lang.f
import' std:testing.f

\ See `./5_stack_alloc.md` which is the actual slide.

\ ## Local references

: example_local_ref_0
  123      \ mov x0, #123
  { one }  \ str x0, [x29, #16] -- evicted to memory by `ref'`.
  ref' one \ add x0, x29, #16   -- evicts local to memory.
  one      \ ldr x1, [x29, #16] -- load from stable location.
  one      \ mov x2, x1         -- read from prior register (temporary).
  { -- }
;
dis' example_local_ref_0
\ stp  x29, x30, [sp, #-32]!
\ mov  x29, sp
\ mov  x0, #123
\ str  x0, [x29, #16]
\ add  x0, x29, #16
\ ldr  x1, [x29, #16]
\ mov  x2, x1
\ ldp  x29, x30, [sp], #32
\ ret

: example_local_ref_1
  123      \ mov  x0, #123
  { val }  \ str  x0, [x29, #16] -- evicted to memory by `ref'`.
  234      \ mov  x0, #234
  ref' val \ add  x1, x29, #16
  !        \ stur x0, [x1]
;
dis' example_local_ref_1
\ stp  x29, x30, [sp, #-32]!
\ mov  x29, sp
\ mov  x0, #123
\ str  x0, [x29, #16]
\ mov  x0, #234
\ add  x1, x29, #16
\ stur x0, [x1]
\ ldp  x29, x30, [sp], #32
\ ret

\ ## `alloca`

: example_alloca
  32 alloca { one } \ sub x0, sp, #32 ; mov sp, x0
  64 alloca { two } \ sub x0, sp, #64 ; mov sp, x0
  one two { -- }
;
dis' example_alloca
\ stp  x29, x30, [sp, #-16]!
\ mov  x29, sp
\ sub  x0, sp, #32 -- reserve memory for `one`
\ mov  sp, x0
\ mov  x2, x0      -- relocate `one` due to clobber
\ sub  x0, sp, #64 -- reserve memory for `two`
\ mov  sp, x0
\ mov  x3, x0      -- relocate `two` due to clobber
\ mov  x0, x2      -- use `one`
\ mov  x1, x3      -- use `two`
\ mov  sp, x29
\ ldp  x29, x30, [sp], #16
\ ret

\ ## Usage with `libc`
\
\ The main use of `alloca` is for stack-allocated structs.

2 0 extern: clock_gettime

\ C: `struct timespec`
struct: Timespec
  S64 1 field: Timespec_sec
  S64 1 field: Timespec_nsec
end

: example_alloca_time
  Timespec alloca { inst }
  0 inst clock_gettime

  inst Timespec_sec  @ { secs }
  inst Timespec_nsec @ { nano }

  secs logf" real seconds: %zd" lf
  nano logf" real nanos:   %zd" lf
;
\ example_alloca_time
