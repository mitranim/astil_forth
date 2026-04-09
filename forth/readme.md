The actual Forth system.

Core files which bootstrap Forth via self-assembly:
- `lang.af` — the default calling convention.
- `lang_s.af` — stack CC; considered legacy.

The rest are mostly library files: testing utils, interfaces to `libc`, etc.

The outer interpreter / compiler is written in C and located in `../comp`.
