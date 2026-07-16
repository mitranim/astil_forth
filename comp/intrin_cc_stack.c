#pragma once
#include "./interp_internal.c"

// #ifdef CLANGD
// #include "./intrin.c"
// #endif

static Err interp_pop_data_ptr(Interp *interp, const U8 **out) {
  Sint buf;
  try(int_stack_pop(&interp->ints, &buf));
  try(interp_validate_data_ptr(buf));
  if (out) *out = (const U8 *)buf;
  return nullptr;
}

static Err interp_pop_data_ptr_opt(Interp *interp, const U8 **out) {
  Sint buf;
  try(int_stack_pop(&interp->ints, &buf));
  if (buf) try(interp_validate_data_ptr(buf));
  if (out) *out = (const U8 *)buf;
  return nullptr;
}

static Err interp_pop_data_len(Interp *interp, Ind *out) {
  Sint len;
  try(int_stack_pop(&interp->ints, &len));
  try(interp_validate_data_len(len));
  if (out) *out = (Ind)len;
  return nullptr;
}

static Err interp_pop_str(Interp *interp, const char **out_buf, Ind *out_len) {
  const auto ints = &interp->ints;

  Sint len;
  Sint buf;
  try(int_stack_pop(ints, &len));
  try(int_stack_pop(ints, &buf));
  try(interp_validate_buf_len(buf, len));

  if (out_buf) *out_buf = (const char *)buf;
  if (out_len) *out_len = (Ind)len;
  return nullptr;
}

static Err interp_pop_reg(Interp *interp, U8 *out) {
  Sint reg;
  try(int_stack_pop(&interp->ints, &reg));
  try(asm_validate_reg(reg));
  if (out) *out = (U8)reg;
  return nullptr;
}

static Err interp_valid_name(Interp *interp, Word_str *out) {
  const char *buf;
  Ind         len;
  try(interp_pop_str(interp, &buf, &len));
  try(valid_word(buf, len, out));
  return nullptr;
}

static Err interp_fun_begin(Interp *interp, Wordlist list, Word_str name) {
  try(interp_begin_definition(interp));
  try(interp_word_begin(interp, list, name));
  return nullptr;
}

static Err interp_fun(Interp *interp, Wordlist list) {
  Word_str name;
  try(interp_read_word(interp, &name));
  try(interp_fun_begin(interp, list, name));

  const auto sym = interp_semicolon_sym(interp);
  try(int_stack_push(&interp->ints, (Sint)sym));
  return nullptr;
}

static Err intrin_fun(Interp *interp) {
  return interp_fun(interp, WORDLIST_EXEC);
}

static Err intrin_fun_comp(Interp *interp) {
  return interp_fun(interp, WORDLIST_COMP);
}

static Err intrin_define_fun(Interp *interp) {
  Word_str name;
  try(interp_valid_name(interp, &name));
  try(interp_fun_begin(interp, WORDLIST_EXEC, name));
  return nullptr;
}

static Err intrin_define_fun_comp(Interp *interp) {
  Word_str name;
  try(interp_valid_name(interp, &name));
  try(interp_fun_begin(interp, WORDLIST_COMP, name));
  return nullptr;
}

static Err intrin_ret(Interp *interp) { return comp_append_ret(&interp->comp); }

static Err intrin_recur(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  try(comp_append_recur(&interp->comp));
  if (sym->has_err) asm_append_err_reg_try(&interp->comp);
  sym->norm.has_recur = true;
  return nullptr;
}

static Err intrin_try(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  try(sym_has_err_set(sym, true, "compile `try`"));
  asm_append_pop_try(&interp->comp);
  return nullptr;
}

static Err intrin_throw(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  try(sym_has_err_set(sym, true, "compile `throw`"));
  asm_append_pop_throw(&interp->comp);
  return nullptr;
}

static Err interp_catch(Interp *interp, Wordlist wordlist) {
  Sym_dict *dict;
  try(interp_wordlist(interp, wordlist, &dict));
  Word_str word;
  try(interp_read_word(interp, &word));

  const auto name = word.buf;
  const auto sym  = dict_get(dict, name);
  if (!sym) return err_word_undefined_in_wordlist(name, wordlist);
  if (!sym->has_err) return err_catch_no_throw(name);

  constexpr bool err_mode = true;
  const auto     comp     = &interp->comp;

  switch (sym->type) {
    case SYM_NORM: {
      try(comp_append_call_norm(comp, sym, err_mode));
      return nullptr;
    }
    case SYM_INTRIN: {
      try(comp_append_call_intrin(comp, sym, err_mode));
      return nullptr;
    }
    case SYM_EXTERN: {
      return errf("unable to catch extern word " FMT_QUOTED, name);
    }
    default: unreachable();
  }
  return nullptr;
}

static Err intrin_catch(Interp *interp) {
  Sint wordlist;
  try(int_stack_pop(&interp->ints, &wordlist));
  try(interp_catch(interp, (Wordlist)wordlist));
  return nullptr;
}

static Err intrin_throws(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  Sint val;
  try(int_stack_pop(&interp->ints, &val));
  try(sym_has_err_set(sym, !!val, "set `throws`"));
  return nullptr;
}

static Err intrin_comp_only(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));

  Sint val;
  try(int_stack_pop(&interp->ints, &val));

  sym->comp_only = !!val;
  return nullptr;
}

static Err intrin_interp_only(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));

  Sint val;
  try(int_stack_pop(&interp->ints, &val));

  sym->interp_only = !!val;
  return nullptr;
}

static Err intrin_inline(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  sym->norm.inlinable = true;
  return nullptr;
}

/*
Caution: unlike most Forth systems, because of Apple's W^X restrictions,
we use two code heaps: writable and executable. Control structures often
patch up instructions in the writable heap during word compilation. When
a word is finalized, by copying its instructions to the executable heap,
any further modifications of its range in the writable heap become nops.
*/
static Err intrin_here_write(Interp *interp) {
  try(int_stack_push(
    &interp->ints, (Sint)(comp_code_next_writable_instr(&interp->comp.code))
  ));
  return nullptr;
}

/*
Returns an address in the executable heap corresponding to the next instruction
in the writable heap during word compilation. The returned address does NOT yet
contain a valid instruction, but may be stashed for later use.
*/
static Err intrin_here_exec(Interp *interp) {
  try(int_stack_push(
    &interp->ints, (Sint)(comp_code_next_prog_counter(&interp->comp.code))
  ));
  return nullptr;
}

static Err intrin_comp_instr(Interp *interp) {
  Sint val;
  try(int_stack_pop(&interp->ints, &val));
  try(asm_append_instr_from_int(&interp->comp, val));
  return nullptr;
}

static Err intrin_comp_load(Interp *interp) {
  try(interp_require_current_sym(interp, nullptr));

  Sint imm;
  U8   reg;
  try(interp_pop_reg(interp, &reg));
  try(int_stack_pop(&interp->ints, &imm));

  asm_append_imm_to_reg(&interp->comp, (U8)reg, imm);
  return nullptr;
}

static Err intrin_comp_alloc_data(Interp *interp) {
  const U8 *buf;
  Ind       len;
  const U8 *adr;
  try(interp_pop_data_len(interp, &len));
  try(interp_pop_data_ptr_opt(interp, &buf));
  try(comp_alloc_data(&interp->comp, buf, len, &adr));
  try(int_stack_push(&interp->ints, (Sint)adr));
  return nullptr;
}

static Err intrin_comp_page_addr(Interp *interp) {
  const U8 *adr;
  U8        reg;
  try(interp_pop_reg(interp, &reg));
  try(interp_pop_data_ptr(interp, &adr));
  try(comp_append_page_addr(&interp->comp, (Uint)adr, reg));
  return nullptr;
}

static Err intrin_comp_page_load(Interp *interp) {
  const U8 *adr;
  U8        reg;
  try(interp_pop_reg(interp, &reg));
  try(interp_pop_data_ptr(interp, &adr));
  try(comp_append_page_load(&interp->comp, (Uint)adr, reg));
  return nullptr;
}

static Err intrin_comp_call(Interp *interp) {
  Sint ptr;
  Sym *sym;
  try(int_stack_pop(&interp->ints, &ptr));
  try(interp_sym_by_ptr(interp, ptr, &sym));
  try(comp_append_call_sym(&interp->comp, sym));
  return nullptr;
}

static Err intrin_read_char(Interp *interp) {
  char out;
  try(interp_read_char(interp, &out));
  try(int_stack_push(&interp->ints, out));
  return nullptr;
}

/*
Technically not fundamental and possible to write in Forth with `intrin_read_char`,
but requires conditionals, character comparison, and a loop, none of which are
available early in the bootstrap process. Neither is the self-assembler, which
means we'd have to hardcode instructions in binary.

TODO: use `intrin_read_char` in Forth to implement a more powerful version
which uses a string rather than a char as a delimiter.
*/
static Err intrin_read_until_char(Interp *interp) {
  const auto ints = &interp->ints;

  Sint delim;
  try(int_stack_pop(ints, &delim));
  try(validate_ascii_printable(delim));

  const char *buf;
  Ind         len;
  try(interp_parse_until(interp, (U8)delim, &buf, &len));
  try(int_stack_push(ints, (Sint)buf));
  try(int_stack_push(ints, (Sint)len));
  return nullptr;
}

// Technically not fundamental. TODO implement in Forth via intrinsic `char`.
static Err intrin_read_word(Interp *interp) {
  const auto  ints = &interp->ints;
  const char *buf;
  Ind         len;
  try(interp_parse_word(interp, &buf, &len));
  try(int_stack_push(ints, (Sint)buf));
  try(int_stack_push(ints, (Sint)len));
  return nullptr;
}

static Err intrin_import(Interp *interp) {
  const char *path;
  Ind         len;
  try(interp_pop_str(interp, &path, &len));
  if (!len) return "unable to import: missing path";

  const auto copy = str_alloc_copy(path, len);
  if (!copy) return "unable to copy import path";

  const auto err = interp_import(interp, copy);
  free(copy);
  return err;
}

static Err intrin_comp_extern_adr(Interp *interp) {
  Word_str name;
  try(interp_valid_name(interp, &name));
  try(interp_extern_adr(interp, name.buf));

  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  sym->norm.has_loads = true;

  const auto            comp = &interp->comp;
  static constexpr auto reg  = ASM_SCRATCH_REG_8;
  asm_append_dysym_load(comp, name.buf, reg, &comp->code.externs);
  asm_append_stack_push_from(comp, reg);

  return nullptr;
}

static Err intrin_extern_fun(Interp *interp) {
  const auto ints = &interp->ints;

  Sint out_len;
  Sint inp_len;
  try(int_stack_pop(ints, &out_len));
  try(int_stack_pop(ints, &inp_len));

  Word_str link_name;
  try(interp_valid_name(interp, &link_name));
  Word_str name;
  try(interp_valid_name(interp, &name));
  try(interp_extern_fun(interp, name.buf, link_name.buf, inp_len, out_len));
  return nullptr;
}

static Err intrin_find_word(Interp *interp) {
  Word_str name;
  try(interp_valid_name(interp, &name));

  Sint wordlist;
  try(int_stack_pop(&interp->ints, &wordlist));

  const Sym *sym;
  try(interp_find_word(interp, name.buf, (Wordlist)wordlist, &sym));
  try(int_stack_push(&interp->ints, (Sint)sym));
  return nullptr;
}

static Err intrin_inline_word(Interp *interp) {
  constexpr bool catch = false;
  Sint           ptr;
  Sym           *caller;
  Sym           *callee;

  try(int_stack_pop(&interp->ints, &ptr));
  try(interp_require_current_sym(interp, &caller));
  try(interp_sym_by_ptr(interp, ptr, &callee));
  try(comp_inline_sym(&interp->comp, caller, callee, catch));
  return nullptr;
}

static Err intrin_call_xt(Interp *interp) { return intrin_end(interp); }

static Err intrin_comp_local(Interp *interp) {
  const auto ints = &interp->ints;

  Sint len;
  Sint name;
  try(int_stack_pop(ints, &len));
  try(int_stack_pop(ints, &name));

  const auto comp = &interp->comp;

  if (name) {
    try(interp_validate_data_ptr(name));
    try(interp_validate_data_len(len));

    Local *loc;
    try(interp_get_local(interp, (const char *)name, (Ind)len, &loc));

    if (!loc->inited) {
      comp_local_alloc_mem(comp, loc);
      loc->inited = true;
    }

    try(int_stack_push(&interp->ints, loc->fp_off));
    return nullptr;
  }

  const auto loc = comp_local_anon(comp);
  comp_local_alloc_mem(comp, loc);
  try(int_stack_push(&interp->ints, loc->fp_off));
  return nullptr;
}

static Err debug_mem(Interp *interp) {
  Sint adr;
  try(int_stack_pop(&interp->ints, &adr));
  debug_mem_at((const Uint *)adr);
  return nullptr;
}

static Err debug_word(Interp *interp) {
  Sint ptr;
  Sym *sym;
  try(int_stack_pop(&interp->ints, &ptr));
  try(interp_sym_by_ptr(interp, ptr, &sym));
  interp_repr_sym(interp, sym);
  return nullptr;
}

/*
Note: the input-output count is ignored in stack-CC.
It's specified only for vain consistency with reg-CC.
*/

static const USED auto INTRIN_THROWS = (Sym){
  .name.buf  = ".throws",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_throws,
  .inp_len   = 1,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_CATCH = (Sym){
  .name.buf = ".catch",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_catch,
  .inp_len  = 1,
  .out_len  = 1,
  .has_err  = true,
};

static const USED auto INTRIN_INLINE = (Sym){
  .name.buf  = ".inline",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_inline,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_INLINE_WORD = (Sym){
  .name.buf  = ".inline_word",
  .wordlist  = WORDLIST_EXEC,
  .intrin    = (void *)intrin_inline_word,
  .inp_len   = 1,
  .out_len   = 1,
  .has_err   = true,
  .comp_only = true,
};

static const USED auto INTRIN_DEBUG_WORD = (Sym){
  .name.buf = ".debug_word",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)debug_word,
  .inp_len  = 1,
  .out_len  = 1,
  .has_err  = true,
};
