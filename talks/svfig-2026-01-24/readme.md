(Presented in the [Silicon Valley Forth Interest Group](https://www.meetup.com/sv-fig) on 2026-01-24. YouTube recording: [link](https://www.youtube.com/watch?v=_4U1BR1U_oM).)

## Intro

A bit about myself.

- Mostly a web developer; about a decade of experience.
  - JS, Python, Clojure, Go, Rust: high-level languages.
- Made many cool web apps.
- Maintain my own web stack in Go and JS.
- This background made Forth a challenge.

More info and contacts on https://mitranim.com.

Passionate about programming language design.
- Long fascination with unique powers of Lisp and Forth.
- Wrote several Lisp-to-JS transpilers.
- Long-term dream: create a "final" language suitable for everything.
  Needed to start at the foundations.
- Forth is my first native-code compiler.

## What is this about

- Showcase and experience report of:
- A new Forth oriented towards self-assembly and self-bootstrapping.
  - Allows programs to assemble themselves.
  - Interpreter provides very few intrinsics.
  - Interpreter does _not_ provide the language.
  - Program code defines Forth on the fly.
  - "Native-only" design.
    - Only native code.
    - Only native calls.
    - No VM, no bytecode, no interpreter overhead in compiled code.
  - Easiest interop with C.
  - Supports two calling conventions:
    - Stack-based: simple, flexible.
    - Register-based: much better performance.
- Simple demos and benchmarks.
- Brief tour of the internals.
- Syntax: how to improve the reading experience.
- Lessons learned.
- Breaking changes and improvements from other Forths and why.

## Disclaimers

- No prior experience with Forth, assembly, or native compilers.
- No compiler design books read; I like to learn from scratch by inventing.
- The system is new and little-tested.
- The Forth vocabulary is incomplete.
- The compiler is _mildly_ optimizing.
- No floating point support yet.
- Currently only Arm64 and MacOS.

## Show me the code!

See the numbered "slide" files in this directory.

(Demo a variety of scripts.)

(Show self-assembler.)

(Show self-bootstrap.)

(Talk about limited set of intrinsics.)

(Mention syntax improvements.)

## Architecture

C compiler (knows nothing about Forth)
  -> Forth initialization file (implements the language on the fly)
  -> actual program.

## Intrinsics

Interpreter / compiler provides very few intrinsics: just enough for the program to bootstrap itself.

Slide: `./8_intrin.md`.

Note that even basic arithmetic is missing. The program can define arithmetic and its own assembler on the fly. The interpreter just needs to support self-compilation.

## Why

- Learn machine code.
- Learn about native compilation.
- Curious about the idea of self-assembling programs.
- Curious about the idea of self-bootstrapping languages.
- Readable and educational assemblers need to exist.
  - The workings of other Forths were incomprehensible to me.
  - Couldn't understand _anything_ in the sources of Gforth and VfxForth.

Larger picture:
- Languages must support freely mixing JIT and AOT compilation.
  - Most languages don't; this causes real problems.
  - I want to prove that it's practical.
  - (AOT in my system is still WIP.)
- Languages should be self-bootstrapping.
  - Most languages have an all-knowing, all-powerful compiler.
  - They always depend on another language.

### Why in C

- Couldn't write this in Forth.
- Wanted to learn C.

## Challenges

My coding background is in high-level, garbage-collected, object-oriented languages. This made Forth a struggle.

It turned out that I can't do concatenative programming. My brain can handle 1, maybe 2 cells. As soon as it's `dup swap rot whatever roll pick over`, it's over. So I have to use locals. Which some people frown upon!

If stuff like the above is comfortable for you, you have my respect. This is impossible for most programmers.

A contentious one: concatenative programming using the stack is extremely error prone. There; I've said it!

- I automatically think in _structures_.
- When considering a problem, my brain begins by inventing a data structure holding all data related to that problem.
- Good structures map 1-to-1 to the problem, describe it, make it easier to solve and less error prone.
- Forth provides no obvious structures.
- Took time to realize how to emulate them.

JIT compilation can be extremely error prone, and debugging involves hours of reading assembly and reverse-engineering what you just generated...

Programming in binary (not even in assembly!) is extremely error prone.

C is extremely error prone when it comes to pointer arithmetic and low-level memory manipulation, which is prolific inside a compiler.

Programming without types and type-checking is extremely error prone.

Programming without a compiler holding your hand about variables, arguments, parameters is extremely error prone.

## Lessons

- Partial self-assembly is possible.
- Single-pass assembly is possible (with fixups 😔).
- Single-pass assembly is incompatible with many optimizations.
- Good Forth syntax highlighting is possible and greatly improves user experience. Requires strong naming conventions, see later notes.
- Dealing with raw machine code can be error-prone and frustrating. Debugging can be hard.
  - Getting real friendly with LLDB / GDB / debugger of choice is a must.
- Is this even portable? It _could_ be ported, so maybe?
- Building a compiler doesn't just involve building an intepreter. It involves building many interpreters for many sub-languages: the IR data structures you're creating, introspecting, and transforming. The more complex the requirements, the more interpreters you have.

## Breaking changes and why

See the slide `./9_changes.md`.

## Future plans

- Ahead of time compilation (AOT).
- Floating point stuff.
- Support other architectures, namely x64.
- Write some actual programs (lol).

## Special thanks

- Chuck Moore for discovering this stuff.
- Forth enthusiasts in general.
- Doug Hoyte: [Frugal Forth](https://github.com/hoytech/frugal) is very educational to read. And very simple!
- Justine Turney: https://justine.lol. Reading her work inspired me to finally learn native code and C.

## Outro

More info and contacts on https://mitranim.com.

I'd love to chat more! Ping me up via Discord / email / Telegram / Zoom or whatever.

(P.S. offtopic: currently available for hire!)

## Future topics: poll

What to present in the next meetup?

**Vote by typing a number in chat!**

### 1. Progress report: stackless Forth

Native register-based calls:

- Quick tour of the implementation.
- Showcase and demos.
  - Already have it working, but not ready for a demo today.
- Performance comparison, benchmarks.
- Where to prefer a stack.
- What effects does this have on code?
- What does it take to keep this internally simple?

### 2. How to improve syntax

- How to improve your Forth experience with syntax highlighting and other visual cues for easier reading.
- What to change in the language.
- What to change in the editor.

### 3. Experience report

- Experience report of someone coming from high-level non-concatenative languages; struggles and solutions; making Forth usable.
- Experience report of learning from scratch about C, Forth, assembly, and native code compilers.
- How to learn by writing a compiler.
- Could be useful to other folks interesting in writing their own.

### 4. Deeper dive into the compiler and assembler

- Could be interesting to other compiler writers.
- Kept it simple. Which was hard!
- Kept it educational. Hopefully this helps others!
- Tips and tricks, debugging hell, how to.
- Coming from a total novice in these things!

## Questions and impressions

I want to hear from the audience!

- Ask questions!
- Tell me your what you think about this!

<!--
Votes:

  Joseph O'Connor (Jan 24, 2026, 20:28)
  1, 4
  andreas (Jan 24, 2026, 20:28)
  3
  Brad Nelson (Jan 24, 2026, 20:31)
  1, 4

TODO check chat for contact: Stephen Adels
-->
