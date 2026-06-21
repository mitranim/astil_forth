## TOC

- [TOC](#toc)
- [Overview](#overview)
- [Show me the code!](#show-me-the-code)
- [Why](#why)
  - [Always extensible](#always-extensible)
  - [Always fast](#always-fast)
- [Easy C interop](#easy-c-interop)
- [Errors done right: ergonomics and ABI](#errors-done-right-ergonomics-and-abi)
- [CLI](#cli)
- [Structure](#structure)
- [Sublime Text](#sublime-text)
- [Library](#library)
- [Tricks and optimizations](#tricks-and-optimizations)
- [Register allocation and greediness](#register-allocation-and-greediness)
- [Performance](#performance)
- [Limitations](#limitations)
- [Non-standard](#non-standard)
- [Why so much C code](#why-so-much-c-code)
- [Lessons](#lessons)
- [What is JIT](#what-is-jit)
- [What is compilation](#what-is-compilation)
- [Name](#name)

## Overview

Astil Forth is an experimental system which uses Forth as a model for exploring self-bootstrapping, self-assembly, and a unified JIT & AOT execution model. Currently supports only Arm64 + MacOS.

Goals:
- [x] Explore combined JIT execution & AOT snapshotting.
- [x] Explore self-bootstrap: defining language on the fly in itself.
- [x] Explore self-assembly in user/library code.
- [x] Explore single-pass assembly (with fixups).
- [x] Be usable for scripting.
- [x] Support direct C interop.
- [x] Avoid a VM or complex IR.
- [x] Keep compiler clear for compiler amateurs.
- [ ] Eventually rewrite in Forth to self-host.

Non-goals:
- Following other compiler designs.
- Portability to multiple ISAs.
- ANS Forth compatibility.
- Complex optimizations.
- Complete stdlib.

The system is mostly human-made. Bots were employed for knowledge search, reviews, tests, and some refactorings.

No dependencies (other than libc). Uses custom assemblers in both C and Forth code.

I gave talks about Astil Forth in SVFIG meetings (Silicon Valley Forth Interest Group). Currently, that's the closest substitute for documentation, in addition to this readme and [`./examples`](./examples).
- 2026-Jan: intro to the system, self-assembly showcase, demos — https://www.youtube.com/watch?v=_4U1BR1U_oM
- 2026-Feb: register allocation and proper ABI interop with C — https://www.youtube.com/watch?v=rCF7wAB2wFQ
- 2026-Apr: turning JIT compilation into AOT compilation — https://www.youtube.com/watch?v=vkJJhURJt78
- [`./talks`](./talks) directory: presentation notes with more details.

Unlike other compiler writers, I focused on keeping the system clear and educational as much as I could. Compilers don't have to be full of impenetrable garbage. They can consist of obvious stuff you'd expect, and can learn from.

## Show me the code!

IO and conditionals:

```forth
use' lang.af

fun: main
  " hello world!" log lf

  if 10 then
    " branch 0" log lf
  elif 20 then
    " branch 1" log lf
  else
    " branch 2" log lf
  end

  0 { ind }
  loop
    ind 12 < while
    " current number: %zd" ind logf lf
    inc: ind
  end
end

main
```

Easy self-assembly (Arm64):

```forth
\ add Xd, Xn, Xm
fun: asm_add_reg { Xd Xn Xm -- instr }
  Xd Xn Xm asm_pattern_arith_reg
  0b1_0_0_01011_00_0_00000_000000_00000_00000 or
end

\ add x0, x0, x1
fun: + { i0 i1 -- i2 }
  [
    0                 comp_realloc_reg
    0 0 1 asm_add_reg comp_instr
    1                 comp_args_set
  ]
end

\ brk 666
fun: abort
  [
    0b110_101_00_001_0000001010011010_000_00 comp_instr
  ]
end
```

Easy AOT compilation; can be used from inside a program, or via CLI flags:

```forth
fun: main { -- exit }
  " hello world!" log lf
  0
end

xt' main           \ Reference to the entry point.
" out.exe"         \ Where to put the executable.
compile_executable

\ Run `./out.exe` in your shell to print the message.
```

Traditional style `: word ;` is also availble, but `fun: word end` is preferred for stylistic reasons and consistency with control structures, which are also terminated with `end`.

## Why

### Always extensible

Most languages ship with a smart, powerful, all-knowing compiler with intrinsic knowledge of the entire language. I feel this approach is too inflexible. It makes the compiler a closed system. Modifying and improving the language requires changing the compiler's source code, which is usually over-complicated.

On top of that, most languages isolate you from the CPU, or even from the OS, from the get-go. Instead of giving you access to the foundations, and providing a convenient pre-built ramp to the high clouds, _all you get_ is high clouds.

Astil Forth explores the opposite direction. The interpreter / compiler provides only the bare minimum of intrinsics for controlling its behavior, and leaves it to the program to define the rest of the language. See [`./forth/lang.af`](./forth/lang.af).

This is possible because of direct access to compilation. Outside the Forth world, this is nearly unheard of. In the Forth world, this is common and extremely powerful. For an elegant and enlightening read, look at [Frugal Forth](https://github.com/hoytech/frugal), which implements if/then/else conditionals in [3 lines](https://github.com/hoytech/frugal/blob/b3bed9bd85f0f2a23a7f334e0af8dc0392f8c796/init.fs#L99-L101) of user/lib code, with very little compiler support. Astil Forth also implements conditionals without compiler support, in barely a page of code, and makes them _much_ nicer to use than standard Forth.

Unlike Frugal and mature systems such as Gforth, Astil Forth goes straight for machine code. It does not have a VM, bytecode of any kind, or even an IR. I enjoy the simplicity of that, despite the non-portability.

The system comes in two variants which use different call conventions: register-based and stack-based. For code simplicity reasons, they compile separately, from differently selected C files. The stack-CC version is legacy and has fewer features.

The outer interpreter / compiler, written in C, doesn't actually implement Forth. It provides just enough intrinsics for self-compilation. The _Forth_ code implements the language, on the fly, bootstrapping via inline assembly.
- Register-CC: boots via [`./forth/lang.af`](./forth/lang.af).
- Stack-CC: boots via [`./forth/lang_s.af`](./forth/lang_s.af).

### Always fast

Most languages lock into one execution model: either interpretation/JIT, or AOT compilation. This causes friction. Interpreted languages are nicer for scripting, prototyping, iterating. Compiled languages produce faster code and make deployment easier (just a binary), but compilation slows down development. How many projects were rewritten in C++ after starting in Python? And Java is the worst joke; wait for "compilation", only to find out you need an interpreter for the resulting image.

Astil Forth _proves_ that combining interpretation/JIT and AOT compilation in one language is easy, practical, and useful. We could end the "language split".

In development, Astil is a scripting language with instant startup and feedback. For "production", it builds a standalone executable, deployable as-is, and without any interpreter inside.

Some other scripting languages offer "compilation", but it's almost never the real deal. Some examples:
- Gforth builds a VM image file, which requires the interpreter.
- Deno and Bun bundle an interpreter when building an executable.

Some recent AOT languages offer comptime execution, which is a nice step towards bridging the gap. Examples include Nim and Zig. Unfortunately, to the best of my knowledge, they do this with interpretation, not compilation, limiting the usefulness. Bun's sources (Zig) have comments like "this used to be calculated at comptime, but too slow, so we moved this to runtime". Astil Forth avoids this problem: "comptime" execution uses assembled code, not an interpreter.

Fun note. AOT compilation in Astil Forth might be the "fastest" of any language, because it simply dumps the already JIT-compiled code of your program (plus data and dyld symbols) into an executable file. At the time of writing, it takes about a millisecond for simple programs.

## Easy C interop

It's trivial to declare and call external functions, such as dynamically linked `libc` stuff. Examples can be found in the core files [`./forth/lang.af`](./forth/lang.af) and [`./forth/lang_s.af`](./forth/lang_s.af).

The reg-CC version of Astil Forth, the default one, uses the native calling convention of the target platform, matching C. Its words can be passed to C by raw instruction addresses, and just work. In addition, it lets you define structs which match the C ABI. This allows perfect interop. See [`./examples`](./examples).

```forth
\ The numbers describe input and output parameters.
1 0 extern: puts
2 1 extern: strcmp

fun: main
  " hello world!" puts
  " one" " two" strcmp .
end
main
```

## Errors done right: ergonomics and ABI

In reg-CC, our error handling combines ergonomics and ABI compatibility:
- An error is just a C-style string pointer.
- An error is the last output param (Go-style).
- Shortcuts `try throw` for local control (Swift-style).
- `try_all` for opting into implicit "try".
- Callers receive errors as values, even if callees "throw".
- Works across ABI boundaries between languages.
- No unwinder; all control is local.

By convention, if the last output parameter is named _exactly_ `err`, the compiler knows it's an error; this is similar to Swift's `throws` annotation. Either way, by default, errors are explicit values. They can be simply returned.

```forth
fun: word { -- err }
  word0 { err }           if err then err ret end
  word1 { val0 err }      if err then err ret end
  word2 { val1 val2 err } err
end
```

For ergonomics, the compiler automatically zeroes the error register if you don't explicitly return it:

```forth
fun: word { -- err }
  word0 { val }           if val then     ret end
  word1 { val0 err }      if err then err ret end
  word2 { val1 val2 err } err
end
```

`try` consumes errors and returns early:

```forth
fun: word { -- err }
  word0 try
  word1 try { val0 }
  word2 try { val1 val2 }
end
```

`try_all` makes this implicit, allowing even shorter code. `try_all` in file root affects the current file (and no other). `try_all` inside a word affects _only_ that word. However, it does _not_ remove errors from signatures. The signature of every word precisely describes its ABI.

```forth
true try_all

fun: word { -- err }
  word0
  word1 { val0 }
  word2 { val1 val2 }
end

fun: word [ false try_all ] end \ Only inside this word.
```

The resulting system is a hybrid between C/Go/Swift styles. Like Swift, we provide shortcuts to make errors behave more like exceptions (locally), and make nil errors implicit. Like Go, we use multiple output parameters, and prefer errors to be strings with useful messages. Unlike Go, we don't have panics, so control is always local, making cross-language calls worry-free. We can pass callbacks to `libc` without any surprises.

The above doesn't quite apply to stack-CC, which still treats errors as "exceptions".

## CLI

With global installation:

```sh
# (Choose between perf and debug.)
make install PROD=true
make install

# Get some instructions:
astil --help

# Register-based calling convention:
astil lang.af -  # REPL mode.
astil <file>   # One-shot run.
astil <file> - # Run file, then REPL.

# Stack-based calling convention:
astil_s lang_s.af - # REPL mode.
astil_s <file>    # One-shot run.
astil_s <file> -  # Run file, then REPL.
```

Don't forget to import `lang.af` via `use' lang.af` (or `use' lang_s.af` for stack-CC) inside your program, or via CLI args.

Note on "use": absolute or explicitly-relative paths are resolved against the current file (PWD in REPL); unprefixed paths are "standard library" only.

The REPL is barebones. For a better experience, using `rlwrap` is recommended:

```sh
rlwrap astil lang.af -

# Or use this shortcut from repo root:
make repl
```

Compile executables via `--build`:

```sh
astil <file> --build=out.exe
./out.exe
```

Local-only usage inside this repo:

```sh
make
./astil.exe
./astil_s.exe
```

Rebuild continuously while hacking:

```sh
make clean build_w
```

When debugging weird crashes, the following can be useful:

```sh
make debug_run '<file>'
make debug_run '<file>' DEBUG=true
make debug_run '<file>' TRACE=true
make debug_run '<file>' RECOVERY=false
```

## Structure

- [`./forth`](./forth):
  - [`lang.af`](./forth/lang.af) — language core.
  - [`testing.af`](./forth/testing.af) — simple testing utils.
  - Other files — optional small libraries; mostly interfaces to `libc` IO.
- [`./examples`](./examples) — how to use `libc` for IO, networking, threading.
- [`./comp`](./comp) — outer interpreter / compiler in C. Mostly library-style code with a small "main" entry point.
- [`./talks`](./talks) — presentation "slides" for the talks I gave in [SVFIG](https://www.meetup.com/sv-fig/) about Astil Forth.
  - For now, this is the substitute for documentation, especially the February talk.
- [`./sublime`](./sublime) — editor support for Sublime Text.

## Sublime Text

Due to divergence from the standard, this dialect prefers its own syntactic support. This repository includes a syntax implementation for Sublime Text. To enable, symlink the directory `./sublime` into ST's `Packages`. Example for MacOS:

```sh
ln -sfn "$(pwd)/sublime" "$HOME/Library/Application Support/Sublime Text/Packages/astil_forth"
```

Note that for standard-adjacent Forths, you should use the [sublime-forth](https://github.com/mitranim/sublime-forth) package, which is available on Package Control.

## Library

Should be usable as a library in another C/C++ program. The "main" file is only a tiny adapter over the top-level library API. See `comp/main.c`.

In this codebase, all C files directly include each other by relative paths, with `#pragma once`. It should be possible to simply clone the repo into a submodule and include `comp/interp.c` which provides the top-level API and includes all other files it needs.

Many function names are "namespaced", but many other symbols are not; you may need to create a separate translation unit to avoid pollution. Almost every function is declared as `static`.

## Tricks and optimizations

I consider this a "mildly optimizing" compiler. It maintains the convenience of single-pass self-assembly while employing several tricks compatible with it. Some are detailed in a ["slide"](talks/svfig_2026_02_28/6_tricks.md) for the February SVFIG presentation.

In both reg-CC and stack-CC:
- Avoid emitting unnecessary prologue and epilogue.
- Inline small leaf functions.
- Delay undecidable instructions, patch them in a fixup pass.
  - Used for prologue, `ret try throw recur`, and more.

In reg-CC:
- Place inputs and outputs in registers, matching the native call ABI.
- Prefer to keep locals in registers.
- Relocate locals lazily and only when necessary.
- Elide relocations of parameter locals when possible.
- Associate locals with param regs, reuse when possible.
- Keep track of clobbers, preserve caller-saved registers when possible.
- Fold comptime constants in most arithmetic words.
- ...Other small tricks. Some are word-specific.

## Register allocation and greediness

The reg-CC version of Astil Forth takes an extremely simple yet effective approach to register allocation. It treats parameter registers as a stack, operating at the _bottom_ of that stack rather than top.

Inside every word definition, the "register stack" starts "empty". This lets the compiler immediately assign values to registers without any outer context. When you write code like `10 20 30`, the values are immediately assigned to `x0 x1 x2`. This matches the Arm64 call ABI; a subsequent call to a verb with 3 inputs will "just work" whether it's an Astil Forth word or a C function.

Register allocation for locals is trickier and smarter. They often have to be relocated, but the final destinations are unknown until the end of a word definition. The compiler reserves instructions, builds a list of pending relocations, and patches them in a fixup pass. This allows us to preserve the single-pass compilation strategy. The compiler employs several tricks to elide unnecessary relocations.

Runtime-only verbs are greedy: they must consume the entire "register stack", replacing it with their outputs. This often requires "stashing" values out of the way into locals, which also makes the code clearer.

Comptime words are allowed to operate at the _top_ of the register stack. For example, all arithmetic words like `+ - * /` come with two definitions. Their runtime definitions are `x0 x1 -> x0`, but their comptime definitions "pop" and "push" top argument registers, often allowing "normal" concatenative code without locals in compiled code. This works by asking and telling about argument registers at comptime: `comp_args_get comp_args_set`.

## Performance

See [`./bench`](./bench). Summary: in these _very limited_ microbenchmarks, the reg-CC version of Astil Forth trounces VM interpreters, approximates other JITs, and vaguely approximates Clang C with `-O2`. Needless to say, this shouldn't be over-generalized. The compiler is simple, stupid.

## Limitations

### Simplicity vs optimization

A core premise of this system is forward-only single-pass compilation. This keeps it simple, while still allowing a degree of low-hanging optimizations, such as basic register allocation and inlining. Some instructions are reserved in the first pass, and patched in a fixup post-pass.

This approach is at odds with most advanced compiler optimizations, which rely on building and analyzing intermediary representations before assembling. Complex compilers end up with multiple IR levels, and multiple passes. Complexity interferes with self-assembly; simply vomiting instructions into the function body no longer suffices; the code must now emit the first-pass IR instead of regular instructions.

I had a go, and bounced off the complexity. How to keep an optimizing compiler simple?

### Other limitations

- Currently only Apple Silicon (MacOS + Arm64).
- Vocabulary / stdlib is somewhat limited.
- Top-level exceptions print only C traces, not Forth traces. (Opt-in via `--trace`.)

## Non-standard

For the sake of my sanity and ergonomics, Astil Forth does _not_ follow the ANS Forth standard. It improves upon it.

Words are case-sensitive.

Numeric literals are unambiguous: anything that begins with `[+-]?\d` must be a valid number followed by whitespace or EOF.

Non-decimal numeric literals are denoted with `0b 0o 0x`. Radix prefixes are case-sensitive. Numbers may contain cosmetic `_` separators. There is no `hex` mode. There is no support for `& # % $` prefixes.

Words are preferred over punctuation (except delimiters):

```
' -> xt'
: -> fun:
; -> end
```

Many unclear words are replaced with clear ones.

More ergonomic control flow structures:
- All conditionals and loops are terminated with `end`. No need to remember other terminators.
- Conditionals take the form `if then else end`, which reads much nicer.
- `elif` is supported.
- Any amount of conditional branches is terminated with a single `end`.
- Any amount of `leave` or `while` is terminated with the same `end` as the loop.

Reg-CC supports exactly _one_ loop form: `loop … end`, with `leave while again` auxiliaries. Other loop forms don't buy anything.

```forth
loop
  predicate while
  body
  if done then leave end
  if skip then again end
end
```

There is no `state` or `does>`. Instead, the system uses two wordlists:
- "exec" — runtime words; not immediate.
- "comp" — comptime words; immediate.

Each word can be defined _twice_: an "exec" variant and a "comp" variant. In compilation mode, the "comp" variant is used first. In interpretation mode, the "exec" variant is used first. When finding a word by name, the caller must choose the wordlist, and thus the variant.

Errors are strings (error messages) rather than numeric codes.

Booleans are `0 1` rather than `0 -1`.

Word-modifiers like `comp_only` are used inside definitions, not outside. Modifiers executed in file scope (`try_all`) affect an entire file.

Special _semantic_ roles get special _syntactic_ roles:
- Word-parsing words which declare end with `:`.
- Word-parsing words which don't declare end with `'`.
- Unusual control-related words begin with `#` or use the `T{ }T` naming style.

Examples:
- Words which declare: `fun: let: var: to:` and more.
  - Syntax highlighters are encouraged to scope the next word like a declaration.
- Parsing words: `use' xt' postpone' compile'`.
  - Syntax highlighters are encouraged to scope the next word like a string.
- Unusual control words: `T{ }T`.
- Well-known control words don't use special characters: `if then else elif end ret` and several more. Syntax highlighters should hardcode them.

Special syntax highlighting is also recommended for `( ) [ ] { }` _inside_ word names. These are commonly used as delimiters, like `T{ }T`.

### No return stack

Because Astil Forth targets Arm64 and assumes an OS, the role of the return stack is fulfilled by registers and the system stack.

Reg-CC emphasizes named local variables. Locals are kept in registers when possible, and spilled to the system stack otherwise. The compiler figures out the locations.

Stack-CC always spills locals to the system stack.

Both conventions make use of temp registers for scratch space.

## Why so much C code

Because I was learning and experimenting. Some of the code is generalized library stuff, checks and error messages (safety and UX), debug logging, code which is relevant but currently unused, and the code-split of supporting two calling conventions.

## Lessons

- Partial self-bootstrap is possible.
- Partial self-assembly is possible.
- Single-pass assembly is possible (with fixups 😔).
- Single-pass assembly is compatible with _basic_ optimizations,
  such as simple register allocation and limited inlining.
- Single-pass assembly is incompatible with _advanced_ optimizations.
- Single-pass assembly with fixups scales to a point.

When the system is simple, single-pass compilation (with fixups) seems to be a simpler choice. After a certain level of complexity, an IR-based multi-pass approach starts to look simpler. I feel like this system is just before that threshold.

## What is JIT

The term "JIT" is used here only for convenience. This doesn't assemble "just" in time; it assembles immediately. Saying "JIT" is just the most compact way to indicate on-the-fly assembly as opposed to interpretation. In the Forth world this is common, usually without buzzwords.

## What is compilation

"Compilation" and especially "JIT compilation" are muddy terms. Many interpreters convert text to VM code, which may qualify as JIT compilation, even if the result remains interpreted. In the Java world, generating VM code files is considered "compilation".

Especially murky in the Forth world. Docs will often mention compilation, and sometimes decompilation, usually without saying what it compiles _to_ and decompiles _from_: VM code or machine code. Some Forth systems use both; some stop at VM code; Astil Forth uses only machine code. Yet all these systems qualify as "compilers".

Sometimes it's useful to describe a non-assembler as a "JIT compiler". For example, one of my Go libraries implements JIT-construction of data structures which, when subsequently interpreted, allow efficient deep traversal of complex Go data structures; it delegates setup to "compilation time" and eliminates many costs at "runtime", although both steps occur when the program runs, and the result is interpreted.

Perhaps we should differentiate "JIT compilers" and "JIT assemblers". But at the end of the day, everything is interpreted, one way or another.

## Name

The name Astil references a sentient magic grimoire from one anime I watched. A book of magic words. Fitting for a Forth, don't you think?

## License

https://unlicense.org
