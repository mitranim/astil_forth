/*
Intrinsic procedures necessary for "bootstrapping" the Forth system.
Should be kept to a minimum. Non-fundamental words should be written
in Forth; where regular language is insufficient, we use inline asm.

Intrinsics don't even provide conditionals or arithmetic.

This file contains "generic" intrinsics which don't care about the calling
convention. Intrinsics which do are located in dedicated files.
*/
#pragma once
#include "./c_comp.c"
#include "./c_interp_internal.c"

static Err err_nested_definition(const Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  return errf("unexpected \":\" in definition of " FMT_QUOTED, sym->name.buf);
}

static Err err_wordlist_at_capacity(const char *name) {
  return errf("unable to create word " FMT_QUOTED ": wordlist at capacity", name);
}

static Err interp_word_begin(Interp *interp, Wordlist wordlist, Word_str name) {
  IF_DEBUG(eprintf(
    "[system] starting definition of " FMT_QUOTED
    " associated with wordlist %d\n",
    name.buf,
    wordlist
  ));

  const auto syms = &interp->syms;
  if (stack_rem(syms) <= 0) return err_wordlist_at_capacity(name.buf);

  /*
  Add to the symbol list, but not to the symbol dict.
  During the colon, the new word is not yet visible.
  */
  const auto sym = stack_push(
    syms,
    (Sym){
      .type     = SYM_NORM,
      .name     = name,
      .wordlist = wordlist,
    }
  );

  comp_sym_beg(&interp->comp, sym);
  return nullptr;
}

static Err interp_begin_definition(Interp *interp) {
  if (interp->comp.ctx.sym) return err_nested_definition(interp);
  try(interp_read_word(interp));
  return nullptr;
}

static void interp_repr_sym(const Interp *, const Sym *);

static Err intrin_semicolon(Interp *interp) {
  const auto comp  = &interp->comp;
  const auto redef = interp->comp.ctx.redefining; // Snapshot before clearing.

#ifdef NATIVE_CALL_CONV
  try(comp_validate_ret_args(comp));
#endif

  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  comp_sym_end(comp, sym);
  interp_snapshot(interp);

  const auto name     = sym->name.buf;
  const auto wordlist = sym->wordlist;

  Sym_dict *dict;
  try(interp_wordlist(interp, wordlist, &dict));

  if (dict_has(dict, name) && !redef) {
    eprintf(
      "[system] redefined word " FMT_QUOTED " in wordlist %d\n", name, wordlist
    );
  }
  dict_set(dict, name, sym);

  IF_DEBUG({
    eputs("[debug] compiled:");
    interp_repr_sym(interp, sym);

    // const auto read = interp->reader;
    // eprintf(
    //   "[system] [reader] position " FMT_UINT "; %s:" FMT_UINT ":" FMT_UINT "\n",
    //   READ_POS_ARGS(read)
    // );
  });

  // const auto ints = &interp->ints;
  // if (stack_len(ints)) {
  //   eprintf(
  //     "[system] warning: " FMT_SINT " stack items after defining " FMT_QUOTED
  //     "; this usually indicates a bug\n",
  //     stack_len(ints),
  //     name
  //   );
  // }
  return nullptr;
}

static void intrin_bracket_beg(Interp *interp) {
  interp->comp.ctx.compiling = false;
}

static void intrin_bracket_end(Interp *interp) {
  interp->comp.ctx.compiling = true;
}

static Err intrin_comp_only(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  sym->comp_only = true;
  return nullptr;
}

static Err intrin_not_comp_only(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  sym->comp_only = false;
  return nullptr;
}

static Err intrin_inline(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  sym->norm.inlinable = true;
  return nullptr;
}

static Err intrin_throws(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  sym->throws = true;
  return nullptr;
}

static Err intrin_redefine(Interp *interp) {
  try(interp_require_current_sym(interp, nullptr));
  interp->comp.ctx.redefining = true;
  return nullptr;
}

/*
Caution: unlike in other Forth systems, `here` is not an executable address.
Executable code is copied to a different memory page when finalizing a word.
*/
static void intrin_here(Interp *interp) {
  stack_push(
    &interp->ints, (Sint)(comp_code_next_writable_instr(&interp->comp.code))
  );
}

static Err intrin_quit(Interp *interp) {
  const auto comp = &interp->comp;
  if (comp->ctx.sym) {
    return err_str("can't quit while defining a word");
  }
  return ERR_QUIT;
}

static Err err_char_eof() {
  return err_str("EOF where a character was expected");
}

static Err intrin_char(Interp *interp) {
  const auto read = interp->reader;
  U8         byte;

  try(read_ascii_printable(read, &byte));
  if (!byte) return err_char_eof();
  try(int_stack_push(&interp->ints, byte));
  return nullptr;
}

// Technically not fundamental. TODO implement in Forth via intrinsic `char`.
static Err intrin_parse_word(Interp *interp) {
  try(interp_read_word(interp));

  const auto word = &interp->reader->word;
  const auto ints = &interp->ints;

  try(int_stack_push(ints, (Sint)word->buf));
  try(int_stack_push(ints, (Sint)word->len));
  return nullptr;
}

static Err intrin_import_quote(Interp *interp) {
  const auto read = interp->reader;
  read_skip_whitespace(read);
  try(read_until(read, '"'));
  try(interp_import(interp, (const char *)read->buf.buf));
  return nullptr;
}

static Err intrin_import_tick(Interp *interp) {
  const auto read = interp->reader;
  try(read_word(read));
  try(interp_import(interp, (const char *)read->word.buf));
  return nullptr;
}

static void intrin_debug_on(Interp *) { DEBUG = true; }

static void intrin_debug_off(Interp *) { DEBUG = false; }

static void debug_flush(void *) {
  fflush(stdout);
  fflush(stderr);
}

static Err debug_throw() { return "debug_throw"; }

static void debug_stack_len(Interp *interp) {
  const auto ints = &interp->ints;
  const auto len  = stack_len(ints);
  if (!len) {
    eprintf("[debug] stack is empty (%p)\n", ints->floor);
    return;
  }
  eprintf("[debug] stack length (%p): " FMT_SINT "\n", ints->floor, len);
}

static void debug_stack(Interp *interp) {
  const auto ints = &interp->ints;
  const auto len  = stack_len(ints);

  if (!len) {
    eprintf("[debug] stack is empty (%p)\n", ints->floor);
    return;
  }

  eprintf("[debug] stack (%p) (" FMT_SINT "):\n", ints->floor, len);

  for (stack_range(auto, ptr, ints)) {
    eprintf(
      "[debug]   %p -- " FMT_SINT " " FMT_UINT_HEX "\n", ptr, *ptr, (Uint)*ptr
    );
  }
}

static void debug_depth(Interp *interp) {
  eprintf("[debug] stack depth: " FMT_SINT "\n", stack_len(&interp->ints));
}

static void debug_top_int(Interp *interp) {
  const auto val = stack_head(&interp->ints);
  eprintf(
    "[debug] stack top int: " FMT_SINT " " FMT_UINT_HEX " %s\n",
    val,
    (Uint)val,
    uint64_to_bit_str((Uint)val)
  );
}

static void debug_top_ptr(Interp *interp) {
  const auto ints = &interp->ints;

  if (stack_len(ints)) {
    eprintf("[debug] stack top ptr: %p\n", (void *)stack_head(ints));
  }
  else {
    eputs("[debug] stack top ptr: none (empty)");
  }
}

static void debug_top_str(Interp *interp) {
  eprintf(
    "[debug] stack top string: %s\n", (const char *)stack_head(&interp->ints)
  );
}

static Err debug_mem(Interp *interp) {
  Sint val;
  try(int_stack_pop(&interp->ints, &val));

  auto ptr = (const Uint *)val;
  eprintf("[debug] memory at address %p:\n", ptr);
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 0, *(ptr + 0));
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 1, *(ptr + 1));
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 2, *(ptr + 2));
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 3, *(ptr + 3));
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 4, *(ptr + 4));
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 5, *(ptr + 5));
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 6, *(ptr + 6));
  eprintf("[debug]   %p -- " FMT_UINT_HEX "\n", ptr + 7, *(ptr + 7));
  return nullptr;
}

static void interp_repr_sym(const Interp *interp, const Sym *sym) {
  switch (sym->type) {
    case SYM_NORM: {
      fprintf(
        stderr,
        "[debug] word:\n"
        "[debug]   name:            %s\n"
        "[debug]   wordlist:        %d\n"
        "[debug]   type:            normal\n"
#ifdef NATIVE_CALL_CONV
        "[debug]   inp_len:         %d\n"
        "[debug]   out_len:         %d\n"
        "[debug]   clobber:         0b%s\n"
#endif
        "[debug]   throws:          %s\n"
        "[debug]   comp_only:       %s\n"
        "[debug]   interp_only:     %s\n"
        "[debug]   inlinable:       %s\n"
        "[debug]   execution token: %p\n",
        sym->name.buf,
        sym->wordlist,
#ifdef NATIVE_CALL_CONV
        sym->inp_len,
        sym->out_len,
        uint32_to_bit_str((U32)sym->clobber),
#endif
        fmt_bool(sym->throws),
        fmt_bool(sym->comp_only),
        fmt_bool(sym->interp_only),
        fmt_bool(sym->norm.inlinable),
        sym
      );
      comp_debug_print_sym_instrs(&interp->comp, sym, "[debug]   ");
      IF_DEBUG({
        eprintf("[debug]   symbol: ");
        repr_struct(sym);
      });
      return;
    }

    case SYM_INTRIN: {
      eprintf(
        "[debug] word:\n"
        "[debug]   name:               %s\n"
        "[debug]   wordlist:           %d\n"
        "[debug]   type:               intrinsic\n"
        "[debug]   inp_len:            %d\n"
        "[debug]   out_len:            %d\n"
        "[debug]   throws:             %d\n"
        "[debug]   execution token:    %p\n"
        "[debug]   executable address: %p\n",
        sym->name.buf,
        sym->wordlist,
        sym->inp_len,
        sym->out_len,
        sym->throws,
        sym,
        sym->intrin
      );
      return;
    }

    case SYM_EXTERN: {
      eprintf(
        "[debug] word:\n"
        "[debug]   name:     %s\n"
        "[debug]   wordlist: %d\n"
        "[debug]   type:     external procedure\n"
        "[debug]   address:  %p\n",
        sym->name.buf,
        sym->wordlist,
        sym->exter
      );
      return;
    }

    default: unreachable();
  }
}

static Err debug_word(Interp *interp) {
  try(interp_read_word(interp));
  const auto name = interp->reader->word.buf;

  const auto exec = dict_get(&interp->dict_exec, name);
  if (exec) interp_repr_sym(interp, exec);

  const auto comp = dict_get(&interp->dict_comp, name);
  if (comp) interp_repr_sym(interp, comp);

  if (exec || comp) return nullptr;
  return errf("word " FMT_QUOTED " is not present in any wordlist\n", name);
}

static Err debug_sync_code(Interp *interp) {
  try(comp_code_sync(&interp->comp.code));
  eprintf("[debug] synced code heaps\n");
  return nullptr;
}

#ifdef NATIVE_CALL_CONV
#include "./c_intrin_cc_reg.c"
#else
#include "./c_intrin_cc_stack.c"
#endif

/*
Note: ALL intrinsics must have certain fields set the same way:

  .type        = SYM_INTRIN
  .name.len    = strlen(.name.buf)
  .clobber     = ARCH_VOLATILE_REGS
  .interp_only = true

Repetition is error-prone, so we set these fields in `sym_init_intrin`.

Some fields are used only in the register-based callvention:

  inp_len
  out_len
  clobber
*/

static constexpr auto INTRIN_COLON = (Sym){
  .name.buf = ":",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_colon,
  .throws   = true,
};

static constexpr auto INTRIN_COLON_COLON = (Sym){
  .name.buf = "::",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_colon_colon,
  .throws   = true,
};

static constexpr auto INTRIN_SEMICOLON = (Sym){
  .name.buf  = ";",
  .wordlist  = WORDLIST_COMP,
  .intrin    = (void *)intrin_semicolon,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_BRACKET_BEG = (Sym){
  .name.buf  = "[",
  .wordlist  = WORDLIST_COMP,
  .intrin    = (void *)intrin_bracket_beg,
  .comp_only = true,
};

static constexpr auto INTRIN_BRACKET_END = (Sym){
  .name.buf = "]",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_bracket_end,
};

static constexpr auto INTRIN_RET = (Sym){
  .name.buf  = "#ret",
  .wordlist  = WORDLIST_COMP,
  .intrin    = (void *)intrin_ret,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_RECUR = (Sym){
  .name.buf  = "#recur",
  .wordlist  = WORDLIST_COMP,
  .intrin    = (void *)intrin_recur,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_COMP_ONLY = (Sym){
  .name.buf  = "comp_only",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_only,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_NOT_COMP_ONLY = (Sym){
  .name.buf  = "not_comp_only",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_not_comp_only,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_INLINE = (Sym){
  .name.buf  = "inline",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_inline,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_THROWS = (Sym){
  .name.buf  = "throws",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_throws,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_REDEFINE = (Sym){
  .name.buf  = "redefine",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_redefine,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_HERE = (Sym){
  .name.buf  = "here",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_here,
  .comp_only = true,
};

static constexpr auto INTRIN_COMP_INSTR = (Sym){
  .name.buf    = "comp_instr",
  .wordlist    = WORDLIST_EXEC,
  .intrin      = (void *)intrin_comp_instr,
  .inp_len     = 1,
  .throws      = true,
  .comp_only   = true,
  .interp_only = true,
};

static constexpr auto INTRIN_COMP_LOAD = (Sym){
  .name.buf  = "comp_load",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_load,
  .inp_len   = 2,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_ALLOC_DATA = (Sym){
  .name.buf = "alloc_data",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_alloc_data,
  .inp_len  = 2, // ( buf len -- addr ) ; buffer address may be nil
  .out_len  = 1,
  .throws   = true,
};

static constexpr auto INTRIN_COMP_PAGE_ADDR = (Sym){
  .name.buf  = "comp_page_addr",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_page_addr,
  .inp_len   = 2, // ( adr reg -- )
  .out_len   = 0,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_COMP_PAGE_LOAD = (Sym){
  .name.buf  = "comp_page_load",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_page_load,
  .inp_len   = 2, // ( adr reg -- )
  .out_len   = 0,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_COMP_CALL = (Sym){
  .name.buf  = "comp_call",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_call,
  .inp_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_QUIT = (Sym){
  .name.buf = "quit",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_quit,
  .throws   = true,
};

static constexpr auto INTRIN_CHAR = (Sym){
  .name.buf = "char",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_char,
  .throws   = true,
};

static constexpr auto INTRIN_PARSE = (Sym){
  .name.buf = "parse",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_parse,
  .inp_len  = 1,
  .out_len  = 2,
  .throws   = true,
};

static constexpr auto INTRIN_PARSE_WORD = (Sym){
  .name.buf = "parse_word",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_parse_word,
  .out_len  = 2,
  .throws   = true,
};

static constexpr auto INTRIN_IMPORT = (Sym){
  .name.buf = "import",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_import,
  .inp_len  = 2,
  .throws   = true,
};

static constexpr auto INTRIN_IMPORT_QUOTE = (Sym){
  .name.buf = "import\"",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_import_quote,
  .throws   = true,
};

static constexpr auto INTRIN_IMPORT_TICK = (Sym){
  .name.buf = "import'",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_import_tick,
  .throws   = true,
};

static constexpr auto INTRIN_EXTERN_GOT = (Sym){
  .name.buf = "extern_got",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_extern_got,
  .inp_len  = 2,
  .out_len  = 1,
  .throws   = true,
};

static constexpr auto INTRIN_EXTERN_PROC = (Sym){
  .name.buf = "extern:",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_extern_proc,
  .inp_len  = 2,
  .throws   = true,
};

static constexpr auto INTRIN_FIND_WORD = (Sym){
  .name.buf = "find_word",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_find_word,
  .inp_len  = 3,
  .out_len  = 1,
  .throws   = true,
};

static constexpr auto INTRIN_INLINE_WORD = (Sym){
  .name.buf  = "inline_word",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_inline_word,
  .inp_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_EXECUTE = (Sym){
  .name.buf = "execute",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_execute,
  .inp_len  = 1,
  .throws   = true,
};

static constexpr auto INTRIN_GET_LOCAL = (Sym){
  .name.buf  = "get_local",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_get_local,
  .inp_len   = 2,
  .out_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_ANON_LOCAL = (Sym){
  .name.buf  = "anon_local",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_anon_local,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_DEBUG_ON = (Sym){
  .name.buf = "debug_on",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_debug_on,
};

static constexpr auto INTRIN_DEBUG_OFF = (Sym){
  .name.buf = "debug_off",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_debug_off,
};

static constexpr auto INTRIN_DEBUG_FLUSH = (Sym){
  .name.buf = "debug_flush",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_flush,
};

static constexpr auto INTRIN_DEBUG_THROW = (Sym){
  .name.buf = "debug_throw",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_throw,
  .throws   = true,
};

static constexpr auto INTRIN_DEBUG_STACK_LEN = (Sym){
  .name.buf = "debug_stack_len",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_stack_len,
};

static constexpr auto INTRIN_DEBUG_STACK = (Sym){
  .name.buf = "debug_stack",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_stack,
};

static constexpr auto INTRIN_DEBUG_DEPTH = (Sym){
  .name.buf = "debug_depth",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_depth,
};

static constexpr auto INTRIN_DEBUG_TOP_INT = (Sym){
  .name.buf = "debug_top_int",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_top_int,
};

static constexpr auto INTRIN_DEBUG_TOP_PTR = (Sym){
  .name.buf = "debug_top_ptr",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_top_ptr,
};

static constexpr auto INTRIN_DEBUG_TOP_STR = (Sym){
  .name.buf = "debug_top_str",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_top_str,
};

static constexpr auto INTRIN_DEBUG_MEM = (Sym){
  .name.buf = "debug_mem",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_mem,
  .throws   = true,
};

static constexpr auto INTRIN_DEBUG_WORD = (Sym){
  .name.buf = "debug'",
  .wordlist = WORDLIST_COMP,
  .intrin   = (void *)debug_word,
  .throws   = true,
};

static constexpr auto INTRIN_DEBUG_SYNC_CODE = (Sym){
  .name.buf = "debug_sync_code",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_sync_code,
  .throws   = true,
};

#ifdef NATIVE_CALL_CONV
#include "./c_intrin_list_cc_reg.c" // IWYU pragma: export
#else
#include "./c_intrin_list_cc_stack.c" // IWYU pragma: export
#endif
