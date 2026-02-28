# Simple optimization tricks

## Difficulties

The focus on direct single-pass assembly causes difficulties which require workarounds:

- Prologue is undecidable until later.
  - Prologue and epilogue needs to be resizable.
- Many other instructions are undecidable until later.
  - `try throw ret` and so on.
  - Reads and writes of locals (some registers are unknown).
  - Loads of large immediates stored after the function body.

The general approach is to reserve instructions, remember what needs fixing, and rewrite them in a fixup pass.

In every case, I prioritized solutions easy to understand and implement.
Every trick is simple.

## Prologue / epilogue resizing

We want to generate code immediately, but we only find out LATER if need a prologue, and how much stack memory to allocate:

```forth
\ Leaf procedures should NOT stash-restore LR/FP.
\ You can often avoid dealing with memory at all.
: example_leaf_no_prologue_epilogue
  10 20 + { -- }
;
dis' example_leaf_no_prologue_epilogue
\ mov  x0, #10
\ mov  x1, #20
\ add  x0, x0, x1
\ ret

\ Locals evicted to memory use FP-relative addressing.
\ They need FP adjustment, creating a frame record:
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
```

Simple solution:
- Reserve the full prologue size and _skip it_.
- When finalizing a procedure:
  - Figure out the prologue we actually need, and retropatch it.
  - Ignore the tiny amount of wasted space.
  - Sometimes skip additional instructions after the prologue. (Local relocations.)

## Delayed fixups

Some operations are undecidable until reaching the end of the current word:

- Do we need to restore SP/FP/LR?
- How much stack memory to release?
- How to jump to the epilogue, what is the offset?
- Reads / writes of register-based locals; see below.

As a result, `ret try throw`, and some other operations, are compiled in two stages:

- Reserve an instruction and store a "fixup" record for later.
- When finalizing the word, iterate the fixups and patch the instructions.

## Locals: lazy assignment and relocation

It's common to assign values to locals for later use, or for documentation, and immediately forward them into the next call. This needs to be zero-cost when locals are not used later. This affects parameters most; they get their own special trick.

Assigned values are often _immediately_ clobbered by subsequent arguments for the next call. This involves a lot of tentative relocations.

Simple solution: local assignments and relocations are _lazy_:

- Compiler temporarily associates locals with param registers.
- Reading a local into one of its associated registers is free.
- When all param associations are clobbered, the last available one is copied to the local's _stable_ location.
  - Compiler lazily emits a "write" operation;`mov` or `str`.
  - Operation type is decided later.
  - Sometimes it becomes a `nop`.

Relevant stuff in compiler code:

- `Comp_ctx`
- `Loc_fixup`
- `Loc_read`
- `Loc_write`
- `asm_fixup_locals`
- `comp_append_local_set_next`

Here, the locals use no instructions, because every register matches and is not clobbered by anything else:

```forth
: example_zero_cost_locals
  10 20
  { one two }    \ free
  one two { -- } \ free
;
dis' example_zero_cost_locals
\ mov  x0, #10
\ mov  x1, #20
\ ret
```

Clobber + reorder causes the compiler to lazily emit relocation instructions. The `nop` is a tentative relocation of `two` due to clobber; it's rewritten with a `nop` because `two` is not used later.

```forth
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
```

The compiler doesn't use callee-saved registers yet. When all caller-saved registers are clobbered, locals are evicted to memory. All lazily emitted "writes" are confirmed, and later rewritten with either `str` or `nop` depending on usage:

```forth
1 0 extern: exit

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
```

Unused relocations waste performance, especially if they're memory ops. Simple solution: replace unused ops with nops. Dirty but simple to do. Nops are still faster than stores.

```forth
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
```

Tiny trick: sometimes an evicted local is guaranteed to have a copy of the most recent value in a register. This is because store-words like `!` would consume all arguments, so you can't use them inside an argument list for another word:

```forth
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
\ ldr  x1, [x29, #16] -- load `one`
\ mov  x2, x1         -- copy `one`; faster than load
\ ldp  x29, x30, [sp], #32
\ ret
```

## Elision of parameter relocations

- Params are usually immediately clobbered.
- Params may or may not be reused later.
- Compiler has to emit tentative relocations.
- Don't want `nop` instructions bloating the code.

Simple solution:

- When finalizing a word:
  - Detect unused relocations immediately after the prologue.
  - Skip them by moving the prologue.
- Works for all clobbers of unused params.
- Doesn't cover other locals.

Example code:

```forth
: example_unused_param_reloc { one two -- three four }
  one two + 123
  \       ^ clobbers `one`
  \         ^ clobbers `two`
;
```

_Without_ the trick, this is what the word would have looked like:

```asm
nop // unused relocation of `one`
nop // unused relocation of `two`
add x0, x0, x1
mov x1, #123
ret
```

The trick skips the nops:

```asm
add x0, x0, x1
mov x1, #123
ret
```

## Scratch registers, lifetimes, and liveness

- "Proper" compilers perform liveness analysis to find lifetimes of locals.
- This allows to "free" registers and reuse them for other locals.
- Requires analyzing data flow over branches and loops.

Our compiler _can't_ do this, because conditionals and loops are implemented in application code. The compiler doesn't know about them.

Simple solutions:

- Each local is live from prologue to epilogue.
- Each local gets its own scratch register. (Or FP offset.)
- Some operations reuse a param reg, or grab a spare param reg for an "atomic" operation without affecting locals.
- Association of locals and param regs is temporary.
- "Stable" local locations use non-param regs.
- Control structures clobber all _param_ regs.
- Control structures don't clobber "stable" local regs.

This lets us skip control flow analysis while keeping many locals in registers.

This relies on the following:

- The architecture has many spare registers (as most modern arches).
- Words tend to be short, with few locals.
- Compiler knows about callee clobbers.
- Many callees don't clobber more than they use.

Callers can use remaining volatile registers:

```forth
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
\ mov  x0, x10 -- `val`
\ ret
```

Branching simply relocates locals from param to the "stable" non-parameter scratch registers. On Arm64 this is `x8 … x15`:

```forth
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
```

(We could also use callee-saved registers `x19 … x27`; `x28` is interpreter pointer; slightly more complex and hasn't been needed yet. In particular, it would bloat the reserved prologue size.)

## What is inlinable

- A lot of words are very small; there's good value in inlining them.
- A "proper" compiler inlines the IR, runs contextual optimizations, and re-assembles.
- We assemble on the fly and don't have a relocatable IR.

Simple solution:
- Detect non-relocatable code.
- Copy-paste relocatable code.

What makes code non-relocatable? PC-relative addressing:
- Calling non-inlined words (`bl <pc_off>`).
- Loading values from data / GOT sections.

A word can also have too many instructions to be worth copy-pasting. I picked a random threshold of 4 and haven't measured.

This lets us easily copy-paste the remaining words. Simple arithmetic, loads, stores, and other tiny things. Little complexity, big value: ✕2-✕3 runtime decrease in some of our microbenchmarks.

```forth
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
```

```forth
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
```

## Dealing with C limitations

Under reg-CC, this Forth uses the native call ABI specified by Arm64. Multiple inputs and outputs are passed and returned in general-purpose registers. But there's a small issue with interpreter intrinsics implemented in C:

- Interpreter has some intrinsics with 3 outputs, namely `parse`.
- Since intrinsic _inputs_ are passed natively, we're love intrinsic _outputs_ to be returned natively. In case of `parse`, the signature would be:
  - `delim interp -- buf len err`
  - In Forth, `interp` and `err` are implicit. The compiler handles this.
- C officially supports only 1 GRP output, or 2 using hacks.

Simple workaround: a custom calling convention.
- Caller allocates outputs on the stack and passes pointers.
- Intrinsics write outputs to those pointers.

This CC is used only when calling interpreter intrinsics from Forth. The compiler emits the adapter code. From Forth perspective, calls look "native".
