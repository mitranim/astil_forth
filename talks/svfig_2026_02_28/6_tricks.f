import' std:lang.f

\ See the actual "slide" `./n_tricks.md`.
\
\ Disasm of examples showcasing very simple optimization
\ tricks compatible with single-pass assembly.

\ ## Prologue / epilogue

\ Leaf procedures should NOT stash-restore LR/FP.
\ You can often avoid dealing with memory at all.
\ But we only find this out at the END of the word.
: example_leaf_no_prologue_epilogue
  10 20 + { -- }
;
dis' example_leaf_no_prologue_epilogue
\ mov  x0, #10
\ mov  x1, #20
\ add  x0, x0, x1
\ ret

: example_stack_memory_prologue_epilogue
  123 { one }
  ref' one { -- }
;
dis' example_stack_memory_prologue_epilogue
\ stp  x29, x30, [sp, #-32]!
\ mov  x29, sp
\ mov  x0, #123
\ str  x0, [x29, #16]
\ add  x0, x29, #16
\ ldp  x29, x30, [sp], #32
\ ret

\ Non-leaf procedures must stash and restore LR (link register) (and FP).
: example_non_leaf_prologue_epilogue
  123 .
;
dis' example_non_leaf_prologue_epilogue
\ stp  x29, x30, [sp, #-16]!
\ mov  x29, sp
\ mov  x0, #123
\ bl   #-3032
\ ldp  x29, x30, [sp], #16
\ ret

\ ## Locals: assignments and relocation

: example_zero_cost_locals
  10 20 { one two }
  one two { -- }
;
dis' example_zero_cost_locals
\ mov  x0, #10
\ mov  x1, #20
\ ret

\ Extra instructions are relocations due to clobber + reorder.
: example_low_cost_locals
  10 20 { one two }
  two one { -- }
;
dis' example_low_cost_locals
\ mov  x0, #10
\ mov  x1, #20
\ mov  x2, x0  -- relocate `one` due to clobber
\ mov  x0, x1  -- `two` to first param
\ nop          -- unused relocation of `two` due to clobber
\ mov  x1, x2  -- `one` to second param
\ ret

1 0 extern: exit

\ (The compiler doesn't use callee-saved registers yet.)
: example_relocation_to_memory
  10 20
  { one }  \ str x0, [x29, #16] -- decided later
  { two }  \ str x1, [x29, #24] -- decided later
  666 exit \ Clobbers scratch registers.
  one two { -- }
;
dis' example_relocation_to_memory
\ stp  x29, x30, [sp, #-32]!
\ mov  x29, sp
\ mov  x0, #10
\ mov  x1, #20
\ str  x0, [x29, #16] -- relocate `one`
\ mov  x0, #666
\ str  x1, [x29, #24] -- relocate `two`
\ adrp x8, #4464640
\ ldr  x8, [x8, #848] -- load `exit
\ blr  x8             -- call `exit`
\ ldr  x0, [x29, #16]
\ ldr  x1, [x29, #24]
\ ldp  x29, x30, [sp], #32
\ ret

: example_skipping_memory_relocation
  10 20
  { one }  \ `nop` -- local is unused later
  { two }  \ `nop` -- local is unused later
  666 exit \ Clobbers scratch registers.
;
dis' example_skipping_memory_relocation
\ stp  x29, x30, [sp, #-16]!
\ mov  x29, sp
\ mov  x0, #10
\ mov  x1, #20
\ nop                 -- didn't need `one`
\ mov  x0, #666
\ nop                 -- didn't need `two`
\ adrp  x8, #4464640
\ ldr  x8, [x8, #848] -- load `exit
\ blr  x8             -- call `exit`
\ ldp  x29, x30, [sp], #16
\ ret

\ Tiny trick: sometimes an evicted local is guaranteed
\ to have a copy of its value in a register.
: example_evicted_local_temp_reg [ redefine ]
  123 { one }
  ref' one \ add x0, x29, 16   -- evicts `one` to memory.
  one      \ ldr x1, [x29, 16] -- stable location in memory.
  one      \ mov x2, x1        -- temporary association with prior reg.
  { -- }
;
dis' example_evicted_local_temp_reg
\ stp  x29, x30, [sp, #-32]!
\ mov  x29, sp
\ mov  x0, #123
\ str  x0, [x29, #16]
\ add  x0, x29, #16
\ ldr  x1, [x29, #16]
\ mov  x2, x1
\ ldp  x29, x30, [sp], #32
\ ret

\ ## Elision of param relocations

: example_unused_param_reloc { one two -- three four }
  one two + 123
;
dis' example_unused_param_reloc
\
\ Without the trick:
\
\ nop
\ nop
\ add  x0, x0, x1
\ mov  x1, #123
\ ret
\
\ With the trick:
\
\ add  x0, x0, x1
\ mov  x1, #123
\ ret

\ ## Lifetimes and liveness

: example_callee_with_clobbers [
  0 comp_clobber \ x0
  1 comp_clobber \ x1
  2 comp_clobber \ x2
  3 comp_clobber \ x3
  4 comp_clobber \ x4
  5 comp_clobber \ x5
  6 comp_clobber \ x6
  7 comp_clobber \ x7
  8 comp_clobber \ x8
  9 comp_clobber \ x9
] ;

: example_caller_with_locals
  10 { val }
  example_callee_with_clobbers \ mov x10, x0
  val { -- }
;
dis' example_caller_with_locals
\ mov  x0, #10
\ mov  x10, x0 -- not clobbered by callee
\ mov  x0, x10
\ ret

: example_reloc_due_to_branching
  10 { val }     \ mov x0, #10
  loop leave end \ mov x8, x0
  val { -- }     \ mov x0, x8
;
dis' example_reloc_due_to_branching
\ mov  x0, #10
\ mov  x8, x0  -- { val }
\ b    #8      -- loop
\ b    #-4     -- leave
\ mov  x0, x8  -- val
\ ret

\ ## What is inlinable

: example_inlinable { one two three -- four } one two + three * ;
dis' example_inlinable
\ add  x0, x0, x1
\ mov  x1, x2
\ mul  x0, x0, x1
\ ret

: example_caller_of_inlinable
  10 20 30 example_inlinable { -- }
;
dis' example_caller_of_inlinable
\ mov  x0, #10
\ mov  x1, #20
\ mov  x2, #30
\ add  x0, x0, x1 -- inlined
\ mov  x1, x2     -- inlined
\ mul  x0, x0, x1 -- inlined
\ ret

: example_kinda_inlinable { val -- val }
  val      { one }
  ref' one @
;
dis' example_kinda_inlinable
\ stp  x29, x30, [sp, #-32]!
\ mov  x29, sp
\ str  x0,  [x29, #16]     -- stack-allocated local value
\ add  x0,  x29, #16       -- local address
\ ldur x0,  [x0]           -- load and return value
\ ldp  x29, x30, [sp], #32
\ ret

: example_caller { -- val }
  123
  example_kinda_inlinable
  inc
;
dis' example_caller
\ The callee splurges its frame record
\ into the caller. Probably harmless.
\
\ mov  x0, #123              -- outer code
\ stp  x29, x30, [sp, #-32]! -- callee begin
\ mov  x29, sp
\ str  x0, [x29, #16]
\ add  x0, x29, #16
\ ldur x0, [x0]
\ ldp  x29, x30, [sp], #32   -- callee end
\ add  x0, x0, #1            -- outer code
\ ret

\ Exceeds our inlining threshold: too many instructions.
: example_not_inlinable 10 20 30 40 50 60 70 80 { -- } ;
dis' example_not_inlinable
\ mov  x0, #10
\ mov  x1, #20
\ mov  x2, #30
\ mov  x3, #40
\ mov  x4, #50
\ mov  x5, #60
\ mov  x6, #70
\ mov  x7, #80
\ ret

: example_caller_of_not_inlinable example_not_inlinable ;
dis' example_caller_of_not_inlinable
\ stp  x29, x30, [sp, #-16]!
\ mov  x29, sp
\ bl   #-48
\ ldp  x29, x30, [sp], #16
\ ret
