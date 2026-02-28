import' std:lang.f

\ See `./1_disasm.md`.

: example_arith
  10 20 + 30 * { -- }
;
dis' example_arith
\ mov  x0, #10
\ mov  x1, #20
\ add  x0, x0, x1
\ mov  x1, #30
\ mul  x0, x0, x1
\ ret

\ Numeric literals are compile-time constants, so the compiler
\ is able to hardcode them into `alloca` instructions.
: example_alloca_static
  Cell alloca { cell }
  128  alloca { chunk }
;
dis' example_alloca_static
\ stp  x29, x30, [sp, #-16]!
\ mov  x29, sp
\ sub  x0, sp, #16   -- allocate `cell`
\ mov  sp, x0
\ nop                -- clobber of `cell`, rewritten
\ sub  x0, sp, #128  -- allocate `chunk`
\ mov  sp, x0
\ mov  sp, x29       -- release memory
\ ldp  x29, x30, [sp], #16
\ ret

\ Compiler supports dynamic `alloca` sizes as well.
: example_alloca_dynamic { size -- }
  size alloca { -- }
;
dis' example_alloca_dynamic
\ stp  x29, x30, [sp, #-16]!
\ mov  x29, sp
\ sub  x0, sp, x0
\ and  x0, x0, #0xfffffffffffffff0
\ mov  sp, x0
\ mov  sp, x29
\ ldp  x29, x30, [sp], #16
\ ret

: example_loop
  12 0 +for: ind
    123 ind * { -- } \ Just something to do.
  end
;
dis' example_loop
\ mov  x0, #12    -- loop ceiling
\ mov  x1, #0     -- loop floor (index)
\ mov  x8, x0     -- store ceiling
\ mov  x9, x1     -- store index (loop begins here)
\ cmp  x0, x1     -- ceiling vs index
\ b.le #32        -- leave if done
\ mov  x0, #123   -- `123`
\ mov  x1, x9     -- `ind`
\ mul  x0, x0, x1 -- `*`; `{ -- }` ignores the result
\ mov  x0, x8     -- read ceiling
\ mov  x1, x9     -- read index
\ add  x1, x1, #1 -- increment index
\ b    #-36
\ ret




\ ## Notes
\
\ The word `dis'` works by invoking `llvm-mc`,
\ which must exist somewhere in the `$PATH`.
\
\ An alternative approach is to dump instructions as hex
\ and invoke a disassembler manually. The example below
\ uses `llvm-mc` as well.
\
\ Getting an instruction dump:
\
\   debug' some_word
\
\ Shell shortcut for disassembly:
\
\   dis.hex() { echo $@ | llvm-mc --disassemble --hex }
\
\ Usage example:
\
\   dis.hex 400180d2810280d20000018bc0035fd6
\
\   mov  x0, #10
\   mov  x1, #20
\   add  x0, x0, x1
\   ret
