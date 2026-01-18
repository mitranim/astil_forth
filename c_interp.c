#pragma once
#include "./c_comp.c"
#include "./c_interp.h"
#include "./c_intrin.c"
#include "./c_read.c"
#include "./c_sym.c"
#include "./lib/cli.h"
#include "./lib/dict.c"
#include "./lib/err.h"
#include "./lib/io.c"
#include "./lib/list.c"
#include "./lib/misc.h"
#include "./lib/stack.c"
#include <string.h>
#include <unistd.h>

static Err interp_deinit(Interp *interp) {
  const auto syms = &interp->syms;

  for (stack_range(auto, sym, syms)) sym_deinit(sym);
  list_deinit(&interp->syms);
  dict_deinit(&interp->words);
  dict_deinit_with_keys((Dict *)&interp->imports);

  Err err = nullptr;
  err     = either(err, comp_deinit(&interp->comp));
  err     = either(err, stack_deinit(&interp->ints));
  err     = either(err, stack_deinit(&interp->syms));

  *interp = (Interp){};
  return err;
}

static Err interp_init_syms(Interp *interp) {
  const auto syms = &interp->syms;
  const auto comp = &interp->comp;

  for (auto intrin = INTRIN; intrin < arr_ceil(INTRIN); intrin++) {
    const auto sym = stack_push(syms, *intrin);
    sym->name.len  = (Ind)strlen(sym->name.buf);
    dict_set(&interp->words, sym->name.buf, sym);
    comp_register_dysym(comp, sym->name.buf, (U64)sym->intrin);
  }

  IF_DEBUG({
    const auto syms = &interp->words;
    aver(dict_has(syms, ":"));
    aver(dict_has(syms, ";"));

    const auto gots = &comp->code.gots;
    aver(gots->inds.len == arr_cap(INTRIN));
    aver(stack_len(&gots->names) == arr_cap(INTRIN));
  });
  return nullptr;
}

static Err interp_init(Interp *interp) {
  *interp       = (Interp){};
  Stack_opt opt = {.len = 1024};

  try(stack_init(&interp->syms, &opt));
  try(stack_init(&interp->ints, &opt));
  try(comp_init(&interp->comp));
  try(interp_init_syms(interp)); // Requires `comp_init` first.

  // Snapshot for rewinding on exception recovery.
  interp->comp_snap = interp->comp;

  IF_DEBUG({
    eprintf("[system] interpreter: %p\n", interp);
    eprintf("[system] integer stack floor: %p\n", interp->ints.floor);
    eprintf(
      "[system] instruction floor (writable): %p\n",
      interp->comp.code.code_write.dat
    );
    eprintf(
      "[system] instruction floor (executable): %p\n",
      interp->comp.code.code_exec.dat
    );
  });
  return nullptr;
}

/*
Restores the interpreter to a consistent state.
Should be used when handling errors in REPL mode.
*/
static void interp_rewind(Interp *interp) {
  interp->comp = comp_rewind(interp->comp_snap, interp->comp);
  stack_trunc(&interp->ints);
}

// clang-format off

/*
Used in unwinding and debugging.
TODO: could use a magic number.
*/
static bool interp_valid(const Interp *interp) {
  return (
    interp &&
    is_aligned(interp) &&
    is_aligned(&interp->comp) &&
    is_aligned(&interp->comp_snap) &&
    is_aligned(&interp->ints) &&
    is_aligned(&interp->syms) &&
    is_aligned(&interp->words) &&
    is_aligned(&interp->reader) &&
    comp_valid(&interp->comp) &&
    reader_valid(interp->reader) &&
    stack_valid((const Stack *)&interp->ints) &&
    stack_valid((const Stack *)&interp->syms) &&
    dict_valid((const Dict *)&interp->words)
  );
}

// clang-format on

/*
Chuck circa June 1970 (PPOL): "type the offending word"; "reset all stacks".

> One more routine belongs in this section: an error routine.
> Whenever an error is detected, a routine should jump to ERROR
> which will type the offending word and an error message. It will
> then reset all stacks and the input pointer and RETURN normally.
>
> The problem of how to treat error messages is an important one. We are in
> a position to do a good job: to avoid setting and testing flags; to avoid
> cascading back through subroutine calls. By clearing the return stack we
> eliminate any pending subroutine returns. By not returning with an error
> flag, we avoid having the subroutine have to worry about errors.
> This simplifies the code, but we must have a standard method of
> handling problems.
>
> The image of a person at a keyboard in invaluable for this purpose.
> No matter what problem arises, we needn't worry about what to do.
> Pass the buck; ask the user. For example, he types a word not in the
> dictionary. What to do? Ask him: type the word and an error message,
> in this case "?". He tries to add 2 numbers and there's only 1 on
> the stack: type the word and "STACK!". He tries to access a field
> beyond the limit of his memory: type the word and "LIMIT!".

We can afford slightly more informative error messages.
*/
static void interp_handle_err(Interp *interp, Err err) {
  eprintf(TTY_RED_BEG "error:" TTY_RED_END " %s\n", err);
  backtrace_print();
  interp_rewind(interp);
}

static Err read_interp_num(Interp *interp) {
  Sint num;
  try(read_num(interp->reader, &num));

  IF_DEBUG(eprintf("[system] read number: " FMT_SINT "\n", num));

  const auto comp = &interp->comp;
  if (comp->ctx.compiling) return comp_append_push_imm(comp, num);

  stack_push(&interp->ints, num);

  IF_DEBUG(eprintf(
    "[system] pushed number " FMT_SINT "; stack len: " FMT_SINT "\n",
    num,
    stack_len(&interp->ints)
  ));
  return nullptr;
}

static Err err_sym_comp_only(const char *name) {
  return errf(
    "unsupported use of compile-only word " FMT_QUOTED
    " outside a colon definition",
    name
  );
}

static Err interp_call_intrin(Interp *interp, const Sym *sym) {
  IF_DEBUG(eprintf(
    "[system] calling intrinsic word " FMT_QUOTED
    " at address %p; stack pointer before call: %p\n",
    sym->name.buf,
    sym->intrin,
    interp->ints.top
  ));

  const auto err = comp_call_intrin(interp, sym);

  IF_DEBUG(eprintf(
    "[system] done called intrinsic word " FMT_QUOTED
    "; stack pointer after call: %p; err after call: %s\n",
    sym->name.buf,
    interp->ints.top,
    err
  ));
  return err;
}

Err interp_call_sym(Interp *interp, const Sym *sym) {
  const auto comp = &interp->comp;

  if (sym->comp_only && !comp->ctx.sym) {
    return err_sym_comp_only(sym->name.buf);
  }

  switch (sym->type) {
    case SYM_NORM: {
      IF_DEBUG(eprintf(
        "[system] calling word " FMT_QUOTED
        " at instruction address %p (" READ_POS_FMT ")\n",
        sym->name.buf,
        sym->norm.exec.floor,
        READ_POS_ARGS(interp->reader)
      ));

      const auto err = interp_call_norm(interp, sym);

      IF_DEBUG(eprintf(
        "[system] done called word " FMT_QUOTED "; error: %p\n", sym->name.buf, err
      ));
      return err;
    }

    case SYM_INTRIN: {
      return interp_call_intrin(interp, sym);
    }

    case SYM_EXT_PTR: {
      stack_push(&interp->ints, (Sint)sym->ext_ptr);
      return nullptr;
    }

    case SYM_EXT_PROC: {
      try(asm_call_extern_proc(&interp->ints, sym));
      return nullptr;
    }

    default: unreachable();
  }
}

static Err interp_word(Interp *interp, Word_str word) {
  const auto comp = &interp->comp;
  const auto name = word.buf;

  if (comp->ctx.compiling) {
    const auto loc = comp_local_get(comp, name);
    if (loc) return comp_append_local_get(comp, loc);
  }

  const auto sym = dict_get(&interp->words, name);
  if (!sym) return err_undefined_word(word.buf);

  if (comp->ctx.compiling && !sym->immediate) {
    return comp_append_call_sym(comp, sym);
  }
  return interp_call_sym(interp, sym);
}

static Err read_interp_word(Interp *interp) {
  const auto read = interp->reader;
  try(read_word(read));

  const auto word = read->word;
  IF_DEBUG(eprintf("[system] read word: " FMT_QUOTED "\n", word.buf));

  return interp_word(interp, word);
}

static Err interp_err(Reader *read, const Err err) {
  if (err == ERR_QUIT) return err;
  return reader_err(read, err);
}

static Err interp_loop(Interp *interp) {
  const auto read = interp->reader;

  for (;;) {
    U8 next;
    try(read_ascii_printable(read, &next));

    switch (HEAD_CHAR_KIND[next]) {
      case CHAR_EOF:        return nullptr;
      case CHAR_WHITESPACE: continue;

      case CHAR_DECIMAL: {
        reader_backtrack(read, next);
        try(read_interp_num(interp));
        continue;
      }

      // `+` and `-` are ambiguous: may begin a number or a word.
      case CHAR_ARITH: {
        U8 next_next;
        try(read_ascii_printable(read, &next_next));
        reader_backtrack(read, next_next);
        reader_backtrack(read, next);

        if (HEAD_CHAR_KIND[next_next] == CHAR_DECIMAL) {
          try(read_interp_num(interp));
          continue;
        }

        try(read_interp_word(interp));
        continue;
      }

      case CHAR_WORD: {
        reader_backtrack(read, next);
        try(read_interp_word(interp));
        continue;
      }

      case CHAR_UNPRINTABLE: [[fallthrough]];
      default:               return err_unsupported_char(next);
    }
  }
  return nullptr;
}

static Err interp_import_inner(
  Interp *interp, const char *prev, const char *next
) {
  if (prev) next = path_join(prev, next);
  const auto real = realpath(next, nullptr);

  if (!real) {
    const auto code = errno;
    return errf(
      "unable to realpath %s; code: %d; msg: %s", next, code, strerror(code)
    );
  }

  const auto imports = &interp->imports;

  if (dict_has(imports, real)) {
    IF_DEBUG(eprintf(
      "[debug] skipping import of already-imported file; path: " FMT_QUOTED
      "; realpath: " FMT_QUOTED "\n",
      next,
      real
    ));
    free(real);
    return nullptr;
  }

  dict_set(imports, real, (Empty){});

  const auto read = interp->reader;
  try(str_copy(&read->file_path, real));

  const char              *path = nullptr; // Don't have to `free` this.
  defer(file_deinit) FILE *file = nullptr;
  try(file_open_fuzzy(real, &path, &read->file));
  reader_init(read);

  IF_DEBUG(eprintf("[system] importing file: " FMT_QUOTED "\n", path));

  for (;;) {
    auto err = interp_loop(interp);
    if (!err) return nullptr;
    err = reader_err(read, err);
    if (!read->tty) return err;
    interp_handle_err(interp, err);
  }
  return nullptr;
}

// There better not be a `longjmp` over this.
Err interp_import(Interp *interp, const char *path) {
  const auto prev      = interp->reader;
  const auto prev_path = prev ? prev->file_path.buf : nullptr;
  Reader     next      = {};
  interp->reader       = &next;
  const auto err       = interp_import_inner(interp, prev_path, path);
  interp->reader       = prev;
  return err;
}
