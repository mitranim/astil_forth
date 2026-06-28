Some hints about the codebase and our Forth dialect.

## Error handling

Stack-CC legacy: errors are exceptions.

Reg-CC variant:

Default: errors are regular outputs. Compiler doesn't care whether an output semantically represents an error, unless `try`, `throw`, or `try_all` is used. By convention, we use C-strings as errors.

Callsites use `try` to check visible error value from call; `try` consumes last output register; requires current word to have its own error output.

By convention, when last output param is named exactly `err`, current word gets `Sym.has_err = true`. This lets current word use explicit `try` / `throw`, and lets callers compiled under `try_all` auto-insert `try` when calling this word.

`throw` requires 1 arg; requires current word to have its own error output; unconditionally moves input to error output register and returns.

`true try_all` enables implicit `try`; non-IO code prefers this style. Code which involves resource cleanup prefers `false try_all`, which is the default. This setting is file-scoped and function-scoped.

How to return:

- Success: just non-errors; compiler rejects nil error as redundant.
- Partial failure: non-errors + non-nil error.
- Total failure: error + `throw`; non-error outputs are undefined.

## Debugging

The outer interpreter provides some debug words; list can be seen in `comp/intrin_list_cc_reg.c`.

In reg-CC, the most useful ones are: `dis'` for disassembly; `#debug_ctx` for comptime context; `debug_arg` for printing decimal + hex + binary.

Avoid `debug_on` because it blows your mind with too much output.

Comptime uses interpreter-provided stack for control frames; it can be viewed with `debug_stack` when debugging control flow structures or root-level operations.

## Scripting

You can run one-off Astil Forth scripts by passing `--eval=<script_content>` to the interpreter. Needs `lang.af` too; full form usually like this: `make run args='lang.af --eval="<script_content>"'`.

## Code reuse

Avoid blatant duplication.

Reuse existing functions which cleanly fit the use case.

Dedup blatantly redundant logic via reusable functions; threshold: 3+ lines; 2+ callsites.

However, avoid creating non-reusable functions for just one new callsite.

When deleting callsites, fold non-reusable auxiliary functions into the last remaining callsite.

Some reusability criteria:
- Already-existing multiple callsites where it saves code.
- Clear separation of domains; for example: pure "asm" functions return instructions; "comp" functions may take them for side effects. Pure functions in general have slightly more reusability.
- Lifecycle functions; common case: per-type init/deinit.

## Codegen tests

General rule for testing:
- We test semantics first.
- Then we test codegen optimizations where applicable.
- We don't bother testing pessimizations.
