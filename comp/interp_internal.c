// Various internal functions used by interpreter intrinsics.
#pragma once
#include "./comp.c"
#include "./lib/cli.h"
#include "./lib/io.c"
#include "./lib/path.c"
#include "./read.c"
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
  For now, accessing a partially-defined word
  after recovering from an exception is UB.

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

static Err err_unrecognized_wordlist(Wordlist val) {
  return errf(
    "unrecognized wordlist type %d; known types: WORDLIST_EXEC = %d, WORDLIST_COMP = %d",
    val,
    WORDLIST_EXEC,
    WORDLIST_COMP
  );
}

static Err interp_wordlist(Interp *interp, Wordlist val, Sym_dict **out) {
  switch (val) {
    case WORDLIST_EXEC: {
      if (out) *out = &interp->dict_exec;
      return nullptr;
    }
    case WORDLIST_COMP: {
      if (out) *out = &interp->dict_comp;
      return nullptr;
    }
    default: return err_unrecognized_wordlist(val);
  }
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

static Err interp_require_current_sym(const Interp *interp, Sym **out) {
  return comp_require_current_sym(&interp->comp, out);
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

  const auto err = asm_call_norm(interp, sym);

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
    sym->exter,
    sym->inp_len,
    sym->out_len
  ));

  const auto err = asm_call_extern(&interp->ints, sym);

  IF_DEBUG(eprintf(
    "[system] done called extern procedure " FMT_QUOTED "\n", sym->name.buf
  ));
  return err;
}

static Err err_sym_comp_only(const char *name) {
  return errf(
    "unsupported use of compile-only word " FMT_QUOTED
    " outside a colon definition",
    name
  );
}

static Err interp_call_sym(Interp *interp, const Sym *sym) {
  const auto comp = &interp->comp;

  if (sym->comp_only && !comp->ctx.sym) {
    return err_sym_comp_only(sym->name.buf);
  }

  switch (sym->type) {
    case SYM_NORM:   return interp_call_norm(interp, sym);
    case SYM_INTRIN: return interp_call_intrin(interp, sym);
    case SYM_EXTERN: return interp_call_extern(interp, sym);
    default:         unreachable();
  }
}

static Err err_word_undefined(const char *name) {
  return errf("undefined word " FMT_QUOTED, name);
}

static Err err_word_undefined_in_wordlist(const char *name, Wordlist list) {
  return errf("word " FMT_QUOTED " not found in wordlist %d", name, list);
}

static Err err_word_undefined_in_each_wordlist(const char *name) {
  return errf("word " FMT_QUOTED " not found in any wordlist", name);
}

static Err err_word_comp_only(const char *name) {
  return errf("word " FMT_QUOTED " is compile-time only", name);
}

static Err interp_word(Interp *interp, Word_str word) {
  const auto dict_exec = &interp->dict_exec;
  const auto dict_comp = &interp->dict_comp;
  const auto comp      = &interp->comp;
  const auto ctx       = &comp->ctx;
  const auto name      = word.buf;

  if (ctx->compiling) {
    const auto loc = comp_local_get(comp, name);
    if (loc) return comp_append_local_get_next(comp, loc);

    auto sym = dict_get(dict_comp, name);
    if (sym) return interp_call_sym(interp, sym);

    sym = dict_get(dict_exec, name);
    if (sym) return comp_append_call_sym(comp, sym);

    return err_word_undefined(name);
  }

  auto sym = dict_get(dict_exec, name);
  if (sym) return interp_call_sym(interp, sym);

  sym = dict_get(dict_comp, name);
  if (sym) {
    if (sym->comp_only && !ctx->sym) return err_word_comp_only(name);
    return interp_call_sym(interp, sym);
  }

  return err_word_undefined(name);
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

#ifdef __APPLE__
#include <limits.h>
#include <mach-o/dyld.h>

static const char *get_exec_path(void) {
  static thread_local char path[PATH_MAX];
  uint32_t                 size = arr_cap(path);
  if (_NSGetExecutablePath(path, &size)) return nullptr;
  return path;
}

#else  // __APPLE__
static const char *get_exec_path(void) { return nullptr; }
#endif // __APPLE__

static Err interp_import_stdio(Interp *interp, const char *path) {
  defer(file_deinit) FILE *file = fopen(path, "r");
  if (!file) return err_file_unable_to_open(path);

  const auto read = interp->reader;
  read->file      = file;
  try(str_copy(&read->file_path, path));

  IF_DEBUG(eprintf("[system] reading code from stdio: " FMT_QUOTED "\n", path));

  if (!isatty(fileno(file))) {
    return interp_err(read, interp_loop(interp));
  }

  for (;;) {
    const auto err = interp_err(read, interp_loop(interp));
    if (!err) return nullptr;
    interp_handle_err(interp, err);
  }
  return nullptr;
}

static char *resolve_import_path(const char *prev, const char *next) {
  {
    defer(str_deinit) char *path = path_join(prev, next, false);
    const auto              out  = realpath(path, nullptr);
    if (out) return out;
  }

  // Imports with the scheme `std:` are resolved magically.
  next = str_without_prefix(next, "std:");
  if (!next || !next[0]) return nullptr;

  // Try relative to the interpreter executable.
  const auto exec = get_exec_path();
  if (exec && is_path_abs(exec)) {
    defer(str_deinit) char *base = path_join(exec, "forth", false);
    defer(str_deinit) char *full = path_join(base, next, true);
    const auto              path = realpath(full, nullptr);
    if (path) return path;
  }

  // Try in `~/.local/share/astil`.
  // SYNC[install_path].
  const auto home = getenv("HOME");
  if (home) {
    defer(str_deinit) char *base = path_join(home, ".local/share/astil", true);
    defer(str_deinit) char *full = path_join(base, next, false);
    const auto              path = realpath(full, nullptr);
    if (path) return path;
  }

  return nullptr;
}

static Err interp_import_inner(
  Interp *interp, const char *prev, const char *next
) {
  // Owned by `interp.imports`, freed in `interp_deinit`.
  const auto path = resolve_import_path(prev, next);

  if (!path) {
    const auto code = errno;
    return errf(
      "unable to resolve import path " FMT_QUOTED " (from " FMT_QUOTED
      "); code: %d; msg: %s",
      next,
      prev,
      code,
      strerror(code)
    );
  }

  const auto imports = &interp->imports;

  if (dict_has(imports, path)) {
    IF_DEBUG(eprintf(
      "[debug] skipping import of already-imported file; path: " FMT_QUOTED
      "; realpath: " FMT_QUOTED "\n",
      next,
      path
    ));
    free(path);
    return nullptr;
  }

  defer(file_deinit) FILE *file = fopen(path, "r");
  if (!file) return err_file_unable_to_open(path);

  const auto read = interp->reader;
  read->file      = file;
  try(str_copy(&read->file_path, path));
  dict_set(imports, path, EMPTY);

  IF_DEBUG(eprintf("[system] importing file: " FMT_QUOTED "\n", path));
  return interp_err(read, interp_loop(interp));
}

// There better not be a `longjmp` over this.
static Err interp_import(Interp *interp, const char *path) {
  const auto prev            = interp->reader;
  const auto prev_path       = prev ? prev->file_path.buf : nullptr;
  const auto stdio_file_path = file_path_stdio(path);
  Reader     next            = {};
  interp->reader             = &next;

  const auto err = stdio_file_path
    ? interp_import_stdio(interp, stdio_file_path)
    : interp_import_inner(interp, prev_path, path);

  interp->reader = prev;
  return err;
}

static Err interp_read_word(Interp *interp) {
  const auto read = interp->reader;
  try(read_word(read));
  IF_DEBUG(eprintf("[system] read word: " FMT_QUOTED "\n", read->word.buf));
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

static Err interp_parse_until(
  Interp *interp, U8 delim, const char **out_buf, Ind *out_len
) {
  const auto read = interp->reader;
  read_skip_space(read);
  try(read_until(read, (U8)delim));

  // IF_DEBUG(eprintf(
  //   "[system] parsed until delim " FMT_CHAR "; length: " FMT_UINT
  //   "; string: %s\n",
  //   (int)delim,
  //   read->buf.len,
  //   read->buf.buf
  // ));

  if (out_buf) *out_buf = read->buf.buf;
  if (out_len) *out_len = read->buf.len;

  // IF_DEBUG({
  //   const auto ints = &interp->ints;
  //   eprintf(
  //     "[system] stack_len(ints) after parsing: " FMT_SINT "\n", stack_len(ints)
  //   );
  //   eprintf("[system] stack pointer after parsing: %p\n", ints->top);
  // });
  return nullptr;
}

static Err find_extern(const char *name, void **out) {
  const auto addr = dlsym(RTLD_DEFAULT, name);

  if (!addr) {
    return errf(
      "unable to find extern symbol " FMT_QUOTED "; error: %s", name, dlerror()
    );
  }

  IF_DEBUG(eprintf(
    "[system] found address of extern symbol " FMT_QUOTED ": %p\n", name, addr
  ));

  if (out) *out = addr;
  return nullptr;
}

/*
Used for accessing external variables such as `stdout`. The resulting address
points to an entry in the GOT (global offset table), and must be used with
intrinsics `intrin_comp_page_addr` or `intrin_comp_page_load` which compile
instructions for accessing that entry.
*/
static Err interp_extern_got(
  Interp *interp, const char *name, Ind len, const U64 **out_addr
) {
  IF_DEBUG(aver((Sint)strlen(name) == len));

  const auto comp     = &interp->comp;
  auto       got_addr = comp_find_dysym(comp, name);

  if (!got_addr) {
    void *ext_addr;
    try(find_extern(name, &ext_addr));
    got_addr = comp_register_dysym(comp, name, (U64)ext_addr);
    IF_DEBUG(aver(*got_addr == (Uint)ext_addr));
  }

  if (out_addr) *out_addr = got_addr;
  return nullptr;
}

static Err interp_read_extern(Interp *interp, void **out) {
  try(interp_read_word(interp));
  const auto name = interp->reader->word.buf;
  try(find_extern(name, out));
  return nullptr;
}

static Err interp_extern_proc(Interp *interp, Sint inp_len, Sint out_len) {
  void *addr;
  try(interp_read_extern(interp, &addr));

  if (inp_len < 0) {
    return err_str("negative input parameter count");
  }
  if (inp_len > ASM_INP_PARAM_REG_LEN) {
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
      .type     = SYM_EXTERN,
      .name     = interp->reader->word,
      .wordlist = WORDLIST_EXEC,
      .exter    = addr,
      .inp_len  = (U8)inp_len,
      .out_len  = (U8)out_len,
      .clobber  = ASM_VOLATILE_REGS, // Only used in reg-based call-conv.
    }
  );

  dict_set(&interp->dict_exec, sym->name.buf, sym);
  comp_register_dysym(&interp->comp, sym->name.buf, (U64)addr);
  return nullptr;
}

static Err interp_find_word(
  Interp *interp, const char *name, Ind len, Wordlist wordlist, const Sym **out
) {
  IF_DEBUG(aver((Sint)strlen(name) == len));

  Sym_dict *dict;
  try(interp_wordlist(interp, wordlist, &dict));

  const auto sym = dict_get(dict, name);
  if (!sym) return err_word_undefined(name);

  IF_DEBUG(eprintf(
    "[system] found " FMT_QUOTED " in wordlist %d; address: %p\n",
    name,
    wordlist,
    sym
  ));

  if (out) *out = sym;
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

static void debug_mem_at(const Uint *adr) {
  eprintf("[debug] memory at address %p:\n", adr);
  for (Ind ind = 0; ind < 8; ind++) {
    const auto ptr = adr + ind;
    eprintf(
      "[debug]   %p -- " FMT_UINT_HEX " (" FMT_SINT ")\n", ptr, *ptr, (Sint)*ptr
    );
  }
}
