/*
Intrinsic functions necessary for "bootstrapping" the Forth system.
Should be kept to a minimum. Non-fundamental words should be written
in Forth; where regular language is insufficient, we use inline asm.

Intrinsics don't even provide conditionals or arithmetic.

This file contains "generic" intrinsics which don't care about the calling
convention. Intrinsics which do are located in dedicated files.
*/
#pragma once
#include "../clib/spawn.c"
#include "./comp.c"
#include "./interp_internal.c"
#include "./sym.c"

static Err err_nested_definition(const Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  return errf("unexpected \":\" in definition of " FMT_QUOTED, sym->name.buf);
}

static Err err_wordlist_at_capacity(const char *name, Wordlist list, Sint cap) {
  return errf(
    "unable to create word " FMT_QUOTED
    " in wordlist %d (%s): wordlist at capacity (" FMT_SINT " words)",
    name,
    list,
    wordlist_name(list),
    cap
  );
}

static Err interp_word_begin(Interp *interp, Wordlist wordlist, Word_str name) {
  IF_DEBUG(eprintf(
    "[system] starting definition of " FMT_QUOTED
    " associated with wordlist %d\n",
    name.buf,
    wordlist
  ));

  const auto syms = &interp->syms;

  if (stack_rem(syms) <= 0) {
    return err_wordlist_at_capacity(name.buf, wordlist, stack_cap(syms));
  }

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

#ifndef CALL_CONV_STACK
  interp->comp.ctx.slop = interp->module ? interp->module->slop : interp->slop;
#endif
  return nullptr;
}

static Err interp_begin_definition(Interp *interp) {
  if (interp->comp.ctx.sym) return err_nested_definition(interp);
  try(interp_snapshot(interp));
  return nullptr;
}

static void interp_repr_sym(const Interp *, const Sym *);

static Err intrin_semicolon(Interp *interp) {
  const auto comp  = &interp->comp;
  const auto redef = interp->comp.ctx.redefining; // Snapshot before clearing.

#ifndef CALL_CONV_STACK
  try(comp_before_append_ret(comp));
#endif

  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  try(sym_validate_name(sym));
  try(comp_sym_end(comp, sym));
  try(interp_snapshot(interp));

  const auto name     = sym->name.buf;
  const auto wordlist = sym->wordlist;

  Sym_dict *dict;
  try(interp_wordlist(interp, wordlist, &dict));

  try(interp_validate_redefinition(interp, name, wordlist, redef));
  dict_set(dict, name, sym);

  IF_DEBUG({
    eputs("[debug] compiled:");
    interp_repr_sym(interp, sym);
  });

  return nullptr;
}

static Sym *interp_semicolon_sym(Interp *interp) { return interp->syms.floor; }

static Err interp_semicolon_push(Interp *interp) {
  const auto sym = interp_semicolon_sym(interp);
  return int_stack_push(&interp->ints, (Sint)sym);
}

static Err intrin_end(Interp *interp) {
  if (!stack_len(&interp->ints)) {
    return err_str("unexpected `end`: no structure to close");
  }

  Sint ptr;
  Sym *sym;
  try(int_stack_pop(&interp->ints, &ptr));
  try(interp_sym_by_ptr(interp, ptr, &sym));
  try(interp_call_sym(interp, sym));
  return nullptr;
}

static void intrin_bracket_beg(Interp *interp) {
  interp->comp.ctx.compiling = false;
}

static void intrin_bracket_end(Interp *interp) {
  interp->comp.ctx.compiling = true;
}

static Err intrin_redefine(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  try(interp_validate_redefinition(interp, sym->name.buf, sym->wordlist, true));
  interp->comp.ctx.redefining = true;
  return nullptr;
}

static Err intrin_plain_call(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  sym->plain_call = true;
  return nullptr;
}

/*
This intrinsic is unnecessary for most programs, as they can simply
call the libc function `exit` or do a raw exit syscall. However, on
the slim chance a C program wants to embed this interpreter and run
some Forth code, this allows Forth code to terminate interpretation
without stopping the outer C program.
*/
static Err intrin_quit(Interp *interp) {
  const auto comp = &interp->comp;
  if (comp->ctx.sym) {
    return err_str("can't quit while defining a word");
  }
  return ERR_QUIT;
}

static Err intrin_ret_exec(Interp *) { return ERR_RET; }

static Err err_char_eof() {
  return err_str("EOF where a character was expected");
}

static Err interp_read_char(Interp *interp, char *out) {
  const auto read = interp_reader(interp);
  const auto next = read_char_at(read, read->pos);
  try(validate_ascii_printable(next));
  if (!next) return err_char_eof();
  read->pos++;
  if (out) *out = next;
  return nullptr;
}

static Err interp_parse_word(Interp *interp, const char **out_buf, Ind *out_len) {
  return read_word(interp_reader(interp), out_buf, out_len);
}

static Err intrin_use_tick(Interp *interp) {
  Word_str path;
  try(interp_read_word(interp, &path));
  try(interp_import(interp, path.buf));
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
      "[debug]   %p -- " FMT_UINT_HEX " " FMT_SINT "\n", ptr, (Uint)*ptr, *ptr
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

static void interp_repr_sym(const Interp *interp, const Sym *sym) {
  const auto list_name = wordlist_name(sym->wordlist);

  switch (sym->type) {
    case SYM_NORM: {
      fprintf(
        stderr,
        "[debug] word:\n"
        "[debug]   type:            regular\n"
        "[debug]   addr:            %p\n"
        "[debug]   name:            %s\n"
        "[debug]   wordlist:        %d (%s)\n"
#ifndef CALL_CONV_STACK
        "[debug]   clobber:         0b%s\n"
        "[debug]   inp_len:         %d\n"
        "[debug]   out_len:         %d\n"
#endif
        "[debug]   plain_call:      %s\n"
        "[debug]   has_err:         %s\n"
        "[debug]   comp_only:       %s\n"
        "[debug]   interp_only:     %s\n"
        "[debug]   inlinable:       %s\n"
        "[debug]   has_loads:       %s\n"
        "[debug]   has_recur:       %s\n"
        "[debug]   has_alloca:      %s\n"
        "[debug]   callers:         " FMT_IND
        "\n"
        "[debug]   callees:         " FMT_IND
        "\n"
        "[debug]   is_leaf:         %s\n"
        "[debug]   execution token: %p\n",
        sym,
        sym->name.buf,
        sym->wordlist,
        list_name,
#ifndef CALL_CONV_STACK
        uint32_to_bit_str((U32)sym->clobber),
        sym->inp_len,
        sym->out_len,
#endif
        bool_str(sym->plain_call),
        bool_str(sym->has_err),
        bool_str(sym->comp_only),
        bool_str(sym->interp_only),
        bool_str(sym->norm.inlinable),
        bool_str(sym->norm.has_loads),
        bool_str(sym->norm.has_recur),
        bool_str(sym->norm.has_alloca),
        sym->callers.len,
        sym->callees.len,
        bool_str(is_sym_leaf(sym)),
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
        "[debug]   type:               compiler intrinsic\n"
        "[debug]   addr:               %p\n"
        "[debug]   name:               %s\n"
        "[debug]   wordlist:           %d (%s)\n"
        "[debug]   inp_len:            %d\n"
        "[debug]   out_len:            %d\n"
        "[debug]   plain_call:         %s\n"
        "[debug]   has_err:            %s\n"
        "[debug]   execution token:    %p\n"
        "[debug]   executable address: %p\n",
        sym,
        sym->name.buf,
        sym->wordlist,
        list_name,
        sym->inp_len,
        sym->out_len,
        bool_str(sym->plain_call),
        bool_str(sym->has_err),
        sym,
        sym->intrin
      );
      return;
    }

    case SYM_EXTERN: {
      eprintf(
        "[debug] word:\n"
        "[debug]   type:               external function\n"
        "[debug]   addr:               %p\n"
        "[debug]   name:               %s\n"
        "[debug]   link_name:          %s\n"
        "[debug]   wordlist:           %d (%s)\n"
        "[debug]   plain_call:         %s\n"
        "[debug]   executable address: %p\n",
        sym,
        sym->name.buf,
        sym->link_name,
        sym->wordlist,
        list_name,
        bool_str(sym->plain_call),
        sym->exter
      );
      return;
    }

    default: unreachable();
  }
}

static Err interp_disasm_sym(Interp *interp, const Sym *sym) {
  switch (sym->type) {
    case SYM_NORM: break;
    case SYM_INTRIN:
      eprintf(
        "[system] unable to disassemble " FMT_QUOTED
        " in wordlist %d (%s), which is a compiler intrinsic\n",
        sym->name.buf,
        sym->wordlist,
        wordlist_name(sym->wordlist)
      );
      return nullptr;
    case SYM_EXTERN:
      eprintf(
        "[system] unable to disassemble " FMT_QUOTED
        " in wordlist %d (%s), which is a dynamically-linked external symbol\n",
        sym->name.buf,
        sym->wordlist,
        wordlist_name(sym->wordlist)
      );
      return nullptr;
    default: unreachable();
  }

  const auto comp = &interp->comp;
  const U8  *floor;
  const U8  *ceil;
  try(comp_sym_instr_range(
    comp, sym, (const Instr **)&floor, (const Instr **)&ceil
  ));

  eprintf(
    "[debug] dissassembly of " FMT_QUOTED " in wordlist %d (%s):\n",
    sym->name.buf,
    sym->wordlist,
    wordlist_name(sym->wordlist)
  );

  try(print_disasm(floor, (Ind)(ceil - floor)));
  return nullptr;
}

static Err debug_word_tick(Interp *interp) {
  Word_str word;
  try(interp_read_word(interp, &word));
  const auto name = word.buf;

  const auto exec = dict_get(&interp->dict_exec, name);
  if (exec) interp_repr_sym(interp, exec);

  const auto comp = dict_get(&interp->dict_comp, name);
  if (comp) interp_repr_sym(interp, comp);

  if (exec || comp) return nullptr;
  return err_word_undefined_in_each_wordlist(name);
}

static Err debug_dis(Interp *interp) {
  Word_str word;
  try(interp_read_word(interp, &word));
  const auto name = word.buf;

  const auto exec = dict_get(&interp->dict_exec, name);
  if (exec) try(interp_disasm_sym(interp, exec));

  const auto comp = dict_get(&interp->dict_comp, name);
  if (comp) try(interp_disasm_sym(interp, comp));

  if (exec || comp) return nullptr;
  return err_word_undefined_in_each_wordlist(name);
}

static Err debug_sync_code(Interp *interp) {
  try(comp_code_sync(&interp->comp.code));
  eprintf("[debug] synced code heaps\n");
  return nullptr;
}

#ifndef CALL_CONV_STACK
#include "./intrin_cc_reg.c"
#else
#include "./intrin_cc_stack.c"
#endif

/*
Note: ALL intrinsics must have certain fields set the same way:

  .type        = SYM_INTRIN
  .name.len    = strlen(.name.buf)
  .clobber     = ASM_REGS_VOLATILE
  .interp_only = true

Repetition is error-prone, so we set these fields in `sym_init_intrin`.

Some fields are used only in the register-based callvention:

  inp_len
  out_len
  clobber
*/

static const USED auto INTRIN_END = (Sym){
  .name.buf   = "end",
  .wordlist   = WORDLIST_COMP,
  .intrin     = (void *)intrin_end,
  .out_len    = 1,
  .has_err    = true,
  .plain_call = true,
};

static const USED auto INTRIN_FUN = (Sym){
  .name.buf = "fun:",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_fun,
  .out_len  = 2,
  .has_err  = true,
};

static const USED auto INTRIN_FUN_COMP = (Sym){
  .name.buf = "fun_comp:",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_fun_comp,
  .out_len  = 2,
  .has_err  = true,
};

static const USED auto INTRIN_DEFINE_FUN = (Sym){
  .name.buf = ".define_fun",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_define_fun,
  .inp_len  = 2,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_DEFINE_FUN_COMP = (Sym){
  .name.buf = ".define_fun_comp",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_define_fun_comp,
  .inp_len  = 2,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_BRACKET_BEG = (Sym){
  .name.buf  = "[",
  .wordlist  = WORDLIST_COMP,
  .intrin    = (void *)intrin_bracket_beg,
  .comp_only = true,
};

static const USED auto INTRIN_BRACKET_END = (Sym){
  .name.buf = "]",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_bracket_end,
};

static const USED auto INTRIN_RET = (Sym){
  .name.buf  = ".ret",
  .wordlist  = WORDLIST_COMP,
  .intrin    = (void *)intrin_ret,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_RET_EXEC = (Sym){
  .name.buf = ".ret",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_ret_exec,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_RECUR = (Sym){
  .name.buf  = ".recur",
  .wordlist  = WORDLIST_COMP,
  .intrin    = (void *)intrin_recur,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_TRY = (Sym){
  .name.buf = ".try",
  .wordlist = WORDLIST_COMP,
  .intrin   = (void *)intrin_try,
  .out_len  = 1,
  .has_err  = true,
#ifndef CALL_CONV_STACK
  .comp_only = true,
#endif // CALL_CONV_STACK
};

static const USED auto INTRIN_THROW = (Sym){
  .name.buf  = ".throw",
  .wordlist  = WORDLIST_COMP,
  .intrin    = (void *)intrin_throw,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_ONLY = (Sym){
  .name.buf  = ".comp_only",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_only,
  .inp_len   = 1,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_PLAIN_CALL = (Sym){
  .name.buf  = ".plain_call",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_plain_call,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_INTERP_ONLY = (Sym){
  .name.buf  = ".interp_only",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_interp_only,
  .inp_len   = 1,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_REDEFINE = (Sym){
  .name.buf  = ".redefine",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_redefine,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_HERE_WRITE = (Sym){
  .name.buf  = ".here_write",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_here_write,
  .out_len   = 2,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_HERE_EXEC = (Sym){
  .name.buf  = ".here_exec",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_here_exec,
  .out_len   = 2,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_INSTR = (Sym){
  .name.buf    = ".comp_instr",
  .wordlist    = WORDLIST_EXEC,
  .intrin      = (void *)intrin_comp_instr,
  .inp_len     = 1,
  .out_len     = 1,
  .has_err     = true,
  .comp_only   = true,
  .interp_only = true,
};

static const USED auto INTRIN_COMP_LOAD = (Sym){
  .name.buf  = ".comp_load",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_load,
  .inp_len   = 2,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_ALLOC_DATA = (Sym){
  .name.buf = ".comp_alloc_data",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_comp_alloc_data,
  .inp_len  = 2, // ( buf len -- addr ) ; buffer address may be nil
  .out_len  = 2,
  .has_err  = true,
};

static const USED auto INTRIN_COMP_PAGE_ADDR = (Sym){
  .name.buf  = ".comp_page_addr",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_page_addr,
  .inp_len   = 2, // ( adr reg -- )
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_PAGE_LOAD = (Sym){
  .name.buf  = ".comp_page_load",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_page_load,
  .inp_len   = 2, // ( adr reg -- )
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_COMP_CALL = (Sym){
  .name.buf  = ".comp_call",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_call,
  .inp_len   = 1,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_QUIT = (Sym){
  .name.buf = ".quit",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_quit,
  .out_len  = 1,
  .has_err  = true,
};

// Renamed from standard Forth `char`.
static const USED auto INTRIN_READ_CHAR = (Sym){
  .name.buf = ".read_char",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_read_char,
  .out_len  = 2,
  .has_err  = true,
};

// Renamed from standard Forth `parse` and made slightly non-standard:
// it really reads until char, without skipping over it.
static const USED auto INTRIN_READ_UNTIL_CHAR = (Sym){
  .name.buf = ".read_until_char",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_read_until_char,
  .inp_len  = 1,
  .out_len  = 3,
  .has_err  = true,
};

// Renamed from standard Forth `parse-name`.
static const USED auto INTRIN_READ_WORD = (Sym){
  .name.buf = ".read_word",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_read_word,
  .out_len  = 3,
  .has_err  = true,
};

static const USED auto INTRIN_IMPORT = (Sym){
  .name.buf = ".use",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_import,
  .inp_len  = 2,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_IMPORT_TICK = (Sym){
  .name.buf = "use'",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_use_tick,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_EXTERN_ADR = (Sym){
  .name.buf = ".comp_extern_adr",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_comp_extern_adr,
  .inp_len  = 2,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_EXTERN_FUN = (Sym){
  .name.buf = ".extern_fun",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_extern_fun,
  .inp_len  = 6,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_FIND_WORD = (Sym){
  .name.buf = ".find_word",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_find_word,
  .inp_len  = 3,
  .out_len  = 2,
  .has_err  = true,
};

// Renamed from standard Forth `execute`.
static const USED auto INTRIN_CALL_XT = (Sym){
  .name.buf = ".call_xt",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_call_xt,
  .inp_len  = 1,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_COMP_LOCAL = (Sym){
  .name.buf  = ".comp_local",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_comp_local,
  .inp_len   = 2,
  .out_len   = 2,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_DEBUG_ON = (Sym){
  .name.buf = ".debug_on",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_debug_on,
};

static const USED auto INTRIN_DEBUG_OFF = (Sym){
  .name.buf = ".debug_off",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_debug_off,
};

static const USED auto INTRIN_DEBUG_FLUSH = (Sym){
  .name.buf = ".debug_flush",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_flush,
};

static const USED auto INTRIN_DEBUG_THROW = (Sym){
  .name.buf = ".debug_throw",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_throw,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_DEBUG_STACK_LEN = (Sym){
  .name.buf = ".debug_stack_len",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_stack_len,
};

static const USED auto INTRIN_DEBUG_STACK = (Sym){
  .name.buf = ".debug_stack",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_stack,
};

static const USED auto INTRIN_DEBUG_DEPTH = (Sym){
  .name.buf = ".debug_depth",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_depth,
};

static const USED auto INTRIN_DEBUG_TOP_INT = (Sym){
  .name.buf = ".debug_top_int",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_top_int,
};

static const USED auto INTRIN_DEBUG_TOP_PTR = (Sym){
  .name.buf = ".debug_top_ptr",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_top_ptr,
};

static const USED auto INTRIN_DEBUG_TOP_STR = (Sym){
  .name.buf = ".debug_top_str",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_top_str,
};

static const USED auto INTRIN_DEBUG_MEM = (Sym){
  .name.buf = ".debug_mem",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_mem,
  .inp_len  = 1,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_DEBUG_WORD_TICK = (Sym){
  .name.buf = "debug'",
  .wordlist = WORDLIST_COMP,
  .intrin   = (void *)debug_word_tick,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_DEBUG_DIS = (Sym){
  .name.buf = "dis'",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_dis,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_DEBUG_SYNC_CODE = (Sym){
  .name.buf = ".debug_sync_code",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_sync_code,
  .out_len  = 1,
  .has_err  = true,
};

#ifndef CALL_CONV_STACK
#include "./intrin_list_cc_reg.c" // IWYU pragma: export
#else
#include "./intrin_list_cc_stack.c" // IWYU pragma: export
#endif
