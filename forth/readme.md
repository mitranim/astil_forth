The actual Forth system.

Core files which bootstrap Forth via self-assembly:
- `lang.af` — reg-CC; default language.
- `lang_s.af` — stack CC; considered legacy.

The rest are mostly library files: interfaces to `libc`; higher-level IO wrappers with nicer errors; various other utils.

The outer interpreter / compiler is written in C and located in `../comp`.
