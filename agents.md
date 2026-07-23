Some hints about the codebase and our Forth dialect.

## Error handling

Stack-CC legacy: errors are exceptions.

Reg-CC variant:

ABI-wise, errors are regular outputs, explicitly declared.

By convention, we use C-strings as errors.

Explicitly returning a nil error is redundant; compiler forbids it.

Naming the last output parameter exactly `err` or `Err` enables use of `.throw` and `.try` inside the current word. Also marks `Sym.has_err = true`, telling callers that this word has a trailing error; makes callers auto-try when their own trailing output is named exactly `err`.

`.throw` returns its input in the current word's error output register; other outputs are undefined; requires current `Sym.has_err = true`.

`.try` is like `.throw` but conditional: consume top argument; test and return as error if non-zero.

Worse: `err .then err .throw end`. Better: `.try`. Worse: `.call { val err } err .then err .throw end`. Better: `.call .try { val }`.

Trailing `err` output (unlike `Err`) also enables local auto-try. Inside a word whose trailing output is named exactly `err`, when calling words with `Sym.has_err = true`, compiler inserts an equivalent of `.try` after each call.

Callers don't care whether callee uses `err` or `Err`; both are just "callee has trailing error output".

Input params, locals, and non-trailing outputs named `err` / `Err` have no
special compiler behavior.

How to return:

- Success: just non-errors; compiler rejects nil error as redundant.
- Partial failure: non-errors + non-nil error.
- Total failure: error + `.throw`; non-error outputs are undefined.

## Debugging

The outer interpreter provides some debug words; list can be seen in `comp/intrin_list_cc_reg.c`.

In reg-CC, the most useful ones are: `dis'` for disassembly; `#debug_ctx` for comptime context; `.debug_arg` for printing decimal + hex + binary.

Avoid `.debug_on` because it blows your mind with too much output.

Comptime uses interpreter-provided stack for control frames; it can be viewed with `.debug_stack` when debugging control flow structures or root-level operations.

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

## Call and name syntax

When editing `.af` under either calling convention:

- Source is whitespace-separated words.
- Numeric literals are classified before word lookup.
- Value-like spelling is `[A-Za-z_][A-Za-z0-9_]*`.
- Call-like spelling is every other non-numeric word.
- Locals and value-like globals must be value-like.
- Word names need call-like source spelling unless manually plain, such as control-only words and value-like generated words.
- Ident-like callable names get call-like spelling by storing a leading dot.
- Lookup is exact; `.logf` looks up the symbol named `.logf`.
- Regular functions, intrinsics, and externs follow the same call-like rule.
- Non-ident callable names are already call-like; do not add dot to `+`, `!b`, `u/mod`, `fun:`, or `xt'`.
- Control-only words are plain by design: `else elif end loop leave again assert`.
- Control words which operate on runtime args use call-like names: `.then .while .try .throw .ret .recur`.
- Word names may begin with `.`, but no stored name may look like a numeric literal.
