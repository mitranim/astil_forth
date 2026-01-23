## Overview

A native code Forth system designed for self-bootstrapping. Currently supports only Arm64.

Goals:
- [x] Simple JIT compilation to native code.
- [x] Easy _self-assembly_ in user/lib code.
- [x] Language features can be implemented in user/lib code.
- [x] Enough language features (in user/lib code) to be usable for scripting.
- [x] Avoid a VM or complex IR.
- [x] Keep the code clear and educational for other compiler amateurs.
- [ ] AOT compilation.

Most languages ship with a smart, powerful, all-knowing compiler with intrinsic knowledge of the entire language. I feel this approach is too inflexible. It makes the compiler a closed system. Modifying and improving the language requires changing the compiler's source code, which is usually over-complicated.

On top of that, most languages isolate you from the CPU, or even from the OS, from the get-go. Instead of giving you access to the foundations, and providing a convenient pre-built ramp to the high clouds, _all you get_ is high clouds.

This system explores the opposite direction. The interpreter/compiler provides only the bare minimum of intrinsics for controlling its behavior, and leaves it to the program to define the rest of the language.

This is possible because of direct access to compilation. Outside the Forth world, this is nearly unheard of. In the Forth world, this is common and extremely powerful. For an elegant and enlightening read, look at [Frugal Forth](https://github.com/hoytech/frugal), which implements if/then/else conditionals in [3 lines](https://github.com/hoytech/frugal/blob/b3bed9bd85f0f2a23a7f334e0af8dc0392f8c796/init.fs#L99-L101) of user/lib code, with very little compiler support. (Our system implements much _better_ conditionals, but at the cost of more code.)

Unlike the system linked above, and mature systems such as Gforth, our implementation goes straight for machine code. It does not have a VM, bytecode of any kind, or even an IR. I enjoy the simplicity of that.

Most language features are implemented in `f_lang.f`, bootstrapping the language on the fly, often via inline assembly.

Unlike other compiler writers, I focused on keeping the system clear and educational as much as I could. Compilers don't have to be full of impenetrable garbage. They can be full of obvious stuff you'd expect, and can learn from.

## Show me the code!

Note: in this system, Forth is defined _in Forth_ on the fly via the `f_lang.f` file. Other files have to import it first.

```forth
import' ./f_lang.f

: main
  log" hello world!" cr

  10
  #if
    log" branch 0" cr
  #else 20 #elif
    log" branch 1" cr
  #else
    log" branch 2" cr
  #end

  12 for: ind
    ind [ 1 ] logf" current number: %zd" cr
  #end
;

main
```

## Usage

```sh
make
./forth.exe f_lang.f -    # REPL mode.
./forth.exe f_demo.f      # One-shot run.
```

Rebuild continuously while hacking:

```sh
make build_w
```

Unix users will scoff at `*.exe`, but it's convenient in development for hiding binary files from code editors 😛. Drop the extension when adding the executable to your `$PATH`.

When debugging weird crashes, the following are useful:

```sh
make debug_run '<file>'
make debug_run '<file>' RECOVERY=false
make debug_run '<file>' DEBUG=true
```

## Library

Should be usable as a library in another C/C++ program. The "main" file is only a tiny adapter over the top-level library API. See `./c_main.c`.

In this codebase, all C files directly include each other by relative paths, with `#pragma once`. It should be possible to use this as a library by simply cloning the repo into a subfolder and including `./c_interp.c` which provides the top-level API, and includes all other files it needs.

Many procedure names are "namespaced", but many other symbols are not; you may need to create a separate translation unit to avoid pollution. Everything here should be `static`.

## Easy C interop

It's trivial to declare and call extern procedures. Examples can be found in the core file `f_lang.f` which makes extensive use of that. Should work for any linked library, such as libc.

```forth
\ The numbers describe input and output parameters.
1 0 extern: puts
2 1 extern: strcmp

: main
  c" hello world!" puts
  c" one" c" two" strcmp .
;
main
```

## Limitations

### Simplicity vs optimization

This system uses naive single-pass assembly, with a tiny post-pass to fix PC offsets.

This approach is incompatible with many optimizations. It requires every operation to be self-contained, bloating the instruction count. For example, simple addition, even if inlined, becomes 6 instructions instead of 1, and most of them are memory ops to boot:

```forth
push push pop2 add push pop
```

Optimizing Forth compilers, such as VfxForth and Gforth, allocate registers and fictionalize many stack operations. Inevitably, this makes the compiler complex. It's not possible to allocate registers in the first pass through source text. You end up with an IR and more passes. This interferes with self-bootstrapping in library code; simply vomiting instructions into the procedure body no longer suffices, as the program must become IR-aware or negotiate register slots with the compiler.

I had a go, and bounced off the complexity. How to keep an optimizing compiler simple?

Such tradeoffs might be irrelevant when your CPU/ISA is designed for stack languages. Notable example: [GreenArrays chips](https://www.greenarraychips.com) designed or co-designed by Forth's inventor Chuck Moore. Looking at this architecture is enlightening in how much _simpler_ an ISA, efficient compilation, and the hardware/software stack can be.

### Other limitations

- Currently only Arm64 and MacOS.
- Vocabulary is still a work in progress.
- Exceptions print only C traces, not Forth traces.

## Non-standard

For the sake of my sanity and ergonomics, this system does _not_ follow the ANS Forth standard. It improves upon it.

Words are case-sensitive.

Numeric literals are unambiguous: anything that begins with `[+-]?\d` must be a valid number followed by whitespace or EOF.

Non-decimal numeric literals are denoted with `0b 0o 0x`. Radix prefixes are case-sensitive. Numbers may contain cosmetic `_` separators. There is no `hex` mode. There is no support for `& # % $` prefixes.

Many unclear words are replaced with clear ones.

More ergonomic control flow structures:
- `elif` is supported.
- Any amount of `else elif` is popped with a single `end`.
- Most loops are terminated with `end`. No need to remember other terminators.
  - Arguably sometimes less flexible.
- Loop controls like `leave` and `while` are terminated by the same `end` as the loop.

Because the system uses native procedure calls, there is no return stack; see below.

There is no `state` or `does>`. The system supports separately defining regular and compile-time definitions of words, overloading them by name, and placing the variants in separate wordlists. When finding a word by name, the caller must choose the wordlist, and thus the variant of that word.

Exceptions are strings (error messages) rather than numeric codes.

Booleans are `0 1` rather than `0 -1`.

Word-modifying words like `comp_only` can only be used inside colon definitions.

Special _semantic_ roles get special _syntactic_ roles. Other Forths already follow some rules, which we replicate:
- String-parsing words end with `"` and terminate with `"`.
- String-parsing words end with `(` and terminate with `)`.

...but not enough. There are more semantic roles which need syntactic distinction. Rules we add:
- Word-parsing words which declare must end with `:`.
- Word-parsing words which don't declare must end with `'`.
- Other immediate words begin with `#`.

Examples:
- Declaring words: `: let: var: to:`.
  - Code editors are encouraged to highlight the next word like a declaration.
- Parsing words: `' postpone' compile'`.
  - Code editors are encouraged to highlight the next word like a string.
- Control words: `#if #else #end`.

Code editors are encouraged to syntax-highlight these prefixes and suffixes to clarify the semantic roles. Exceptions were made for `\ ( ; [` which are also immediate; special syntax highlighting is recommended for those.

Special syntax highlighting is also recommended for `( ) [ ] { }` _inside_ word names, which are commonly used as delimiters with case-by-case functionality.

Most printing words don't implicitly add spaces or newlines.

The REPL doesn't print "ok".

### No return stack

Since our system is "subroutine-threaded", where procedures call and return natively, it doesn't provide a return stack. The system stack provided by the OS is sufficient.

Operations which would normally use the return stack for scratch space can just use registers, locals, or globals.

## What is compilation

"Compilation" and especially "JIT compilation" are muddy terms. Many interpreters convert text to bytecode, which qualifies as JIT compilation, even if the bytecode remains interpreted. In the Java world, generating bytecode files is considered "compilation".

Especially murky in the Forth world. Docs will often mention compilation, and sometimes decompilation, usually without saying what it compiles _to_ and decompiles _from_: VM code or machine code. Some Forth systems use both, some stop at VM code, ours uses only native code. Yet all these systems qualify as "compilers".

Sometimes it's useful to describe a non-assembler as a "JIT compiler". For example, one of my Go libraries implements JIT-construction of data structures which, when subsequently interpreted, allow efficient deep traversal of complex Go data structures; it delegates setup to "compilation time" and eliminates many costs at "runtime", although both steps occur when the program runs, and the result is interpreted.

Perhaps we should differentiate "JIT compilers" and "JIT assemblers". But at the end of the day, everything is interpreted, one way or another.
