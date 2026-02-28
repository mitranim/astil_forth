## Overview

A native-code Forth system designed for self-bootstrapping. Currently supports only Arm64.

Goals:
- [x] Simple JIT compilation to native code.
- [x] Easy _self-assembly_ in user/lib code.
- [x] Language features can be implemented in user/lib code.
- [x] Enough language features (in user/lib code) to be usable for scripting.
- [x] Support native register-based ABI.
- [x] Avoid a VM or complex IR.
- [x] Keep the code clear and educational for other compiler amateurs.
- [ ] Rewrite in Forth to self-host.
- [ ] AOT compilation.

Everything was written by me. This is not bot-generated.

Most languages ship with a smart, powerful, all-knowing compiler with intrinsic knowledge of the entire language. I feel this approach is too inflexible. It makes the compiler a closed system. Modifying and improving the language requires changing the compiler's source code, which is usually over-complicated.

On top of that, most languages isolate you from the CPU, or even from the OS, from the get-go. Instead of giving you access to the foundations, and providing a convenient pre-built ramp to the high clouds, _all you get_ is high clouds.

This system explores the opposite direction. The interpreter/compiler provides only the bare minimum of intrinsics for controlling its behavior, and leaves it to the program to define the rest of the language.

This is possible because of direct access to compilation. Outside the Forth world, this is nearly unheard of. In the Forth world, this is common and extremely powerful. For an elegant and enlightening read, look at [Frugal Forth](https://github.com/hoytech/frugal), which implements if/then/else conditionals in [3 lines](https://github.com/hoytech/frugal/blob/b3bed9bd85f0f2a23a7f334e0af8dc0392f8c796/init.fs#L99-L101) of user/lib code, with very little compiler support. (Our system implements much nicer conditionals, but at the cost of more code.)

Unlike the system linked above, and mature systems such as Gforth, our implementation goes straight for machine code. It does not have a VM, bytecode of any kind, or even an IR. I enjoy the simplicity of that.

The system comes in two variants which use different call conventions: register-based and stack-based. For code maintenance reasons, they compile separately from differently selected C files. The stack-CC version is legacy and has fewer features.

The outer interpreter / compiler, which is written in C, doesn't actually implement Forth. It provides just enough intrinsics for self-compilation. The _Forth_ code implements Forth, on the fly, bootstrapping via inline assembly.
- Register-CC: boots via `forth/lang.f`.
- Stack-CC: boots via `forth/lang_s.f`.

Unlike other compiler writers, I focused on keeping the system clear and educational as much as I could. Compilers don't have to be full of impenetrable garbage. They can be full of obvious stuff you'd expect, and can learn from.

## Show me the code!

```forth
import' std:lang.f

: main
  log" hello world!" lf

  10
  if
    log" branch 0" lf
  else 20 elif
    log" branch 1" lf
  else
    log" branch 2" lf
  end

  12 0 +for: ind
    ind logf" current number: %zd" lf
  end
;

main
```

## Easy C interop

It's trivial to declare and call extern procedures. Examples can be found in the core files `forth/lang_s.f` and `forth/lang.f`. Should work for any linked library, such as libc.

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

## Exceptions done right: ABI compatibility

When using the register-based calling convention, you can _opt out of exceptions_ for individual words. Every call is "caught", and error values are always explicit.

```forth
: word [ true catches ]
  word0 {           err }
  word1 { val0      err }
  word2 { val1 val2 err }
;
```

Under the hood, an exception is a Go-style error value, implicitly appended to the success outputs. By default it's invisible and treated as an exception. When you "catch", the compiler reveals the error value, and skips the instructions it would normally insert to handle the error. A call becomes Go-style, as shown above. The caller has to check the error.

The resulting system is an exact inverse of Go (and Rust). By default, errors are exceptions and don't clutter the code. When you want explicit errors, it's _for real_, without hidden panics. There is no separate panic mechanism, no stack unwinder. At the ABI level, caller code always has local control.

The best part is cross-language ABI compatibility. Having exceptions without a runtime or unwinder means that other languages can seamlessly call our functions and handle returned errors. We can pass callbacks to libc and guarantee no surprises, such as a panic handler unwinding the C stack.

This is fairly easy to implement. Not aware of any other language with this feature, which is weird. I think it's the only reasonable way of doing error handling.

## Usage

With global installation:

```sh
make install

# Register-based calling convention.
astil std:lang.f -  # REPL mode.
astil some_file.f   # One-shot run.
astil some_file.f - # Run file, then REPL.

# Stack-based calling convention.
astil_s std:lang_s.f - # REPL mode.
astil_s some_file.f    # One-shot run.
astil_s some_file.f -  # Run file, then REPL.
```

Don't forget to `import' std:lang.f` (or `lang_s.f` for stack-CC) inside your program, or on the command line. Without it, the language does not exist.

The REPL is barebones. For a better experience, using `rlwrap` is recommended:

```sh
rlwrap astil std:lang.f -
```

Local-only usage inside this repo:

```sh
make

# Register-based calling convention.
./astil.exe forth/lang.f - # REPL mode.
./astil.exe some_file.f    # One-shot run.
./astil.exe some_file.f -  # Run file, then REPL.

# Stack-based calling convention.
./astil_s.exe forth/lang_s.f - # REPL mode.
./astil_s.exe some_file.f      # One-shot run.
./astil_s.exe some_file.f -    # Run file, then REPL.
```

Rebuild continuously while hacking:

```sh
make clean build_w
```

When debugging weird crashes, the following commands are useful:

```sh
make debug_run '<file>'
make debug_run '<file>' DEBUG=true
make debug_run '<file>' RECOVERY=false
```

## Sublime Text

Due to divergence from the standard, this dialect wants its own syntactic support. This repository includes a syntax implementation for Sublime Text. To enable, symlink the directory `./sublime` into ST's `Packages`. Example for MacOS:

```sh
ln -sfn "$(pwd)/sublime" "$HOME/Library/Application Support/Sublime Text/Packages/astil_forth"
```

Note that for standard-adjacent Forths, you should use the [sublime-forth](https://github.com/mitranim/sublime-forth) package, which is available on Package Control.

## Structure

- `./comp` — outer interpreter / compiler in C. Mostly library code with a small "main" entry point.
- `./forth`:
  - `lang.f` — core language.
  - `testing.f` — simple testing utils.
  - Other files — optional small libraries.
- `./sublime` — editor support for Sublime Text.

## Library

Should be usable as a library in another C/C++ program. The "main" file is only a tiny adapter over the top-level library API. See `comp/main.c`.

In this codebase, all C files directly include each other by relative paths, with `#pragma once`. It should be possible to simply clone the repo into a submodule and include `comp/interp.c` which provides the top-level API and includes all other files it needs.

Many procedure names are "namespaced", but many other symbols are not; you may need to create a separate translation unit to avoid pollution. Almost every symbol is declared as `static`.

## Limitations

### Simplicity vs optimization

A core premise of this system is forward-only single-pass compilation. This keeps it simple, while still allowing a degree of low-hanging optimizations, such as basic register allocation and inlining. Some instructions are reserved in the first pass, and patched in a fixup post-pass.

This approach is at odds with most advanced compiler optimizations, which rely on building and analyzing intermediary representations before assembling. Complex compilers end up with multiple IR levels, and multiple passes. This interferes with self-assembly in library code; simply vomiting instructions into the procedure body no longer suffices; the code must now emit the first-pass IR instead of regular instructions.

I had a go, and bounced off the complexity. How to keep an optimizing compiler simple?

### Other limitations

- Currently only Arm64 and MacOS.
- Vocabulary is still a work in progress.
- Exceptions print only C traces, not Forth traces. (Opt-in via env `TRACE=true`.)

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
- Loop controls like `leave` and `while` are terminated by the same `end` as the loop, and can be used many times in one loop.

Because the system uses native procedure calls, there is no return stack; see below.

There is no `state` or `does>`, or any form of state-smartness. Instead, the system uses two wordlists:
- "exec" -- execution-time words; not immediate.
- "comp" -- compile-time words; immediate.

Each word can be defined _twice_: an "exec" variant and a "comp" variant. In compilation mode, the "comp" variant is used first. In interpretation mode, the "exec" variant is used first. When finding a word by name, the caller must choose the wordlist, and thus the variant.

Exceptions are strings (error messages) rather than numeric codes.

Booleans are `0 1` rather than `0 -1`.

Modifier words like `catches` and `comp_only` can only be used inside colon definitions.

Special _semantic_ roles get special _syntactic_ roles:
- Word-parsing words which declare must end with `:`.
- Word-parsing words which don't declare must end with `'`.
- Custom and unusual control-related words begin with `#`.

Examples:
- Words which declare: `: :: let: var: to:` and more.
  - Syntax highlighters are encouraged to scope the next word like a declaration.
- Parsing words: `' postpone' compile'`.
  - Syntax highlighters are encouraged to scope the next word like a string.
- Custom or unusual control words: `#word_beg #word_end`.
- Well-known control words don't use special characters: `if else end ret` and several more. Syntax highlighters should hardcode them.

Special syntax highlighting is also recommended for `( ) [ ] { }` _inside_ word names. These are commonly used as delimiters, like `T{ }T`.

### No return stack

Since we use native calls and don't target embedded systems, the role of the return stack is fulfilled by scratch registers and system stack space.

Reg-CC emphasizes named local variables. Locals are kept in registers when possible, and spilled to the system stack otherwise. The compiler figures out the locations.

Stack-CC always spills locals to the system stack.

Both conventions make use of temp registers for scratch space.

## Why so much C code

Because I was learning and experimenting. Some of the code is generalized library stuff, checks and error messages (safety and UX), debug logging, code which is relevant but currently unused, and the code-split of supporting two calling conventions.

## Lessons

- Partial self-assembly is possible.
- Single-pass assembly is possible (with fixups 😔).
- Single-pass assembly is compatible with _basic_ optimizations,
  such as simple register allocation and limited inlining.
- Single-pass assembly is incompatible with _advanced_ optimizations.
- Single-pass assembly with fixups scales to a point.

When the system is simple, single-pass compilation (with fixups) seems to be a simpler choice. After a certain level of complexity, an IR-based multi-pass approach starts to look simpler. I feel like this system is just before that threshold.

## What is compilation

"Compilation" and especially "JIT compilation" are muddy terms. Many interpreters convert text to bytecode, which qualifies as JIT compilation, even if the bytecode remains interpreted. In the Java world, generating bytecode files is considered "compilation".

Especially murky in the Forth world. Docs will often mention compilation, and sometimes decompilation, usually without saying what it compiles _to_ and decompiles _from_: VM code or machine code. Some Forth systems use both, some stop at VM code, ours uses only native code. Yet all these systems qualify as "compilers".

Sometimes it's useful to describe a non-assembler as a "JIT compiler". For example, one of my Go libraries implements JIT-construction of data structures which, when subsequently interpreted, allow efficient deep traversal of complex Go data structures; it delegates setup to "compilation time" and eliminates many costs at "runtime", although both steps occur when the program runs, and the result is interpreted.

Perhaps we should differentiate "JIT compilers" and "JIT assemblers". But at the end of the day, everything is interpreted, one way or another.

## Name

The name Astil references a sentient magic grimoire from some anime I watched. A book of magic words. Fitting for a Forth, don't you think?
