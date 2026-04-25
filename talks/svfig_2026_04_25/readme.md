# Combining JIT & AOT; interpreter-free Forth

- Freedom of execution: freely mixing JIT and AOT.
- Compiling Forth programs without an interpreter or runtime.
- Ending the split between interpreted and compiled languages.



## Intro

More about me: https://mitranim.com.

Audience: compiler amateurs (like myself) who might be interested in turning their JIT language into a "proper" compiled language, with interpreter-free static executables.

I've only done this for Arm64 + MacOS (Mach-O executable format). The lessons should apply to other formats too.

This is a follow-up from prior talks about my (heretical) Forth system; see SVFIG 2026-Jan and 2026-Feb; see recordings on YT.

Quick recap of Astil Forth:

- not VM-based
- native code only
- self-assembling
- self-bootstrapping
- made for register CPUs
- compiled code doesn't use the data stack
  - no runtime needed
  - easier for executables!



## Disclaimers

Combining interpretation and AOT-compilation isn't new. But it _is_ incredibly rare, especially outside of Forth.

I'm sure many Forth authors have already done this, but it seems hard to find specific examples. Please do point them out to me.

SVG and ASCII graphics for this talk are slopbot-generated. (But accurate.)



## Show me!

We must actually _run_ a program first. It defines `main` and decides whether to run it immediately, or only in AOT.

Via CLI:

```sh
astil examples/aot_cli.af --build=out.exe

./out.exe

pagestuff -a out.exe
otool -hvl out.exe
make disasm file=out.exe && open local/out.s
```

From inside a running program:

```forth
: main { -- exit }
  has_interp
  if
    " hello world (JIT mode)!" log lf
  else
    " hello world (AOT-compiled)!" log lf
  end
  0
;

\ optional:
main

xt' main " out.exe" compile_executable

\ in your shell:
\
\ ./out.exe

\ Note: NO source file!
```

(First run takes longer because of MacOS security checks. This only happens once for each executable.)

### Repeated compilation in REPL

You can repeatedly update a program's state, and snapshot each state into its own executable:

```sh
astil std:lang.af -
```

```forth
: main { -- exit } " hello world! (first)" log lf 0 ;

xt' main " out0.exe" compile_executable

: main { -- exit } " hello world! (second)" log lf 0 ;

xt' main " out1.exe" compile_executable

: main { -- exit } " hello world! (third)" log lf 0 ;

xt' main " out2.exe" compile_executable
```

```sh
./out0.exe && ./out1.exe && ./out2.exe

# hello world! (first)
# hello world! (second)
# hello world! (third)
```

Changes to mutable data persist into executables:

```forth
123 var: VAL

: main { -- exit } VAL @ . 0 ;

main \ 123

456 VAL !

main \ 456

xt' main " out.exe" compile_executable
```

```sh
./out.exe
# 456
```



## Problem: the language split

- `0_language_split.txt`
- `0_language_split.svg`

- We have a split between scripting and compiled languages.
- JIT: good scripting, prototyping, fast iteration.
- AOT: good deployment, good for final users.

Most languages lock into one execution mode, which creates friction. How many projects started in Python but ended up rewritten in C++? And Java is the worst joke; wait for "compilation", only to find out you need an interpreter for the resulting image.

Some scripting languages "compile" by bundling an interpreter.

Astil Forth shows an alternative: run your program in JIT mode, AOT-snapshot resulting code for deployment.



## NOT

- `1_not.md`
- `1_ours_theirs.svg`
- `1_dev_vs_prod.svg`
- `1_dev_vs_prod.txt`

What we're NOT doing:

- no VM bytecode
- no VM image
- no bundled interpreter
- no separate compiler

Contrast: Gforth, Deno, Bun.



## How this works

- `3_jit_to_aot_layout.svg`
- `3_jit_to_aot_layout.txt`
- `3_validation.txt`
- `3_pc_rel_off.txt`
- `3_mach_o_layout.svg`
- `3_dis.asm`

Turns out, it's really easy in a JIT compiler to dump program state into a runnable binary. Key: position-independent code, and keeping control of your memory (instead of `malloc`-ing stuff).

- JIT heap + data → dumped into executable file.
- JIT memory layout → virtual memory segments.
- Machine code is kept position-independent.
- Instructions referencing other code, or data, or external symbols, always use PC-relative offsets.
  - "PC" = "program counter": address of current instruction.
- Offsets between code / data / symbols are preserved in AOT.



### Linking

- `4_link.txt`
- `4_dyld.md`
- `4_dyld_fixup_chain.svg`

We have to instruct the OS to load the dylinker and patch addresses of external symbols. Dylinker instructions are in the `__LINKEDIT` section; it patches the `__got` section at runtime.


Words which rely on interpreter state are not callable in AOT. Validated at compilation time. (Caller-callee relations are known.)



### Fastest AOT?

AOT "compilation" takes ≈1 millisecond; it's just `memcpy` with paperwork!



## Internals

Relevant reading in the repo:

- `comp/mach_o.c`
- `comp/comp.h`
- `clib/mach_o.h`



## Performance

See `../../bench/results.txt`.

JIT execution involves compiling the entire language (≈3ms on my system), followed by the actual program. AOT-compiled executables skip that cost.

AOT performance is otherwise identical to JIT.



## Lessons

Astil Forth proves that execution modes DO NOT have to be mutually exclusive:

- interpretation
- JIT → fast development
- AOT → nice deployment

Separation between interpretation and compilation is NOT inevitable.

We prove that being able to freely choose between JIT and AOT execution is _easy_ and _practical_. It shows how to collapse that split.

Why are major languages **NOT** doing this? We need this in more languages!



## Outro

https://github.com/mitranim/astil_forth

My info and contacts on https://mitranim.com.

I like to chat. Ping me up on Discord / Telegram / email.
