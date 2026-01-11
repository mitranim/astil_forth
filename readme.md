Work in progress.

## Overview

Prototype for a self-assembling Forth system. Currently supports only Arm64.

Goals:
- [x] Simple JIT compilation.
- [x] JIT _self_-compilation in user/lib code.
- [x] Language features can be implemented in user/lib code.
- [ ] Enough language features (in user/lib code) to be usable for scripting.
- [ ] AOT compilation.
- [ ] Rewrite in Forth and simplify.

Most languages ship with a smart, powerful, all-knowing compiler with intrinsic knowledge of the entire language. I feel this approach is too inflexible. It makes the compiler a closed system. Modifying and improving the language requires changing the compiler's source code.

On top of that, most languages isolate you from the CPU, or even from the OS, from the get-go. Instead of giving you access to the foundations, and providing a convenient pre-built ramp to the high clouds, _all you get_ is high clouds.

This system explores the opposite direction. The interpreter/compiler provides only the bare minimum of intrinsics for controlling its behavior, and leaves it to the program to define the rest of the language.

This is possible because of direct access to compilation. Outside the Forth world, this is nearly unheard of. In the Forth world, this is common and extremely powerful. For an elegant and enlightening read, look at [Frugal Forth](https://github.com/hoytech/frugal), which implements if/then/else conditionals in [3 lines](https://github.com/hoytech/frugal/blob/b3bed9bd85f0f2a23a7f334e0af8dc0392f8c796/init.fs#L99-L101) of user/lib code, with very little compiler support.

Unlike the system linked above, and mature systems such as Gforth, our implementation goes straight for machine code. It does not have a VM, bytecode of any kind, or even an IR.

Basic language features are implemented in `f_init.f`, "bootstrapping" core words via inline assembly.

## Usage

```sh
make
./forth.exe f_init.f -            # REPL mode.
./forth.exe f_init.f f_demo.f     # One-shot run.
```

Rebuild continuously while hacking:

```sh
make build_w
```

Unix users may scoff at `*.exe`, but it lets you easily hide executables in code editors 😛.

## Tradeoffs

This system uses naive single-pass assembly, with a tiny post-pass to fix PC offsets.

Single-pass assembly is incompatible with optimization. It requires every operation to be self-contained, bloating instruction count. For example, simple addition, even if inlined, becomes 6 instructions instead of 1, and most of them are memory ops to boot:

```forth
push push pop2 add push pop
```

Optimizing Forth compilers, such as VfxForth and Gforth, allocate registers and fictionalize many stack operations. Inevitably, this makes the compiler complex. It's not possible to allocate registers in the first pass through source text. You end up with an IR and more passes. This interferes with self-assembly; simply vomiting instructions into the procedure body no longer suffices, as the program must negotiate register slots with the compiler.

I had a go, and bounced off the complexity. How to keep an optimizing compiler simple?

Such tradeoffs become irrelevant when your CPU/ISA is designed for stack languages. Notable example: [GreenArrays chips](https://www.greenarraychips.com) designed (or co-designed) by Forth's inventor Chuck Moore. Looking at this architecture is enlightening in how much _simpler_ an ISA, efficient compilation, and the hardware/software stack can be.

## What is compilation

"Compilation" and especially "JIT compilation" are muddy terms. Many interpreters convert text to bytecode, which qualifies as JIT compilation, even if the bytecode remains interpreted. In the Java world, generating bytecode files is considered "compilation".

Especially murky in the Forth world. Docs will often mention compilation, and sometimes decompilation, usually without saying what it compiles _to_ and decompiles _from_: VM code or machine code. Some Forth systems use both, some stop at VM code, and ours doesn't even have VM code. Yet all these systems qualify as "compilers".

Sometimes it's useful to describe a non-assembler as a "JIT compiler". For example, one of my Go libraries implements JIT-construction of data structures which, when subsequently interpreted, allow efficient deep traversal of complex Go data structures; it delegates setup to "compilation time" and eliminates many costs at "runtime", although both steps occur when the program runs, and the result is interpreted.

Perhaps we should differentiate "JIT compilers" and "JIT assemblers". But at the end of the day, everything is interpreted.

## Non-standard

For the sake of my sanity, this does _not_ follow the ANS Forth standard.

No return stack; see below.

Words are case-sensitive.

Numeric literals are unambiguous: anything that begins with `[+-]?\d` must be a valid number followed by whitespace or EOF.

Non-decimal numeric literals are denoted with `0b 0o 0x`. Radix prefixes are case-sensitive. Numbers may contain cosmetic `_` separators. There is no `hex` mode. There is no support for `& # % $` prefixes.

A bunch of unclear words are replaced with clear ones.

Special _semantic_ roles get special _syntactic_ roles. Other Forths already follow some rules, which we replicate:
- String-parsing words end with `"` and terminate with `"`.
- String-parsing words end with `(` and terminate with `)`.

...but not enough. There are more semantic roles which need syntactic distinction. Rules we add:
- Word-parsing words which declare → end with `:`.
- Word-parsing words which don't declare → end with `'`.
- Other immediate words begin with `#`.
- Any of the above _automatically_ makes a word immediate.

Code editors are encouraged to syntax-highlight these prefixes and suffixes to clarify the semantic roles syntactically. Exceptions were made for `\ ( ; [` which are also immediate; special syntax highlighting is recommended for those.

Related: special syntax highlighting is recommended for `( ) [ ] { }` in word names, which are commonly used as delimiters with case-by-case functionality.

Word-modifying words like `immediate` and `comp_only` are used immediately inside colon definitions.

By convention, the suffix `:` denotes parsing words which declare other words. Examples include `:`, `let:`, `var:`, `to:`. Code editors are encouraged to syntax-highlight the next word like a declaration.

By convention, the names of parsing words which _don't_ declare words end with `'`. Code editors are encouraged to syntax-highlight the next word like a string.

There is no `state`, `does>`, or any form of state-smartness. (Anton Ertl would probably [approve](https://www.complang.tuwien.ac.at/papers/ertl01state.pdf).) Many words come in separate "execution time" and "compile time" variants, such as `.` vs `#.`.

Exceptions are strings (error messages) rather than numeric codes.

Booleans are `0 1` rather than `0 -1`.

The REPL doesn't print "ok".

Aside from `.`, the other printing words don't implicitly add spaces or newlines.

### No return stack

There is no return stack; procedures return natively.

Operations which need to manipulate the return address can still do that, by assembling the right instructions.

Operations which would normally use the return stack for scratch space can just use registers, locals, or globals.

## Misc

The system should be embeddable in another C/C++ program. Most of the C files are basically libraries; the "main" file is a tiny adapter using the top-level library API. See `./c_main.c`.

Furthermore, the system uses a "unity build" where files directly include files (with `#pragma once`). A program which uses this library could simply `#include` the files in its own "main" or whatever, and be good to go, without having to configure the build system.
