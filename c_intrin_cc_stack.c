#pragma once
#include "./c_interp_internal.c"

// #ifdef CLANGD
// #include "./c_intrin.c"
// #endif

static Err interp_validate_data_ptr(Sint ptr, const U8 **out) {
  /*
  Some systems deliberately ensure that virtual memory addresses
  are just out of range of `int32`. Would be nice to check if the
  pointer is somewhere in that range, but the assumption would be
  really non-portable.

  In a Forth implementation for OS-free microchips with a very small
  amount of real memory, valid data pointers could begin close to 0,
  depending on how data is organized. Since our implementation only
  supports 64-bit machines and requires an OS, we can at least assume
  addresses which are "decently large".
  */
  if (ptr > (1 << 14)) {
    *out = (U8 *)ptr;
    return nullptr;
  }
  return errf("suspiciously invalid-looking data pointer: %p", (U8 *)ptr);
}

static Err interp_validate_data_len(Sint len, Ind *out) {
  if (len > 0 && len < (Sint)IND_MAX) {
    *out = (Ind)len;
    return nullptr;
  }
  return errf("invalid data length: " FMT_SINT, len);
}

static Err interp_pop_data_ptr(Interp *interp, const U8 **out) {
  Sint buf;
  try(int_stack_pop(&interp->ints, &buf));
  try(interp_validate_data_ptr(buf, out));
  return nullptr;
}

static Err interp_pop_data_ptr_opt(Interp *interp, const U8 **out) {
  Sint buf;
  try(int_stack_pop(&interp->ints, &buf));
  if (buf) {
    try(interp_validate_data_ptr(buf, out));
  }
  else if (out) {
    *out = nullptr;
  }
  return nullptr;
}

static Err interp_pop_data_len(Interp *interp, Ind *out) {
  Sint len;
  try(int_stack_pop(&interp->ints, &len));
  try(interp_validate_data_len(len, out));
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
  try(interp_pop_data_len(interp, &len));
  try(interp_pop_data_ptr(interp, (const U8 **)&buf));
  try(valid_word(buf, len, out));
  return nullptr;
}

static Err intrin_colon(Interp *interp) {
  try(interp_begin_definition(interp));
  try(interp_word_begin(interp, WORDLIST_EXEC, interp->reader->word));
  return nullptr;
}

static Err intrin_colon_colon(Interp *interp) {
  try(interp_begin_definition(interp));
  try(interp_word_begin(interp, WORDLIST_COMP, interp->reader->word));
  return nullptr;
}

static Err intrin_colon_named(Interp *interp) {
  Word_str name;
  try(interp_valid_name(interp, &name));
  try(interp_word_begin(interp, WORDLIST_EXEC, name));
  return nullptr;
}

static Err intrin_colon_colon_named(Interp *interp) {
  Word_str name;
  try(interp_valid_name(interp, &name));
  try(interp_word_begin(interp, WORDLIST_COMP, name));
  return nullptr;
}

static Err intrin_ret(Interp *interp) { return comp_append_ret(&interp->comp); }

static Err intrin_recur(Interp *interp) {
  return comp_append_recur(&interp->comp);
}

static Err intrin_comp_instr(Interp *interp) {
  Sint val;
  try(int_stack_pop(&interp->ints, &val));
  try(asm_append_instr_from_int(&interp->comp, val));
  return nullptr;
}

static Err intrin_comp_load(Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));

  Sint imm;
  U8   reg;
  try(interp_pop_reg(interp, &reg));
  try(int_stack_pop(&interp->ints, &imm));

  bool has_load = true;
  asm_append_imm_to_reg(&interp->comp, (U8)reg, imm, &has_load);
  if (has_load) sym->norm.has_loads = true;
  return nullptr;
}

static Err intrin_alloc_data(Interp *interp) {
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

static Err interp_sym_by_ptr(Interp *interp, Sint ptr, Sym **out) {
  const auto sym = (Sym *)ptr;
  try(interp_validate_sym_ptr(interp, sym));

  IF_DEBUG(eprintf(
    "[system] found address of symbol " FMT_QUOTED ": %p\n", sym->name.buf, sym
  ));

  if (out) *out = sym;
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

/*
Technically not fundamental and possible to write in Forth with `intrin_char`,
but requires conditionals, character comparison, and a loop, none of which are
available early in the bootstrap process. Neither is the self-assembler, which
means we'd have to hardcode instructions in binary.

TODO: use `intrin_char` in Forth to implement a more powerful version
which uses a string rather than a char as a delimiter.
*/
static Err intrin_parse(Interp *interp) {
  Sint delim;
  try(int_stack_pop(&interp->ints, &delim));
  try(validate_char_ascii_printable(delim));
  try(interp_parse_until(interp, (U8)delim));
  return nullptr;
}

static Err intrin_import(Interp *interp) {
  const auto ints = &interp->ints;
  Sint       path;

  try(int_stack_pop(ints, nullptr)); // discard length
  try(int_stack_pop(ints, &path));
  try(interp_import(interp, (const char *)path));
  return nullptr;
}

// See comment on `interp_extern_got` for explanation.
static Err intrin_extern_got(Interp *interp) {
  const char *name;
  Ind         len;
  try(interp_pop_data_len(interp, &len));
  try(interp_pop_data_ptr(interp, (const U8 **)&name));
  try(interp_extern_got(interp, name, len));
  return nullptr;
}

static Err intrin_extern_proc(Interp *interp) {
  const auto ints = &interp->ints;
  Sint       out_len;
  Sint       inp_len;
  try(int_stack_pop(ints, &out_len));
  try(int_stack_pop(ints, &inp_len));
  try(interp_extern_proc(interp, inp_len, out_len));
  return nullptr;
}

static Err intrin_find_word(Interp *interp) {
  Sint      wordlist;
  const U8 *buf;
  Ind       len;
  try(interp_pop_data_len(interp, &len));
  try(interp_pop_data_ptr(interp, &buf));
  try(int_stack_pop(&interp->ints, &wordlist));
  try(interp_find_word(interp, (const char *)buf, len, (Wordlist)wordlist));
  return nullptr;
}

static Err intrin_inline_word(Interp *interp) {
  Sint ptr;
  Sym *sym;
  try(int_stack_pop(&interp->ints, &ptr));
  try(interp_sym_by_ptr(interp, ptr, &sym));
  try(comp_inline_sym(&interp->comp, sym));
  return nullptr;
}

static Err intrin_execute(Interp *interp) {
  Sint ptr;
  Sym *sym;
  try(int_stack_pop(&interp->ints, &ptr));
  try(interp_sym_by_ptr(interp, ptr, &sym));
  try(interp_call_sym(interp, sym));
  return nullptr;
}

static Err intrin_get_local(Interp *interp) {
  const auto comp = &interp->comp;
  const U8  *buf;
  Ind        len;
  Local     *loc;
  try(interp_pop_data_len(interp, &len));
  try(interp_pop_data_ptr(interp, &buf));
  try(interp_get_local(interp, (const char *)buf, len, &loc));

  if (!loc->inited) {
    comp_local_alloc_mem(comp, loc);
    loc->inited = true;
  }

  const auto tok = local_token(loc);
  try(int_stack_push(&interp->ints, tok));
  return nullptr;
}

static Err intrin_anon_local(Interp *interp) {
  const auto comp = &interp->comp;
  const auto loc  = comp_local_anon(comp);

  comp_local_alloc_mem(comp, loc);

  const auto tok = local_token(loc);
  try(int_stack_push(&interp->ints, (Sint)tok));
  return nullptr;
}

static constexpr USED auto INTRIN_COLON_NAMED = (Sym){
  .name.buf = "colon",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_colon_named,
  .inp_len  = 2,
  .throws   = true,
};

static constexpr USED auto INTRIN_COLON_COLON_NAMED = (Sym){
  .name.buf = "colon_colon",
  .wordlist = WORDLIST_EXEC,
  .intrin   = (void *)intrin_colon_colon_named,
  .inp_len  = 2,
  .throws   = true,
};
