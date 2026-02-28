# Comparison of calling conventions

Stack-CC versus reg-CC, as implemented in this system.

## Arithmetic

Equivalent definitions:

```forth
\ Stack-CC:
: word ( -- out ) 10 20 30 + * ;

\ Reg-CC:
: word { -- out } 20 30 + 10 * ;

\ Printing disassembly:
dis' word
```

Generated code:

stack-CC                  | reg-CC
--------------------------|----------------
mov  x8, #10              | mov  x0, #20
str  x8, [x27], #8        | mov  x1, #30
mov  x8, #20              | add  x0, x0, x1
str  x8, [x27], #8        | mov  x1, #10
mov  x8, #30              | mul  x0, x0, x1
str  x8, [x27], #8        | ret
ldp  x1, x2, [x27, #-16]! |
add  x1, x1, x2           |
str  x1, [x27], #8        |
ldp  x1, x2, [x27, #-16]! |
mul  x1, x1, x2           |
str  x1, [x27], #8        |
ret                       |

Fewer instructions, zero memory ops.

Making this easy to optimize requires some restrictions on how the "stack" works. _Truly_ replicating all stack behaviors while producing good machine code is too complex. But with a few small limitations, we can make it very simple.

The resulting register allocator is trivial, and does _not_ rely on intrinsic knowledge of the language. It doesn't even need to understand branching.

## Loops

```forth
\ Stack-CC:
: example_loop
  12 0 +for: ind
    123 ind * drop
  end
;

\ Reg-CC:
: example_loop
  12 0 +for: ind
    123 ind * { -- } \ Ignores the value.
  end
;

\ Printing disassembly:
dis' example_loop
```

stack-CC                   | reg-CC
---------------------------|----------------
stp  x29, x30, [sp, #-32]! | mov  x0, #12
mov  x29, sp               | mov  x1, #0
mov  x8, #12               | mov  x8, x0
str  x8, [x27], #8         | mov  x9, x1
mov  x8, #0                | cmp  x0, x1
str  x8, [x27], #8         | b.le #32
ldr  x1, [x27, #-8]!       | mov  x0, #123
str  x1, [x29, #24]        | mov  x1, x9
ldr  x1, [x27, #-8]!       | mul  x0, x0, x1
str  x1, [x29, #16]        | mov  x0, x8
ldr  x1, [x29, #24]        | mov  x1, x9
ldr  x2, [x29, #16]        | add  x1, x1, #1
cmp  x1, x2                | b    #-36
b.ge #52                   | ret
mov  x8, #123              |
str  x8, [x27], #8         |
ldr  x8, [x29, #24]        |
str  x8, [x27], #8         |
ldp  x1, x2, [x27, #-16]!  |
mul  x1, x1, x2            |
str  x1, [x27], #8         |
sub  x27, x27, #8          |
ldr  x1, [x29, #24]        |
add  x1, x1, #1            |
str  x1, [x29, #24]        |
b    #-60                  |
ldp  x29, x30, [sp], #32   |
ret                        |

No memory ops.

## Additional notes

I have a bone to pick with Forth compiler authors.

Anton Ertl, one of the two principal authors of Gforth, had been steadily publishing articles on optimizing stack machines over the last 30-35 years or so.

The premise always seems to be:
- Start with a stack machine abstraction.
- Discover that the hardware doesn't like it.
- Do trickery to transform this into code the hardware likes.

To his credit, emphasis was made on keeping the trickery simple.

The premise _should_ be:
- Look for an abstraction which is the best match between the machine and the human mind.
  - Both seem to prefer named locations (registers and locals).
  - Any compiler writer should recognize the affinity and abandon an abstraction which no longer fits.

Probably an unpopular opinion which is going to get me expelled from here.

See the "slide" `./2_reg_alloc.md` which shows why.
