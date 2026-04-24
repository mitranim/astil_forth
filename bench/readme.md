Microbenchmarks comparing:
- Register-CC version of Astil Forth in AOT mode.
- Register-CC version of Astil Forth in JIT mode.
- Stack-CC version of Astil Forth in JIT mode.
- Gforth.
- (occasional) C (Clang).
- (occasional) JS (Bun).

Run `./bench.sh`. Requires [`hyperfine`](https://github.com/sharkdp/hyperfine).

Results: [`bench/result.txt`](./result.txt).

Summary: in these _very limited_ microbenchmarks, the reg-CC implementation of Astil Forth trounces interpreters, and vaguely approaches C (Clang with `-O2`).
