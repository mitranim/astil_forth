# Simple register allocation with single-pass assembly

TLDR: it's possible to replace stack with registers, while assembling in one pass with fixups and keeping the compiler reasonably simple.

Key enablers:
- Every verb consumes all arguments.
- Built-in locals and relocations.
- Upfront parameter declarations.

Spoiler: the result is similar to how arguments and calls work in C. That's okay. It makes Forth more usable.

## Interpretation vs compilation

- Everything below is for _compiled_ code (inside words).
- Top-level _interpretation_ keeps inputs and outputs in the data stack.
- When _interpreting_ words, we pop from stack to registers, make the call, then push from registers to stack.

Why:

- In interpretation, control alternates between interpreter code and program code _for each argument_.
- Not possible to build up an argument list in registers while interpreting on the fly.
- Intermediary book-keeping data structure is unavoidable.
- That's the data stack.

Interpreted code is executed once.
Compiled code executes repeatedly.

## Greedy verbs

The following code is valid under stack-CC, but does _not_ compile under reg-CC due to arity mismatch. It's also misleading.

```forth
: word 10 20 30 + * . ;

: word
  10 20 30 + * .
  \        ^ error here: arity = 2, inputs = 3
  \        `+` tries to consume all 3 arguments!
;
```

Under reg-CC, it must be converted to:

```forth
: word 20 30 + 10 * . ;

: word
  20 30 + \ inputs: 2, outputs: 1
  10    * \ inputs: 2, outputs: 1
        . \ inputs: 1, outputs: 0
;
```

Making verbs "greedy" solves major headaches:
- Machine affinity: easy register allocation.
  The registers naturally become a stack.
- Inputs and outputs are obvious from the code.
  No need to remember the arity of every word.
- Programming is _much_ less error prone.

## Parameters and arity

To make this work, compiler needs to know input-output arity.
It supports parameter declarations and local assignments, like this:

```forth
: +
  { inp0 inp1 -- out0 } \ 2 inputs -- 1 output
  \ ... implementation
;
```

Compiler checks arity in calls and assignments.
Each of the following is a compile-time error:

```forth
10       +             \ Not enough inputs.
10 20 30 +             \ Too many inputs.
10 20    + { one two } \ Not enough outputs.
```

But the following is OK:

```forth
10 20 + { one } \ Correct inputs; output is consumed.
10 20 + { --  } \ Correct inputs; this is how we `drop` outputs.
```

Local names replace stack manipulation (see below).

Why not infer arity by analyzing stack-based code? Because my system self-assembles, and the compiler can't detect every push / pop.

## Register allocation word-by-word

Truly replicating stack semantics with registers is complicated. You have to delay decisions and build an IR. I had a go, and bounced off the complexity.

It turns out, input-output registers can be _naturally a stack_ if you make verbs greedy. For example, Arm64 uses `x0 x1 x2 x3 x4 x5 x6 x7` for inputs and outputs. Outputs naturally flow into inputs. The lower-numbered registers are the "bottom" of the stack.

Simplest solution:
- Every call replaces the entire "stack".
- Other values must be manually stashed to locals.
- Compiler allocates locals to temp registers or memory in a fixup pass.

This allows the compiler to place low-numbered stack items into low-numbered parameter registers right away.

Simple example:

```forth
: word 20 30 + 10 * . ;

20 \ mov x0, 20     -- length: 1
30 \ mov x1, 30     -- length: 2
+  \ add x0, x0, x1 -- length: 1; x0 is input-output
10 \ mov x1, 10     -- length: 2
*  \ mul x0, x0, x1 -- length: 1; x0 is input-output
```

With named inputs. The compiler doesn't implicitly "push" parameters, but it knows where they are (register slots):

```forth
: arith { one two three -- four }
        \ x0  x1  x2       x0
  one   \ -- register already x0
  two   \ -- register already x1
  +     \ add x0, x0, x1
  three \ mov x1, x2
  *     \ mul x0, x0, x1
;
```

We can immediately see deficiencies in the algorithm. Clang does slightly better:

```asm
_arith:
  add x8, x1, x0
  mul x0, x8, x2
  ret
```

If we reverse value order, the compiler has to shuffle the registers around. Matching input order is already second nature to Forth users:

```forth
: word { one two -- out }
      \ mov x2, x0
  two \ mov x0, x1
  one \ mov x1, x2
  +   \ add x0, x0, x1
;
```

Assigning literals to names works the same:

```forth
: word { -- out }
  10          \ mov x0, 10
  20          \ mov x1, 20
  { one two } \ Associate registers with names.
  one two     \ Do nothing at all: registers match.
  +           \ add x0, x0, x1
;
```

## Changes to Forth code

- Verbs are greedy.
- Stack is gone.
- `dup`, `swap`, `rot`, etc. are gone.
- Parameters are not implicitly pushed.
- How do we manipulate the "stack"?
- Locals: both necessary and sufficient.

The following stack-based code was BRUTAL for me. The stack state is not obvious. It's also invalid in reg-CC because there's no stack manipulation:

```forth
: asm_lsl_imm ( Xd Xn imm6 -- instr )
  dup negate 64 mod 6 bit_trunc 16 lsl    \ immr
  63 rot -          6 bit_trunc 10 lsl or \ imms
  swap                           5 lsl or \ Xn
                                       or \ Xd
  0b1_10_100110_1_000000_000000_00000_00000 or
;
```

Reg-CC doesn't support the above. The code needs to use locals. Note that this is equally valid in stack-CC:

```forth
: asm_lsl_imm { Xd Xn imm6 -- instr }
  imm6 negate 64 mod 6 bit_trunc 16 lsl { immr }
  63 imm6 -          6 bit_trunc 10 lsl { imms }
  Xn 5 lsl
  Xd or imms or immr or \ Still kinda concatenative.
  0b1_10_100110_1_000000_000000_00000_00000 or
;
```

Note that good Forth code style ALREADY uses signature declarations, and sometimes comments about the stack state! So we're not adding much code.

This partially surrenders concatenative properties of Forth. When forwarding to another word, sometimes you have to repeat all arguments, bloating the code:

```forth
\ Stack-based:
: compare ( str0 len0 str1 len1 -- direction ) rot min strncmp ;
: str=    ( str0 len0 str1 len1 -- bool      ) compare =0 ;
: str<    ( str0 len0 str1 len1 -- bool      ) compare <0 ;
: str<>   ( str0 len0 str1 len1 -- bool      ) compare <>0 ;



\ Stack-based without comments:
: compare rot min strncmp ;
: str=    compare =0 ;
: str<    compare <0 ;
: str<>   compare <>0 ;



\ Locals-based. Can't be made shorter.
\ This is why the compiler doesn't implicitly "push"
\ parameters to the "stack": so you CAN reorder them.
: compare { str0 len0 str1 len1 -- direction }
  len0 len1 min { len } \ perform "rot"
  str0 str1 len strncmp
;
: str=  { str0 len0 str1 len1 -- bool } str0 len0 str1 len1 compare =0 ;
: str<  { str0 len0 str1 len1 -- bool } str0 len0 str1 len1 compare <0 ;
: str<> { str0 len0 str1 len1 -- bool } str0 len0 str1 len1 compare <>0 ;
```

Concatenation is not entirely lost. You still write forward code. Outputs still flow into inputs:

```forth
10 20 + 30 *

10 20 + { val }
val 30 +
```

For me, the tradeoff is well worth it. Forward-execution is preserved, while error-prone stack management is eliminated.

## Control flow analysis (how to not)

Control structures simply clobber the entire "stack" (parameter registers). This was sufficient for avoiding data flow analysis. The "stack" is only `x0 x1 x2 x3 x4 x5 x6 x7`, and the rest can be used for locals. Simplicity is nice:

```forth
: word { cond }    \ Assigned to x2: non-param location.
  123      { val } \ Assigned to x0: param location.
  val cond { -- }
;
\ mov x2, x0
\ mov x0, #123
\ mov x1, x2
\ ret



: word { cond } \ Assigned to x8: non-param location.
  123 { val }   \ Assigned to x9: non-param location.

  \ `if` and `end` clobber param registers and relocate `val`.
  cond if 234 { val } end

  val cond { -- } \ x0, x1 <- x9 x8
;
\ mov  x8, x0
\ mov  x0, #123
\ mov  x9, x0
\ mov  x0, x8
\ cbz  x0, #12
\ mov  x0, #234
\ mov  x9, x0
\ mov  x0, x9
\ mov  x1, x8
\ ret
```

- Control structures clobber the "stack" using `comp_barrier`.
- Usage examples: `elif_init loop leave until` and more.
- Simple hack to avoid branch analysis.
- Branching is implemented in Forth, compiler doesn't _know_ about it.

## Brief dive into compiler code

Relevant C stuff:
- `Comp_ctx`: compilation context: how much book-keeping does this need?
  (Very little!)
- `comp_append_push_imm`: "pushing" numbers.
- `comp_append_local_get_next`: "pushing" from locals.
- `comp_after_append_call`: how verbs replace the "stack".
- `asm_resolve_local_location`: how we decide where to place locals.
  - Locals go into registers when possible, but sometimes we have to evict them.

## Side benefits

Voicing an unpopular opinion: stack-based programming is too hard. Names are better.

We replace error-prone stack manipulation with clear order, clear arities, making inputs obvious from the code. This makes it easier to program. There's an affinity between what the machine wants and what the programmer wants.

This is better, and gets us a faster language.

## Optimization tricks

For all my talk about simplicity, the system uses a fair amount of _tricks_ to reduce unnecessary instructions, while still being able to assemble forward:

- Reserve prologue space, skip it later.
- Delayed fixups.
- Lazy local relocations.
- Skip early param relocations.
- Basic inlining.
- Sometimes rewrite stores with nops.
- ...And various other minor things.

See the associated "slide" `./6_tricks.md`.
