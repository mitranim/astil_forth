// Various internal functions used by interpreter intrinsics.
#pragma once
#include "./c_comp.c"
#include "./c_read.c"
#include "./lib/cli.h"
#include "./lib/io.c"
#include <dlfcn.h>

static void interp_snapshot(Interp *interp) {
  interp->snap.comp     = interp->comp;
  interp->snap.ints_len = (Ind)stack_len(&interp->ints);
  interp->snap.syms_len = (Ind)stack_len(&interp->syms);
}

/*
Restores the interpreter to a consistent state.
Should be used when handling errors in REPL mode.

TODO: drop currently being defined symbol from list and dict.
Our dicts don't support deletion at the moment.
*/
static void interp_rewind(Interp *interp) {
  const auto snap = &interp->snap;

  interp->comp = comp_rewind(snap->comp, interp->comp);
  stack_trunc_to(&interp->ints, snap->ints_len);

  const auto syms     = &interp->syms;
  const auto len_prev = snap->syms_len;

  /*
  TODO: support deletion in dicts.
  For now, accessing a partially-defined word is UB.

  const auto len_next = (Ind)stack_len(syms);
  const auto words = &interp->words;
  for (Ind ind = len_prev; ind < len_next; ind++) {
    const auto sym = &syms->floor[ind];
    dict_del(words, sym->buf.name);
  }
  */

  stack_trunc_to(syms, len_prev);
}

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

static Err interp_call_norm(Interp *interp, const Sym *sym) {
  try(comp_code_ensure_sym_ready(&interp->comp.code, sym));
  const auto fun = comp_sym_exec_instr(&interp->comp, sym);

  IF_DEBUG(eprintf(
    "[system] calling word " FMT_QUOTED
    " at instruction address %p (" READ_POS_FMT ")\n",
    sym->name.buf,
    fun,
    READ_POS_ARGS(interp->reader)
  ));

  const auto err = arch_call_norm(interp, sym);

  IF_DEBUG(eprintf(
    "[system] done called word " FMT_QUOTED "; error: %p\n", sym->name.buf, err
  ));
  return err;
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
    "; stack pointer after call: %p; error: %s\n",
    sym->name.buf,
    interp->ints.top,
    err
  ));
  return err;
}

static Err interp_call_extern(Interp *interp, const Sym *sym) {
  IF_DEBUG(eprintf(
    "[system] calling external procedure " FMT_QUOTED
    " at address %p; inp_len: %d; out_len: %d\n",
    sym->name.buf,
    sym->ext_proc,
    sym->inp_len,
    sym->out_len
  ));

  const auto err = arch_call_extern(&interp->ints, sym);

  IF_DEBUG(eprintf(
    "[system] done called extern procedure " FMT_QUOTED "\n", sym->name.buf
  ));
  return err;
}

static Err interp_call_sym(Interp *interp, const Sym *sym) {
  const auto comp = &interp->comp;

  if (sym->comp_only && !comp->ctx.sym) {
    return err_sym_comp_only(sym->name.buf);
  }

  switch (sym->type) {
    case SYM_NORM:     return interp_call_norm(interp, sym);
    case SYM_INTRIN:   return interp_call_intrin(interp, sym);
    case SYM_EXT_PTR:  return int_stack_push(&interp->ints, (Sint)sym->ext_ptr);
    case SYM_EXT_PROC: return interp_call_extern(interp, sym);
    default:           unreachable();
  }
}

static Err interp_require_current_sym(const Interp *interp, Sym **out) {
  return comp_require_current_sym(&interp->comp, out);
}

static Err err_undefined_word(const char *name) {
  return errf("undefined word " FMT_QUOTED, name);
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

static const Err ERR_QUIT = "quit";

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
static Err interp_import(Interp *interp, const char *path) {
  const auto prev      = interp->reader;
  const auto prev_path = prev ? prev->file_path.buf : nullptr;
  Reader     next      = {};
  interp->reader       = &next;
  const auto err       = interp_import_inner(interp, prev_path, path);
  interp->reader       = prev;
  return err;
}

static Err interp_require_sym(Interp *interp, const char *name, Sym **out) {
  const auto sym = dict_get(&interp->words, name);
  if (!sym) return err_undefined_word(name);
  if (out) *out = sym;
  return nullptr;
}

static Err interp_read_word(Interp *interp) {
  const auto read = interp->reader;
  try(read_word(read));
  IF_DEBUG(eprintf("[system] read word: " FMT_QUOTED "\n", read->word.buf));
  return nullptr;
}

static Err interp_read_sym(Interp *interp, Sym **out) {
  try(interp_read_word(interp));
  return interp_require_sym(interp, interp->reader->word.buf, out);
}

/*
Stores constant data of arbitrary length, namely a string,
into the constant region, and compiles `( -- addr size )`.

TODO: either provide a way to align, or auto-align to `sizeof(void*)`.
*/
static Err interp_comp_const(Interp *interp, const U8 *buf, Ind len, U8 reg) {
  try(asm_alloc_const_append_load(&interp->comp, buf, len, (U8)reg));

  IF_DEBUG(eprintf(
    "[system] appended constant with address %p and length " FMT_UINT "\n",
    buf,
    (Uint)len
  ));

  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  sym->norm.has_loads = true;
  return nullptr;
}

// TODO: either provide a way to align, or auto-align to `sizeof(void*)`.
static Err interp_comp_static(Interp *interp, Ind len, U8 reg) {
  U8 *addr;
  try(asm_alloc_data_append_load(&interp->comp, len, reg, &addr));
  try(int_stack_push(&interp->ints, (Sint)addr));

  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  sym->norm.has_loads = true;
  return nullptr;
}

static Err err_sym_out_bounds(const Sym *floor, const Sym *ceil, const Sym *val) {
  return errf(
    "expected a word address between %p and %p; got an out of bounds value: %p",
    floor,
    ceil,
    val
  );
}

static Err interp_validate_sym_ptr(Interp *interp, Sym *sym) {
  const auto syms = &interp->syms;
  if (sym >= syms->floor && sym < syms->ceil) return nullptr;
  return err_sym_out_bounds(syms->floor, syms->ceil, sym);
}

static Err interp_parse_until(Interp *interp, U8 delim) {
  const auto read = interp->reader;
  read_skip_whitespace(read);
  try(read_until(read, (U8)delim));

  // IF_DEBUG(eprintf(
  //   "[system] parsed until delim " FMT_CHAR "; length: " FMT_UINT
  //   "; string: %s\n",
  //   (int)delim,
  //   read->buf.len,
  //   read->buf.buf
  // ));

  const auto ints = &interp->ints;
  try(int_stack_push(ints, (Sint)read->buf.buf));
  try(int_stack_push(ints, (Sint)read->buf.len));

  // IF_DEBUG({
  //   eprintf(
  //     "[system] stack_len(ints) after parsing: " FMT_SINT "\n", stack_len(ints)
  //   );
  //   eprintf("[system] stack pointer after parsing: %p\n", ints->top);
  // });
  return nullptr;
}

static Err interp_read_extern(Interp *interp, void **out_addr) {
  try(interp_read_word(interp));
  const auto name = interp->reader->word.buf;

  IF_DEBUG(
    eprintf("[system] read name of extern symbol: " FMT_QUOTED "\n", name)
  );

  const auto addr = dlsym(RTLD_DEFAULT, name);

  if (!addr) {
    return errf(
      "unable to find extern symbol " FMT_QUOTED "; error: %s", name, dlerror()
    );
  }

  IF_DEBUG(eprintf(
    "[system] found address of extern symbol " FMT_QUOTED ": %p\n", name, addr
  ));

  if (out_addr) *out_addr = addr;
  return nullptr;
}

static Err interp_extern_proc(Interp *interp, Sint inp_len, Sint out_len) {
  void *addr;
  try(interp_read_extern(interp, &addr));

  if (inp_len < 0) {
    return err_str("negative input parameter count");
  }
  if (inp_len > ARCH_INP_PARAM_REG_LEN) {
    return err_str("too many input parameters");
  }
  if (out_len < 0) {
    return err_str("negative output parameter count");
  }
  if (out_len > 1) {
    return err_str("too many output parameters");
  }

  const auto sym = stack_push(
    &interp->syms,
    (Sym){
      .type     = SYM_EXT_PROC,
      .name     = interp->reader->word,
      .ext_proc = addr,
      .inp_len  = (U8)inp_len,
      .out_len  = (U8)out_len,
      .clobber  = ARCH_VOLATILE_REGS, // Only used in reg-based call-conv.
    }
  );

  dict_set(&interp->words, sym->name.buf, sym);
  comp_register_dysym(&interp->comp, sym->name.buf, (U64)addr);
  return nullptr;
}

static Err interp_find_word(Interp *interp, const char *name, Ind len) {
  if (DEBUG) aver((Sint)strlen(name) == len);

  const auto sym = dict_get(&interp->words, name);
  if (!sym) return err_undefined_word(name);

  IF_DEBUG(
    eprintf("[system] found symbol " FMT_QUOTED " at address %p\n", name, sym)
  );

  try(int_stack_push(&interp->ints, (Sint)sym));
  return nullptr;
}

static Err interp_get_local(
  Interp *interp, const char *buf, Ind len, Local **out
) {
  const auto comp = &interp->comp;
  Word_str   name = {};
  Local     *loc;

  try(valid_word(buf, len, &name));
  try(comp_local_get_or_make(comp, name, &loc));
  if (out) *out = loc;
  return nullptr;
}
