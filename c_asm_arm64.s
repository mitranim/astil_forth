.include "./generated/c_asm.s"

/*
See the C prototype for the procedure signature.

See `./c_asm_arm64.c` for docs on special registers.

If the word "throws", the error is returned in `x0`. This register is set to
`nullptr` in C before the call. For calls of procedures which may "throw" by
returning an error pointer, the compiler inserts a conditional branch instr.
For calls of procedures which don't return an error pointer, the compiler
inserts an instruction which zeroes `x0`, since it's caller-saved.

Registers x29-x30 are FP-LR and used for frame records and returns.

Registers x19-x28 are callee-saved, which means we can consider them stable
and allocate them for special values, at the cost of having to store and load
them when switching between C and Forth.

SYNC[asm_call_forth_stack].
*/
asm_call_forth:
  stp x29, x30, [sp, -16]! // Frame record.
  mov x29, sp              // Frame record address.
  stp x27, x28, [sp, -16]!
  mov x28, x2

  // See comments in `c_mach_exc.c`.
  adrp x2, ASM_MAGIC@page
  add x2, x2, ASM_MAGIC@pageoff
  ldr x2, [x2]
  stp x26, x2, [sp, -16]!

  ldp x26, x27, [x28, INTERP_INTS_FLOOR] // SYNC[stack_floor_top].
  blr x1

asm_call_forth_epilogue:
  str x27, [x28, INTERP_INTS_TOP]
  ldp x26, x2, [sp], 16
  ldp x27, x28, [sp], 16
  ldp x29, x30, [sp], 16
  ret

/*
When the native exception handler detects a recoverable exception
and successfully unwinds the stack, it restores FP / LR / SP to
what they're supposed to had been at `asm_call_forth_epilogue`,
and sets the PC to transfer control here for trace capture.

The handler also provides an error pointer which we have to stash
to avoid clobber by the backtrace procedure.
*/
asm_call_forth_trace:
  str x0, [sp, -16]!
  bl _backtrace_capture
  ldr x0, [sp], 16
  b asm_call_forth_epilogue
  brk 666

/*
TODO find out if we can move this to `c_asm_gen.c` and share the number
between C and asm without duplication. Adding this directive to the gen
file doesn't seem to do anything; Clang ignores it.

SYNC[asm_magic].
*/
ASM_MAGIC: .8byte 0xabcdFEEDabcdFACE
