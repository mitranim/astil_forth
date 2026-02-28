The actual Forth system.

Core files which bootstrap Forth via self-assembly:
- `lang.f` — the default calling convention.
- `lang_s.f` — stack CC; considered legacy.

The rest are mostly library files: testing utils, interfaces to `libc`, etc.

The outer interpreter / compiler is written in C and located in `../comp`.
