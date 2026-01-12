#pragma once
#include "./c_asm.c"
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
#include "./lib/set.c"
#include "./lib/stack.c"
#include <string.h>
#include <unistd.h>

static Err interp_deinit(Interp *interp) {
  const auto syms = &interp->syms;

  for (stack_range(auto, sym, syms)) sym_deinit(sym);
  list_deinit(&interp->syms);
  dict_deinit(&interp->dict);

  Err err = nullptr;
  err     = either(err, asm_deinit(&interp->asm));
  err     = either(err, stack_deinit(&interp->ints));
  err     = either(err, stack_deinit(&interp->syms));

  *interp = (Interp){};
  return err;
}

static Err interp_init_syms(Interp *interp) {
  const auto syms = &interp->syms;
  const auto asm  = &interp->asm;

  for (auto intrin = INTRIN; intrin < arr_ceil(INTRIN); intrin++) {
    const auto sym = stack_push(syms, *intrin);
    sym->name.len  = (Ind)strlen(sym->name.buf);
    dict_set(&interp->dict, sym->name.buf, sym);
    asm_register_dysym(asm, sym->name.buf, (U64)sym->intrin.fun);
  }

  IF_DEBUG({
    const auto syms = &interp->dict;
    aver(dict_has(syms, ":"));
    aver(dict_has(syms, ";"));

    const auto gots = &asm->gots;
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
  try(asm_init(&interp->asm));
  try(interp_init_syms(interp)); // Requires `asm_init` first.

  // Snapshot for recovery from exceptions in interactive mode.
  interp->asm_snap = interp->asm;

  IF_DEBUG({
    eprintf("[system] interpreter: %p\n", interp);
    eprintf("[system] integer stack floor: %p\n", interp->ints.floor);
    eprintf(
      "[system] instruction floor (writable): %p\n", interp->asm.code_write.dat
    );
    eprintf(
      "[system] instruction floor (executable): %p\n", interp->asm.code_exec.dat
    );
  });
  return nullptr;
}

/*
Restores the interpreter to a consistent state.
Should be used when handling errors in REPL mode.
*/
static void interp_rewind(Interp *interp) {
  interp->asm = interp->asm_snap;
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
    is_aligned(&interp->asm) &&
    is_aligned(&interp->asm_snap) &&
    is_aligned(&interp->ints) &&
    is_aligned(&interp->syms) &&
    is_aligned(&interp->dict) &&
    is_aligned(&interp->reader) &&
    is_aligned(&interp->defining) &&
    asm_valid(&interp->asm) &&
    reader_valid(interp->reader) &&
    stack_valid((const Stack *)&interp->ints) &&
    stack_valid((const Stack *)&interp->syms) &&
    dict_valid((const Dict *)&interp->dict)
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

We can now afford slightly more informative error messages.
*/
static void interp_handle_err(Interp *interp, Err err) {
  eprintf(TTY_RED_BEG "error:" TTY_RED_END " %s\n", err);
  backtrace_print();
  interp_rewind(interp);
}

static Err interp_num(Interp *interp, U8 head, Sint sign) {
  Sint num;
  try(read_num(interp->reader, head, sign, &num));

  IF_DEBUG(eprintf("[system] read number: " FMT_SINT "\n", num));

  if (interp->compiling) {
    asm_append_stack_push_imm(&interp->asm, num);
    IF_DEBUG(eprintf("[system] compiled push of number " FMT_SINT "\n", num));
    return nullptr;
  }

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

Err interp_call_sym(Interp *interp, const Sym *sym) {
  if (sym->comp_only && !interp->defining) {
    return err_sym_comp_only(sym->name.buf);
  }

  switch (sym->type) {
    case SYM_NORM: {
      return asm_call_norm(&interp->asm, interp->reader, sym, interp);
    }

    case SYM_INTRIN: {
      IF_DEBUG({
        eprintf(
          "[system] calling intrinsic word " FMT_QUOTED
          " at address %p; stack pointer before call: %p\n",
          sym->name.buf,
          sym->intrin.fun,
          interp->ints.top
        );
      });

      Err err = nullptr;

      if (sym->throws) {
        typedef Err(Fun)(Interp *);
        const auto fun = (Fun *)sym->intrin.fun;
        err            = fun(interp);
      }
      else {
        typedef void(Fun)(Interp *);
        const auto fun = (Fun *)sym->intrin.fun;
        fun(interp);
      }

      IF_DEBUG(eprintf(
        "[system] done called intrinsic word " FMT_QUOTED
        "; stack pointer after call: %p; err after call: %s\n",
        sym->name.buf,
        interp->ints.top,
        err
      ));
      return err;
    }

    case SYM_EXT_PTR: {
      stack_push(&interp->ints, (Sint)sym->ext_ptr.addr);
      return nullptr;
    }

    case SYM_EXT_PROC: {
      try(asm_call_extern_proc(&interp->ints, sym));
      return nullptr;
    }

    default: unreachable();
  }
}

static void sym_auto_comp_only(Sym *caller, const Sym *callee) {
  if (caller->comp_only) return;
  if (!callee->comp_only) return;

  caller->comp_only = true;

  IF_DEBUG(eprintf(
    "[system] word " FMT_QUOTED " uses compile-only word " FMT_QUOTED
    ", marking as compily-only\n",
    caller->name.buf,
    callee->name.buf
  ));
}

// Redundant with the caller-callee graph. TODO consider deduping.
static void sym_auto_interp_only(Sym *caller, const Sym *callee) {
  if (caller->interp_only) return;
  if (!callee->interp_only) return;

  caller->interp_only = true;

  IF_DEBUG(eprintf(
    "[system] word " FMT_QUOTED " uses interpreter-only word " FMT_QUOTED
    ", marking as interpreter-only\n",
    caller->name.buf,
    callee->name.buf
  ));
}

static void sym_auto_throws(Sym *caller, const Sym *callee) {
  if (callee->throws) caller->throws = true;
}

Err compile_call_sym(Interp *interp, Sym *callee) {
  Sym *caller;
  try(current_sym(interp, &caller));
  sym_auto_comp_only(caller, callee);
  sym_auto_interp_only(caller, callee);

  if (callee->norm.inlinable) return interp_inline_sym(interp, callee);

  set_add(&caller->callees, callee);
  set_add(&callee->callers, caller);
  sym_auto_throws(caller, callee);

  const auto asm = &interp->asm;

  switch (callee->type) {
    case SYM_NORM: {
      asm_append_call_norm(asm, caller, callee);
      break;
    }
    case SYM_INTRIN: {
      asm_append_call_intrin(asm, caller, callee, INTERP_INTS_TOP);
      break;
    }
    case SYM_EXT_PTR: {
      asm_append_load_extern_ptr(asm, callee->name.buf);
      break;
    }
    case SYM_EXT_PROC: {
      asm_append_call_extern_proc(asm, caller, callee);
      break;
    }
    default: unreachable();
  }

  IF_DEBUG({
    eprintf(
      "[system] compiled call of symbol " FMT_QUOTED "\n", callee->name.buf
    );
  });
  return nullptr;
}

static Err interp_word(Interp *interp, U8 head) {
  const auto read = interp->reader;
  try(read_word_after(read, head));

  const auto word = read->word;
  IF_DEBUG(eprintf("[system] read word: " FMT_QUOTED "\n", word.buf));

  if (interp->compiling && asm_appended_local_push(&interp->asm, word.buf)) {
    return nullptr;
  }

  const auto sym = dict_get(&interp->dict, word.buf);
  if (!sym) return err_undefined_word(word.buf);

  if (!interp->compiling || sym->immediate) {
    return interp_call_sym(interp, sym);
  }
  return compile_call_sym(interp, sym);
}

// Head byte is `-` or `+`, the rest is either a numeric literal or a word.
static Err interp_arith(Interp *interp, U8 head) {
  const auto read = interp->reader;

  U8 next;
  try(read_ascii_printable(read, &next));

  if (HEAD_CHAR_KIND[next] == CHAR_DECIMAL) {
    return interp_num(interp, next, (head == '-' ? -1 : 1));
  }

  reader_backtrack(read, next);
  return interp_word(interp, head);
}

static Err interp_err(Reader *read, const Err err) {
  if (err == ERR_QUIT) return err;
  return reader_err(read, err);
}

static Err interp_loop(Interp *interp) {
  for (;;) {
    U8 next;
    try(read_ascii_printable(interp->reader, &next));

    // SYNC[interp_loop].
    switch (HEAD_CHAR_KIND[next]) {
      case CHAR_WHITESPACE: continue;
      case CHAR_DECIMAL:    {
        try(interp_num(interp, next, 1));
        continue;
      }
      case CHAR_ARITH: {
        try(interp_arith(interp, next));
        continue;
      }
      case CHAR_WORD: {
        try(interp_word(interp, next));
        continue;
      }
      case CHAR_EOF:         return nullptr;
      case CHAR_UNPRINTABLE: [[fallthrough]];
      default:               return err_unsupported_char(next);
    }
  }
  return nullptr;
}

static Err interp_include_inner(Interp *interp, const char *path) {
  const auto read = interp->reader;
  try(str_copy(&read->file_path, path));

  defer(file_deinit) FILE *file = nullptr;
  try(file_open_fuzzy(path, &path, &read->file));
  reader_init(read);

  IF_DEBUG(eprintf("[system] including file: " FMT_QUOTED "\n", path));

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
Err interp_include(Interp *interp, const char *path) {
  const auto prev = interp->reader;
  Reader     next = {};
  interp->reader  = &next;
  const auto err  = interp_include_inner(interp, path);
  interp->reader  = prev;
  return err;
}
