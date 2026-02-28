# Stackless Forth and cross-language ABI compatibility

- Comparison with stack-based calls.
- Breaking the mold: Forth is not about stacks.
- Native calls: how to keep register allocation simple.
- Exceptions the right way: ABI compatibility with C.
- Releasing the code: https://github.com/mitranim/astil_forth.

## Intro

More about me: https://mitranim.com.

This is a follow-up on the January talk about my Forth compiler; see SVFIG 2026-Jan-24.

My "slides" for this talk do NOT fit into the time window. They are in the repository. If you find this interesting, read the slides after the talk, at your leisure.

Disclaimer: still haven't read any compiler books. Still exploring from scratch.

The talk is going to be fairly low-level.
- This is about native code and compiler algorithms.
- Target audience: people who write small compilers.
- Which, in Forth, is probably everyone, so...

## Show me the code!

See `./0_comparison.md` which compares quality of generated code between stack-CC and reg-CC.

See `./1_disasm.md` on how to read generated code.

## Register allocation in single-pass assembly

See `./2_reg_alloc.md` which explains our simple algorithm.

## Exceptions done right

See `./3_exceptions.md` which showcases the value of doing exceptions the "right" way: ABI interop, simplicity, consistent performance.

See `/examples/threads_*.f` which show seamless interop between `libc` and our system, which uses "exceptions".

## Structs

`./4_structs.f`: quick mention of struct support; ABI interop again.

## System stack allocation

`./5_stack_alloc.md`: quick glance at stack-allocated local references and C-style `alloca`.

## Tricks and workarounds

See the "slide" `./6_tricks.md` on optimization tricks.

It's a long one!

## Lessons

Single-pass assembly and compiler simplicity is compatible with basic register allocation and fully native calls.

Single-pass assembly with fixups scales _to a point_. After a certain level of complexity, it stops being helpful, and starts asking for an IR-based multi-pass approach. I feel my system is near that threshold.

## Why do this?

- ABI compatibility between languages.
- Performance on register-oriented CPUs.
- Usability.

Forth has nothing to do with stacks.

- Stack-based coding is _totally superficial_.
- Forth is about _simplicity and machine affinity_.
- Match the machine's preference.
- Match the programmer's preference.
  - It's not stacks; it's names.
- Optimizing a stack machine abstraction is actually more complicated!
  - Authors of Gforth spent like 30 years tuning it.
  - On my CPU, it's trounced by an algorithm which barely takes a page of code.
  - And this is _still Forth_.

When Chuck "discovered" Forth, there wasn't an OS around to offer a stack. The language _was_ the OS, so it had to provide one. Chuck figured, this was both necessary and sufficient. Well, for many programmers as well as modern machines, it's not sufficient; adding named locations in both hardware and software makes it better.

Additionally: we should not be stacking layers of interpreters and adapters. The CPU interprets the machine code; there's a common ABI for each platform; that's enough. Stick to the ABI.

The affinity has changed. Forth is allowed to change, too.

## Outro

The system is now public on GitHub: https://github.com/mitranim/astil_forth.

- Check out `./forth`: language and library code.
- Check out `./examples`: mostly `libc` interop.
- Check out `./talks`:
  - This and previous SVFIG talks are in the repo.
  - There are notes in this talk I didn't have time to cover; especially optimization tricks. Check the files.

More info and contacts on https://mitranim.com.

I like to chat. Ping me up on Discord / Telegram / email.
