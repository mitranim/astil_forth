import' std:lang.f

\ See the actual slide `./2_reg_alloc.md`.
\
\ Below are just disassembly drafts.

\ Valid under stack-CC but not under reg-CC
\ because every verb must consume all args.
: word
  \ 10 20 30 + * .
  \          ^ error here: arity: 2, inputs: 3
;

: word [ redefine ]
  \ The following should fail to compile.
  \ 10       +             \ Not enough inputs.
  \ 10 20 30 +             \ Too many inputs.
  \ 10 20    + { one two } \ Not enough outputs.

  \ The following should compile OK.
  10 20 + { one }
;
\ dis' word

: word { one two three -- four } [ redefine ]
  \      x0  x1  x2       x0

  one   \ -- ignored: register already x0
  two   \ -- ignored: register already x1

  +     \ add x0, x0, x1
  three \ mov x1, x2
  *     \ mul x0, x0, x1
;
\ dis' word

: word { one two -- out } [ redefine ]
      \ mov x2, x0     -- compiled later during `;`
  two \ mov x0, x1     -- compiled immediately
      \ nop            -- tentative relocation of `two`, erased during `;`
  one \ mov x1, x2     -- compiled later during `;`
  +   \ add x0, x0, x1 -- compiled immediately
;
\ dis' word

\ When locals are not clobbered, the compiler keeps
\ them in registers and doesn't bother with memory.
: word [ redefine ]
  123 234 { one two }
;
\ dis' word
\
\ mov x0, #123
\ mov x1, #234
\ ret

\ When locals are clobbered, the compiler prefers to keep them
\ in other, unclobbered, registers if possible.
: word [ redefine ]
  123 234 { one two }
  345 456 { -- }      \ Clobbering causes relocation.
  one two { -- }      \ Ensure the locals are "used".
;
\ dis' word
\
\ mov  x0, #123
\ mov  x1, #234
\ mov  x2, x0   -- Relocate `one`.
\ mov  x0, #345
\ mov  x3, x1   -- Relocate `two`.
\ mov  x1, #456
\ mov  x0, x2
\ mov  x1, x3
\ ret
