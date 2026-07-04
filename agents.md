Some hints about the codebase and our Forth dialect.

## Error handling

Stack-CC legacy: errors are exceptions.

Reg-CC variant:

Default: errors are regular outputs. Compiler doesn't care whether an output semantically represents an error, unless `.try`, `.throw`, or the trailing `err`/`Err` output convention is used. By convention, we use C-strings as errors.

Callsites use `.try` to check visible error value from call; `.try` consumes last output register; requires current word to have its own error output.

By convention, when last output param is named exactly `err` or `Err`, current word gets `Sym.has_err = true`. This lets current word use explicit `.try` / `.throw`; callers with trailing `err` auto-insert `.try` when calling this word.

`.throw` requires 1 arg; requires current word to have its own error output; unconditionally moves input to error output register and returns.

Trailing `err` enables implicit `.try` inside the current word; non-resource code prefers this style. Trailing `Err` keeps errors locally visible; code which involves resource cleanup prefers this style. This policy is function-local and not part of ABI.

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

## Reg-CC call syntax

When editing `.af` under reg-CC:

- Source is whitespace-separated words.
- Numeric literals are classified before word lookup.
- Value-like spelling is `[A-Za-z_][A-Za-z0-9_]*`.
- Call-like spelling is every other non-numeric word.
- Locals and value-like globals must be value-like.
- Callable names need call-like source spelling unless manually plain, such as control-only words and value-like generated words.
- Ident-like callable names get call-like spelling via dot-call.
- Compiler strips exactly one leading `.` for lookup; `.logf` looks up `logf`.
- Regular functions, intrinsics, and externs follow the same call-like rule.
- Non-ident callable names are already call-like; do not add dot to `+`, `!b`, `u/mod`, `fun:`, or `xt'`.
- Control-only words are plain by design: `if ifz else elif elifz end loop leave again assert`.
- Control words which operate on runtime args use call-like spelling. Example control words which require dot-call: `.then .while .try .throw .ret .recur`.
- At file/eval root, `.ret` stops interpreting current input and leaves interpreter stack as-is; inside a compiled word, `.ret` returns current outputs.
- Stored names cannot begin with `.` or look like numeric literals.
