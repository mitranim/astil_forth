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

static Err intrin_colon(Interp *interp) {
  const auto comp = &interp->comp;
  if (comp->ctx.sym) return err_nested_definition(interp);

  const auto read = interp->reader;
  try(read_word(read));

  const auto name = read->word;
  IF_DEBUG(eprintf("[system] read word name: " FMT_QUOTED "\n", name.buf));

  const auto syms = &interp->syms;
  if (stack_rem(syms) <= 0) return err_wordlist_at_capacity(name.buf);

  /*
  Add to the symbol list, but not to the symbol dict.
  During the colon, the new word is not yet visible.
  */
  const auto sym = stack_push(
    syms,
    (Sym){
      .type      = SYM_NORM,
      .name      = name,
      .immediate = is_name_immediate(&name),
    }
  );

  comp_sym_beg(comp, sym);
  return nullptr;
}

static Err intrin_semicolon(Interp *interp) {
  const auto comp = &interp->comp;

#ifdef NATIVE_CALL_ABI
  try(comp_validate_ret_args(comp));
#endif

  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  comp_sym_end(comp, sym);
  interp_snapshot(interp);

  const auto words = &interp->words;
  if (dict_has(words, sym->name.buf) && !interp->comp.ctx.redefining) {
    eprintf("[system] redefined word " FMT_QUOTED "\n", sym->name.buf);
  }
  dict_set(words, sym->name.buf, sym);

  /*
  Prints instructions as hexpairs in the system's endian order,
  which is usually little endian. How to disassemble:

    echo <hex> | llvm-mc --disassemble --hex
  */
  IF_DEBUG({
    eprintf("[system] compiled word: " FMT_QUOTED "\n", sym->name.buf);
    comp_debug_print_sym_instrs(comp, sym, "[system]   ");

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
  //     sym->name.buf
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

static Err intrin_immediate(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  sym->immediate = true;
  return nullptr;
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

static Err intrin_extern_ptr(Interp *interp) {
  void *addr;
  try(interp_read_extern(interp, &addr));

  const auto sym = stack_push(
    &interp->syms,
    (Sym){
      .type    = SYM_EXT_PTR,
      .name    = interp->reader->word,
      .ext_ptr = addr,
    }
  );

  dict_set(&interp->words, sym->name.buf, sym);
  comp_register_dysym(&interp->comp, sym->name.buf, (U64)addr);
  return nullptr;
}

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

static Err debug_word(Interp *interp) {
  Sym *sym;
  try(interp_read_sym(interp, &sym));

  const auto name = sym->name.buf;

  switch (sym->type) {
    case SYM_NORM: {
      eprintf(
        "[debug] word:\n"
        "[debug]   name: %s\n"
        "[debug]   type: normal\n"
        "[debug]   execution token: %p\n",
        name,
        sym
      );
      comp_debug_print_sym_instrs(&interp->comp, sym, "[debug]   ");
      IF_DEBUG({
        eprintf("[debug]   symbol: ");
        repr_struct(sym);
      });
      return nullptr;
    }

    case SYM_INTRIN: {
      eprintf(
        "[debug] word:\n"
        "[debug]   name: %s\n"
        "[debug]   type: intrinsic\n"
        "[debug]   execution token: %p\n"
        "[debug]   address: %p\n",
        name,
        sym,
        sym->intrin
      );
      return nullptr;
    }

    case SYM_EXT_PTR: {
      eprintf(
        "[debug] word:\n"
        "[debug]   name: %s\n"
        "[debug]   type: external symbol\n"
        "[debug]   execution token: %p\n"
        "[debug]   address: %p\n",
        name,
        sym,
        sym->ext_ptr
      );
      return nullptr;
    }

    case SYM_EXT_PROC: {
      eprintf(
        "[debug] word:\n"
        "[debug]   name: %s\n"
        "[debug]   type: external procedure\n"
        "[debug]   address: %p\n",
        name,
        sym->ext_proc
      );
      return nullptr;
    }

    default: unreachable();
  }
}

static Err debug_sync_code(Interp *interp) {
  try(comp_code_sync(&interp->comp.code));
  eprintf("[debug] synced code heaps\n");
  return nullptr;
}

#ifdef NATIVE_CALL_ABI
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
  .name.buf  = ":",
  .intrin    = (void *)intrin_colon,
  .throws    = true,
  .immediate = true,
};

static constexpr auto INTRIN_SEMICOLON = (Sym){
  .name.buf  = ";",
  .intrin    = (void *)intrin_semicolon,
  .throws    = true,
  .immediate = true,
  .comp_only = true,
};

static constexpr auto INTRIN_BRACKET_BEG = (Sym){
  .name.buf  = "[",
  .intrin    = (void *)intrin_bracket_beg,
  .immediate = true,
};

static constexpr auto INTRIN_BRACKET_END = (Sym){
  .name.buf = "]",
  .intrin   = (void *)intrin_bracket_end,
};

static constexpr auto INTRIN_RET = (Sym){
  .name.buf  = "#ret",
  .intrin    = (void *)intrin_ret,
  .throws    = true,
  .immediate = true,
  .comp_only = true,
};

static constexpr auto INTRIN_RECUR = (Sym){
  .name.buf  = "#recur",
  .intrin    = (void *)intrin_recur,
  .throws    = true,
  .immediate = true,
  .comp_only = true,
};

static constexpr auto INTRIN_IMMEDIATE = (Sym){
  .name.buf  = "immediate",
  .intrin    = (void *)intrin_immediate,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_COMP_ONLY = (Sym){
  .name.buf  = "comp_only",
  .intrin    = (void *)intrin_comp_only,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_NOT_COMP_ONLY = (Sym){
  .name.buf  = "not_comp_only",
  .intrin    = (void *)intrin_not_comp_only,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_INLINE = (Sym){
  .name.buf  = "inline",
  .intrin    = (void *)intrin_inline,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_THROWS = (Sym){
  .name.buf  = "throws",
  .intrin    = (void *)intrin_throws,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_REDEFINE = (Sym){
  .name.buf  = "redefine",
  .intrin    = (void *)intrin_redefine,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_HERE = (Sym){
  .name.buf  = "here",
  .intrin    = (void *)intrin_here,
  .comp_only = true,
};

static constexpr auto INTRIN_COMP_INSTR = (Sym){
  .name.buf    = "comp_instr",
  .intrin      = (void *)intrin_comp_instr,
  .inp_len     = 1,
  .throws      = true,
  .comp_only   = true,
  .interp_only = true,
};

static constexpr auto INTRIN_COMP_LOAD = (Sym){
  .name.buf  = "comp_load",
  .intrin    = (void *)intrin_comp_load,
  .inp_len   = 2,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_ALLOC_DATA = (Sym){
  .name.buf  = "alloc_data",
  .intrin    = (void *)intrin_alloc_data,
  .inp_len   = 2, // ( buf len -- addr ) ; buffer address may be nil
  .out_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_COMP_PAGE_ADDR = (Sym){
  .name.buf  = "comp_page_addr",
  .intrin    = (void *)intrin_comp_page_addr,
  .inp_len   = 2, // ( adr reg -- )
  .out_len   = 0,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_COMP_CALL = (Sym){
  .name.buf  = "comp_call",
  .intrin    = (void *)intrin_comp_call,
  .inp_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_QUIT = (Sym){
  .name.buf = "quit",
  .intrin   = (void *)intrin_quit,
  .throws   = true,
};

static constexpr auto INTRIN_CHAR = (Sym){
  .name.buf = "char",
  .intrin   = (void *)intrin_char,
  .throws   = true,
};

static constexpr auto INTRIN_PARSE = (Sym){
  .name.buf = "parse",
  .intrin   = (void *)intrin_parse,
  .inp_len  = 1,
  .out_len  = 2,
  .throws   = true,
};

static constexpr auto INTRIN_PARSE_WORD = (Sym){
  .name.buf = "parse_word",
  .intrin   = (void *)intrin_parse_word,
  .out_len  = 2,
  .throws   = true,
};

static constexpr auto INTRIN_IMPORT = (Sym){
  .name.buf = "import",
  .intrin   = (void *)intrin_import,
  .inp_len  = 2,
  .throws   = true,
};

static constexpr auto INTRIN_IMPORT_QUOTE = (Sym){
  .name.buf = "import\"",
  .intrin   = (void *)intrin_import_quote,
  .throws   = true,
};

static constexpr auto INTRIN_IMPORT_TICK = (Sym){
  .name.buf = "import'",
  .intrin   = (void *)intrin_import_tick,
  .throws   = true,
};

static constexpr auto INTRIN_EXTERN_PTR = (Sym){
  .name.buf = "extern_ptr:",
  .intrin   = (void *)intrin_extern_ptr,
  .throws   = true,
};

static constexpr auto INTRIN_EXTERN_PROC = (Sym){
  .name.buf = "extern:",
  .intrin   = (void *)intrin_extern_proc,
  .inp_len  = 2,
  .throws   = true,
};

static constexpr auto INTRIN_FIND_WORD = (Sym){
  .name.buf = "find_word",
  .intrin   = (void *)intrin_find_word,
  .inp_len  = 2,
  .out_len  = 1,
  .throws   = true,
};

static constexpr auto INTRIN_INLINE_WORD = (Sym){
  .name.buf  = "inline_word",
  .intrin    = (void *)intrin_inline_word,
  .inp_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_EXECUTE = (Sym){
  .name.buf = "execute",
  .intrin   = (void *)intrin_execute,
  .inp_len  = 1,
  .throws   = true,
};

static constexpr auto INTRIN_GET_LOCAL = (Sym){
  .name.buf  = "get_local",
  .intrin    = (void *)intrin_get_local,
  .inp_len   = 2,
  .out_len   = 1,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_ANON_LOCAL = (Sym){
  .name.buf  = "anon_local",
  .intrin    = (void *)intrin_anon_local,
  .throws    = true,
  .comp_only = true,
};

static constexpr auto INTRIN_DEBUG_FLUSH = (Sym){
  .name.buf = "debug_flush",
  .intrin   = (void *)debug_flush,
};

static constexpr auto INTRIN_DEBUG_THROW = (Sym){
  .name.buf = "debug_throw",
  .intrin   = (void *)debug_throw,
  .throws   = true,
};

static constexpr auto INTRIN_DEBUG_STACK_LEN = (Sym){
  .name.buf = "debug_stack_len",
  .intrin   = (void *)debug_stack_len,
};

static constexpr auto INTRIN_DEBUG_STACK = (Sym){
  .name.buf = "debug_stack",
  .intrin   = (void *)debug_stack,
};

static constexpr auto INTRIN_DEBUG_DEPTH = (Sym){
  .name.buf = "debug_depth",
  .intrin   = (void *)debug_depth,
};

static constexpr auto INTRIN_DEBUG_TOP_INT = (Sym){
  .name.buf = "debug_top_int",
  .intrin   = (void *)debug_top_int,
};

static constexpr auto INTRIN_DEBUG_TOP_PTR = (Sym){
  .name.buf = "debug_top_ptr",
  .intrin   = (void *)debug_top_ptr,
};

static constexpr auto INTRIN_DEBUG_TOP_STR = (Sym){
  .name.buf = "debug_top_str",
  .intrin   = (void *)debug_top_str,
};

static constexpr auto INTRIN_DEBUG_MEM = (Sym){
  .name.buf = "debug_mem",
  .intrin   = (void *)debug_mem,
  .throws   = true,
};

static constexpr auto INTRIN_DEBUG_WORD = (Sym){
  .name.buf  = "debug'",
  .intrin    = (void *)debug_word,
  .throws    = true,
  .immediate = true,
};

static constexpr auto INTRIN_DEBUG_SYNC_CODE = (Sym){
  .name.buf = "debug_sync_code",
  .intrin   = (void *)debug_sync_code,
  .throws   = true,
};

#ifdef NATIVE_CALL_ABI
#include "./c_intrin_list_cc_reg.c" // IWYU pragma: export
#else
#include "./c_intrin_list_cc_stack.c" // IWYU pragma: export
#endif
