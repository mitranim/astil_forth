// Various internal functions used by interpreter intrinsics.
#pragma once
#include "../clib/cli.h"
#include "../clib/io.c"
#include "../clib/path.c"
#include "./comp.c"
#include "./read.c"
#include "./sym.c"
#include <dlfcn.h>
#include <stdio.h>

static Err comp_snapshot(const Comp *prev, Comp *next) {
  try(comp_ctx_snapshot(&prev->ctx, &next->ctx));
  next->code = prev->code;
  return nullptr;
}

static Err interp_snapshot(Interp *interp) {
  try(comp_snapshot(&interp->comp, &interp->snap.comp));
  interp->snap.ints = interp->ints;
  interp->snap.syms = interp->syms;
  return nullptr;
}

/*
Restores the interpreter to a known consistent state.
Should be used when handling REPL and import errors.

TODO: drop currently defined symbol from list and dict.
Our dicts don't support deletion at the moment.
*/
static Err interp_rewind(Interp *interp) {
  const auto prev = &interp->snap;
  const auto sym  = interp->comp.ctx.sym;

  comp_rewind(&prev->comp, &interp->comp);
  stack_rewind(&prev->ints, &interp->ints);
  stack_rewind(&prev->syms, &interp->syms);

  /*
  TODO: support deletion in dicts. For now, accessing a partially
  defined word after recovering from an error is UB.

    const auto len_next = (Ind)stack_len(syms);
    const auto words = &interp->words;
    for (Ind ind = len_prev; ind < len_next; ind++) {
      const auto sym = &syms->floor[ind];
      dict_del(words, sym->buf.name);
    }
  */

  if (!sym) {
    IF_DEBUG(eprintf("[debug] rewound interpreter state\n"));
    return nullptr;
  }

  IF_DEBUG(eprintf(
    "[debug] rewound interpreter state and exited the definition of " FMT_QUOTED
    "\n",
    sym->name.buf
  ));

  return nullptr;
}

static Reader *interp_reader(Interp *interp) {
  return interp && interp->module ? &interp->module->reader : nullptr;
}

static const Reader *interp_reader_const(const Interp *interp) {
  return interp && interp->module ? &interp->module->reader : nullptr;
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

---

We can afford slightly more informative error messages.

Truncating the integer stack is technically optional
but it makes REPL behavior less weird. For example:

  10 20 30 : word ; stack_clear .

`;` snapshots `10 20 30`. Without the truncation, when `.` underflows,
rewind resets the (empty) stack to `10 20 30` which feels kinda weird.
*/
static Err interp_on_tty_err(Interp *interp, Err err) {
  eprintf(TTY_RED_BEG "error:" TTY_RED_END " %s\n", err);
  if (TRACE) backtrace_print();
  try(interp_rewind(interp));
  stack_trunc(&interp->ints);
  return nullptr;
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

static Err interp_validate_redefinition(
  Interp *interp, const char *name, Wordlist wordlist, bool redefining
) {
  const auto module = interp->module;
  if (module ? module->slop : interp->slop) return nullptr;

  Sym_dict *dict;
  try(interp_wordlist(interp, wordlist, &dict));

  const auto had = dict_has(dict, name);
  if (had == redefining) return nullptr;

  const auto read = interp_reader_const(interp);
  if (read && read->tty) {
    if (had) {
      eprintf(
        "[system] redefined word " FMT_QUOTED " in wordlist %d (%s)\n",
        name,
        wordlist,
        wordlist_name(wordlist)
      );
    }
    return nullptr;
  }

  if (had) {
    return errf(
      "word " FMT_QUOTED
      " already defined in wordlist %d (%s); hint: use `[ redefine ]` to replace it",
      name,
      wordlist,
      wordlist_name(wordlist)
    );
  }

  return errf(
    "redundant `redefine` marker in word " FMT_QUOTED
    " which is not previously defined in wordlist %d (%s)",
    name,
    wordlist,
    wordlist_name(wordlist)
  );
}

static Err read_interp_num(Interp *interp) {
  Sint num;
  try(read_num(interp_reader(interp), &num));

  IF_DEBUG(eprintf("[system] read number: " FMT_SINT "\n", num));

  const auto comp = &interp->comp;

  if (comp->ctx.compiling) {
    return comp_append_push_imm(comp, num);
  }

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
    READ_POS_ARGS(interp_reader(interp))
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
    "; stack pointer after call: %p; error: %p\n",
    sym->name.buf,
    interp->ints.top,
    err
  ));
  return err;
}

static Err interp_call_extern(Interp *interp, const Sym *sym) {
  IF_DEBUG(eprintf(
    "[system] calling external function " FMT_QUOTED
    " at address %p; inp_len: %d; out_len: %d\n",
    sym->name.buf,
    sym->exter,
    sym->inp_len,
    sym->out_len
  ));

  const auto err = asm_call_extern(&interp->ints, sym);

  IF_DEBUG(eprintf(
    "[system] done called extern function " FMT_QUOTED "\n", sym->name.buf
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
  return errf(
    "word " FMT_QUOTED " not found in wordlist %d (%s)",
    name,
    list,
    wordlist_name(list)
  );
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
    if (loc) return comp_append_push_from_local(comp, loc);

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
  const auto read = interp_reader(interp);
  Word_str   word;
  try(read_valid_word(read, &word));

  IF_DEBUG(eprintf("[system] read word: " FMT_QUOTED "\n", word.buf));

  return interp_word(interp, word);
}

static const Err ERR_QUIT = "quit";
static const Err ERR_RET  = "ret";

static Err interp_err(Reader *read, const Err err) {
  if (err == ERR_QUIT) return err;
  if (err == ERR_RET) return nullptr;
  return reader_err(read, err);
}

static Err interp_loop(Interp *interp) {
  const auto read = interp_reader(interp);

  while (reader_has_more(read)) {
    const auto next = read_char_at(read, read->pos);
    try(validate_ascii_printable(next));

    switch (HEAD_CHAR_KIND[next]) {
      case CHAR_EOF: return nullptr;

      case CHAR_WHITESPACE: {
        read->pos++;
        continue;
      }

      case CHAR_DECIMAL: {
        try(read_interp_num(interp));
        continue;
      }

      // `+` and `-` are ambiguous: may begin a number or a word.
      case CHAR_ARITH: {
        if (HEAD_CHAR_KIND[read_char_at(read, read->pos + 1)] == CHAR_DECIMAL) {
          try(read_interp_num(interp));
          continue;
        }

        try(read_interp_word(interp));
        continue;
      }

      case CHAR_WORD: {
        try(read_interp_word(interp));
        continue;
      }

      case CHAR_UNPRINTABLE: [[fallthrough]];
      default:               return err_unsupported_char(next);
    }
  }
  return nullptr;
}

static void interp_welcome(Interp *interp) {
  if (interp->welcomed) return;

  const auto name = "words";

  puts("Running Astil Forth REPL.");

  if (dict_has(&interp->dict_exec, name) || dict_has(&interp->dict_comp, name)) {
    printf("Type " FMT_QUOTED " to list the available words.\n", name);
  }

  puts(
    "Type `debug' <word>` to view its details.\n"
    "Type `dis' <word>` to disassemble a word.\n"
    "(Only regular words; requires `llvm-mc`.)\n"
  );

  interp->welcomed = true;
}

#ifdef __APPLE__
#include <limits.h>
#include <mach-o/dyld.h>

static const char *get_exec_path() {
  static thread_local char path[PATH_MAX];
  uint32_t                 size = arr_cap(path);
  if (_NSGetExecutablePath(path, &size)) return nullptr;
  return path;
}

#else  // __APPLE__
static const char *get_exec_path() { return nullptr; }
#endif // __APPLE__

static Err interp_eval(Interp *interp, const char *src) {
  const auto len  = (Ind)strlen(src);
  Module_ctx next = {};
  next.reader     = (Reader){.src = src, .len = len, .path = "<eval>"};

  Module_ctx *prev     = interp->module;
  next.slop            = prev ? prev->slop : interp->slop;
  interp->module       = &next;
  defer interp->module = prev;

  return interp_err(&next.reader, interp_loop(interp));
}

static Err interp_import_stdin(Interp *interp) {
  const auto path = "/dev/stdin";
  const auto file = stdin;
  const auto read = interp_reader(interp);
  const auto fdes = STDIN_FILENO;
  const auto tty  = isatty(fdes);

  if (tty) interp_welcome(interp);

  IF_DEBUG(eprintf("[system] reading code from stdio: " FMT_QUOTED "\n", path));

  if (!tty) {
    deferred(chars_deinit) char *src = nullptr;
    Uint                         len;
    try(file_stream_read_text(path, file, &src, &len));
    if (!len) return nullptr;

    *read = (Reader){.src = src, .len = (Ind)len, .path = path, .tty = tty};
    return interp_err(read, interp_loop(interp));
  }

  for (;;) {
    deferred(chars_deinit) char *src = nullptr;
    Uint                         len;
    try(fd_read_available_text(path, fdes, &src, &len));
    if (!len) return nullptr;

    *read = (Reader){.src = src, .len = (Ind)len, .path = path, .tty = tty};
    const auto err = interp_err(read, interp_loop(interp));

    if (!err) continue;
    try(interp_on_tty_err(interp, err));
  }

  return nullptr;
}

static char *resolve_local_import_path(const char *prev, const char *next) {
  deferred(chars_deinit) char *path = path_join(prev, next, false);
  return realpath(path, nullptr);
}

static char *resolve_std_import_path(const char *next) {
  // Try relative to the interpreter executable.
  const auto exec = get_exec_path();
  if (exec && is_path_abs(exec)) {
    deferred(chars_deinit) char *base = path_join(exec, "forth", false);
    deferred(chars_deinit) char *full = path_join(base, next, true);
    const auto                   path = realpath(full, nullptr);
    if (path) return path;
  }

  // Try in `~/.local/share/astil`.
  // SYNC[install_path].
  const auto home = getenv("HOME");
  if (home) {
    deferred(chars_deinit) char *base = path_join(
      home, ".local/share/astil", true
    );
    deferred(chars_deinit) char *full = path_join(base, next, false);
    const auto                   path = realpath(full, nullptr);
    if (path) return path;
  }

  return nullptr;
}

static char *resolve_import_path(const char *prev, const char *next) {
  if (is_path_implicit_rel(next)) return resolve_std_import_path(next);
  return resolve_local_import_path(prev, next);
}

static Err interp_import_inner(
  Interp *interp, bool prev_tty, const char *prev, const char *next
) {
  if (prev_tty) prev = nullptr;

  deferred(chars_deinit) char *path = resolve_import_path(prev, next);

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
    return nullptr;
  }

  deferred(chars_deinit) char *src = nullptr;
  Uint                         len;
  try(file_read_text(path, &src, &len));

  const auto read = interp_reader(interp);

  *read = (Reader){.src = src, .len = (Ind)len, .path = path};

  // Registering the import before interpreting the file
  // enables partial cyclic imports and reduces surprise.
  dict_set(imports, strdup(path), EMPTY); // The dict owns the key copy.

  IF_DEBUG(eprintf("[system] importing file: " FMT_QUOTED "\n", path));
  try(interp_err(read, interp_loop(interp)));
  IF_DEBUG(eprintf("[system] done importing file: " FMT_QUOTED "\n", path));
  return nullptr;
}

// There better not be a `longjmp` over this.
static Err interp_import(Interp *interp, const char *path) {
  try(interp_snapshot(interp));

  const auto prev      = interp->module;
  const auto prev_path = prev ? prev->reader.path : nullptr;
  const auto prev_tty  = prev && prev->reader.tty;
  Module_ctx next      = {};
  next.slop            = prev ? prev->slop : interp->slop;
  interp->module       = &next;
  defer interp->module = prev;

  const auto err = is_path_stdin(path)
    ? interp_import_stdin(interp)
    : interp_import_inner(interp, prev_tty, prev_path, path);

  if (err) (void)interp_rewind(interp);
  return err;
}

static Err interp_read_word(Interp *interp, Word_str *out) {
  const auto read = interp_reader(interp);
  try(read_valid_word(read, out));
  IF_DEBUG(eprintf("[system] read word: " FMT_QUOTED "\n", out->buf));
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

static Err interp_sym_by_ptr(Interp *interp, Sint ptr, Sym **out) {
  const auto sym = (Sym *)ptr;
  try(interp_validate_sym_ptr(interp, sym));

  IF_DEBUG(eprintf(
    "[system] validated address %p of symbol " FMT_QUOTED "\n", sym, sym->name.buf
  ));

  if (out) *out = sym;
  return nullptr;
}

static Err interp_parse_until(
  Interp *interp, U8 delim, const char **out_buf, Ind *out_len
) {
  const auto read = interp_reader(interp);
  Ind        len;
  try(read_until_char(read, (U8)delim, out_buf, &len));

  // IF_DEBUG(eprintf(
  //   "[system] parsed until delim " FMT_CHAR_QUOTED "; length: " FMT_UINT "\n",
  //   (int)delim,
  //   len
  // ));

  if (out_len) *out_len = len;

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

static Err interp_extern_adr(Interp *interp, const char *name) {
  const auto comp    = &interp->comp;
  const auto syms    = &comp->code.externs;
  auto       ext_adr = comp_find_extern(comp, name);

  if (!ext_adr) {
    try(find_extern(name, &ext_adr));
    comp_register_dysym(syms, name, (U64)ext_adr);
  }
  return nullptr;
}

static Err interp_extern_fun(
  Interp *interp, const char *name, Sint inp_len, Sint out_len
) {
  void *ext_adr;
  try(find_extern(name, &ext_adr));

  if (inp_len < 0) {
    return errf(FMT_QUOTED ": negative input parameter count", name);
  }
  if (inp_len > ASM_INP_PARAM_REG_LEN) {
    return errf(FMT_QUOTED ": too many input parameters", name);
  }
  if (out_len < 0) {
    return errf(FMT_QUOTED ": negative output parameter count", name);
  }
  if (out_len > 1) {
    return errf(FMT_QUOTED ": too many output parameters", name);
  }

  const auto wordlist = WORDLIST_EXEC;
  Word_str   word;
  try(valid_word(name, (Ind)strlen(name), &word));
  const auto redef = interp->comp.ctx.redefining;
  try(interp_validate_redefinition(interp, word.buf, wordlist, redef));

  const auto sym = stack_push(
    &interp->syms,
    (Sym){
      .type     = SYM_EXTERN,
      .name     = word,
      .wordlist = wordlist,
      .exter    = ext_adr,
      .inp_len  = (U8)inp_len,
      .out_len  = (U8)out_len,
      .clobber  = ASM_REGS_VOLATILE, // Only used in reg-based call-conv.
    }
  );

  const auto dict = &interp->dict_exec;
  const auto comp = &interp->comp;
  const auto syms = &comp->code.externs;

  dict_set(dict, sym->name.buf, sym);
  comp_register_dysym(syms, sym->name.buf, (U64)ext_adr);

  return nullptr;
}

static Err interp_find_word(
  Interp *interp, const char *name, Wordlist wordlist, const Sym **out
) {
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

static Err interp_validate_data_ptr(Sint val) {
  /*
  Some systems deliberately ensure that virtual memory addresses
  are just out of range of `int32`. Would be nice to check if the
  pointer is somewhere in that range, but the assumption would be
  really non-portable.

  In a Forth implementation for OS-free microchips with a very small
  amount of real memory, valid data pointers could begin close to 0,
  depending on how data is organized. Since our implementation only
  supports 64-bit machines and runs inside an OS, 0-adjacent data
  addresses aren't going to happen; we can assume addresses which
  are "decently large".
  */
  if (val > (1u << 14u)) return nullptr;
  return errf("suspiciously invalid-looking data pointer: %p", (U8 *)val);
}

static Err interp_validate_data_len(Sint val) {
  if (val > 0 && val < (Sint)IND_MAX) return nullptr;
  return errf("invalid data length: " FMT_SINT, val);
}

static Err interp_validate_buf_len(Sint buf, Sint len) {
  try(interp_validate_data_ptr(buf));
  try(interp_validate_data_len(len));
  return nullptr;
}
