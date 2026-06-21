Some hints about the codebase and our Forth dialect.

## Error handling

Stack-CC legacy: errors are exceptions.

Reg-CC variant:

Default: errors are regular outputs. Compiler doesn't care whether an output semantically represents an error, unless `try`, `throw`, or `try_all` is used. By convention, we use C-strings as errors.

Callsites use `try` to check visible error value from call; `try` consumes last output register; requires current word to have its own error output.

By convention, when last output param is named exactly `err`, current word gets `Sym.has_err = true`. This lets current word use explicit `try` / `throw`, and lets callers compiled under `try_all` auto-insert `try` when calling this word.

`throw` requires 1 arg; requires current word to have its own error output; unconditionally moves input to error output register and returns.

In file root, `true try_all` enables implicit `try` in subsequent word definitions; `false try_all` disables it. Inside word definitions, `try_all` can be toggled at comptime via brackets: `[ false try_all ]`. This is word-scoped and resets on `end` to the file-root setting.

How to return:

- Success: just non-errors; compiler rejects nil error as redundant.
- Partial failure: non-errors + non-nil error.
- Total failure: error + `throw`; non-error outputs are undefined.

## Debugging

The outer interpreter provides some debug words; list can be seen in `comp/intrin_list_cc_reg.c`.

In reg-CC, the most useful ones are: `dis'` for disassembly; `#debug_ctx` for comptime context; `debug_arg` for printing decimal + hex + binary.

Avoid `debug_on` because it blows your mind with too much output.

Comptime uses interpreter-provided stack for control frames; it can be viewed with `debug_stack` when debugging control flow structures or root-level operations.
