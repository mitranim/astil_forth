#pragma once
#include "./c_interp_internal.c"

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
  Sint reg;
  try(int_stack_pop(&interp->ints, &imm));
  try(int_stack_pop(&interp->ints, &reg));

  aver(reg >= 0 && reg < ASM_REG_LEN);

  bool has_load = true;
  asm_append_imm_to_reg(&interp->comp, (U8)reg, imm, &has_load);
  if (has_load) sym->norm.has_loads = true;
  return nullptr;
}

static Err interp_validate_buf_ptr(Sint ptr, const U8 **out) {
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

static Err interp_pop_buf(Interp *interp, const U8 **out) {
  Sint buf;
  try(int_stack_pop(&interp->ints, &buf));
  try(interp_validate_buf_ptr(buf, out));
  return nullptr;
}

static Err interp_validate_buf_len(Sint len, Ind *out) {
  if (len > 0 && len < (Sint)IND_MAX) {
    *out = (Ind)len;
    return nullptr;
  }
  return errf("invalid data length: " FMT_SINT, len);
}

static Err interp_pop_len(Interp *interp, Ind *out) {
  Sint len;
  try(int_stack_pop(&interp->ints, &len));
  try(interp_validate_buf_len(len, out));
  return nullptr;
}

static Err interp_pop_reg(Interp *interp, U8 *out) {
  Sint reg;
  try(int_stack_pop(&interp->ints, &reg));
  try(asm_validate_reg(reg));
  if (out) *out = (U8)reg;
  return nullptr;
}

static Err intrin_comp_const(Interp *interp) {
  U8        reg;
  const U8 *buf;
  Ind       len;
  try(interp_pop_reg(interp, &reg));
  try(interp_pop_len(interp, &len));
  try(interp_pop_buf(interp, &buf));
  try(interp_comp_const(interp, buf, len, reg));
  return nullptr;
}

static Err intrin_comp_static(Interp *interp) {
  U8  reg;
  Ind len;
  try(interp_pop_reg(interp, &reg));
  try(interp_pop_len(interp, &len));
  try(interp_comp_static(interp, len, reg));
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
  return interp_parse_until(interp, (U8)delim);
}

static Err intrin_import(Interp *interp) {
  const auto ints = &interp->ints;
  Sint       path;

  try(int_stack_pop(ints, nullptr)); // discard length
  try(int_stack_pop(ints, &path));
  try(interp_import(interp, (const char *)path));
  return nullptr;
}

static Err intrin_extern_proc(Interp *interp) {
  const auto ints = &interp->ints;
  Sint       out_len;
  Sint       inp_len;
  try(int_stack_pop(ints, &out_len));
  try(int_stack_pop(ints, &inp_len));
  return interp_extern_proc(interp, inp_len, out_len);
}

static Err intrin_find_word(Interp *interp) {
  const U8 *buf;
  Ind       len;
  try(interp_pop_len(interp, &len));
  try(interp_pop_buf(interp, &buf));
  return interp_find_word(interp, (const char *)buf, len);
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
  const U8 *buf;
  Ind       len;
  Local    *loc;
  try(interp_pop_len(interp, &len));
  try(interp_pop_buf(interp, &buf));
  try(interp_get_local(interp, (const char *)buf, len, &loc));

  const auto tok = local_token(loc);
  try(int_stack_push(&interp->ints, tok));
  return nullptr;
}
