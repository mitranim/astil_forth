# Libc Return-Value Policy

Purpose: keep current/future direction for handling C/libc return values at raw
`extern:` callsites. This is planning memory, not a changelog.

## Core Model

Internal Forth values are machine words. Internal `-1` must be real machine-word
`-1`, not a narrow C value accidentally left in a register.

Raw `extern:` declarations currently encode only input/output arity. C return
type knowledge lives at raw callsites and in wrapper words. Treat each raw libc
call as a small boundary where the callsite normalizes C's return convention into
Forth's machine-word convention.

Near-term design: keep one `extern:` form. Model the only return-width case that
matters in practice: C `int`. Let normal machine-word returns pass through as-is.

## Width Repair

Use `cint_to_cell` to sign-extend a C `int` return value to a machine word. Use
it directly after raw libc calls whose return type is C `int` and whose value
will be kept as a Forth signed word.

Example for C `int` returns:

```forth
.blah .cint_to_cell { val }
if val <0 .then .errno " some_msg" .os_err .throw end
```

Pure `0|-1 :Cint` calls do not need width repair when the callsite only branches
on success/failure. Branch on the raw result and convert the failure to a project
error there:

```forth
if .blah .then .errno " some_msg" .os_err .throw end
```

Machine-word returns need no width repair.

Example for `ssize_t` / word-sized signed returns:

```forth
.blah { len }
if len <0 .then .errno " some_msg" .os_err .throw end
```

`size_t` returns need no width repair. They are unsigned machine-word values on
the supported ABI. Handle them according to the specific API's count/short-count
semantics.

## Error Sentinel Policy

Use explicit callsite logic for APIs whose contract says a negative return means
failure and `errno` explains the failure. Normalize C `int` values first only
when the value must live as a Forth signed word, such as `open`'s `fdes|-1`.

Pure `0|-1 :Cint`:

```forth
if .blah .then .errno " some_msg" .os_err .throw end
```

Visible `fdes|-1 :Cint`:

```forth
.open .cint_to_cell { fdes }
if fdes <0 .then .errno " open" .os_err .throw end
```

Word-sized `len|-1`:

```forth
.read { len }
if len <0 .then .errno " read" .os_err .throw end
```

Use API-specific handling for comparison APIs or APIs where negative values are
valid non-error results, such as `strcmp`.

Use count/short-count handling for `size_t` APIs such as `fread`/`fwrite`; those
use separate EOF/error checks such as `feof`/`ferror`.

## Negative Error Branches

Prefer direct branching for pure `0|-1 :Cint`; prefer explicit `.cint_to_cell`
followed by `<0` / `-1 =` only when the C `int` value must live as a Forth
signed word. Avoid broad helpers for negative error sentinels; each callsite
should make width repair and errno capture visible.

Prefer a full rename from `int_to_cell` to `cint_to_cell`, without a broad alias.
That keeps callsites explicit about C `int` width repair.

## Extern Comment Annotations

Annotate narrow C returns in comments. Normal machine-word returns can stay
unannotated.

Preferred forms:

```forth
2 1 extern: open  ( path flag -- fdes|-1 :Cint )
1 1 extern: close ( fdes -- 0|-1 :Cint )
3 1 extern: read  ( fdes buf len -- len|-1 )
2 1 extern: strcmp ( cstr0 cstr1 -- cmp :Cint )
```

Use `:Cint` for C `int` returns. Use result spelling such as `fdes|-1`, `0|-1`,
`char|-1`, or `cmp` to document semantics.

## Wrapper Policy

Library-style wrappers are useful for common total-failure cases, especially
because project errors are strings. Wrap repeated high-level operations, and
keep uncommon or low-level cases as raw `extern:` calls with explicit width
repair and clear comments.

Prefer wrappers for repeated high-level operations such as open/read/write-all.
Raw `extern:` use remains fine when callers need specific failure handling or
when a wrapper would add more code than value.

## Source Facts

- `open` returns a nonnegative file descriptor or `-1` with `errno`:
  https://man7.org/linux/man-pages/man2/open.2.html
- `read` returns `ssize_t`: byte count, zero at EOF, or `-1` with `errno`:
  https://man7.org/linux/man-pages/man2/read.2.html
- `write` returns `ssize_t`: byte count or `-1` with `errno`; short writes are
  possible:
  https://man7.org/linux/man-pages/man2/write.2.html
- `fread`/`fwrite` return `size_t` item counts. Short count can mean EOF or
  error, and callers must use `feof`/`ferror` to distinguish:
  https://man7.org/linux/man-pages/man3/fread.3.html
- `ferror` reports the stream error indicator; it does not set `errno`. If a
  later cleanup or diagnostic formatting call may clobber `errno`, save `errno`
  before cleanup/formatting:
  https://man7.org/linux/man-pages/man3/ferror.3.html
