#pragma once

#ifdef CLANGD
#include "./c_intrin.c"
#endif // CLANGD

static Err intrin_comp_instr(Instr instr, Interp *interp) {
  return asm_append_instr(&interp->comp, instr);
}

static Err intrin_comp_load(Sint reg, Sint imm, Interp *interp) {
  Sym *sym;
  try(interp_require_current_sym(interp, &sym));
  try(asm_validate_reg(reg));

  bool has_load = true;
  asm_append_imm_to_reg(&interp->comp, (U8)reg, imm, &has_load);
  if (has_load) sym->norm.has_loads = true;
  return nullptr;
}

static Err interp_validate_buf_ptr(Sint val) {
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
  if (val > (1 << 14)) return nullptr;
  return errf("suspiciously invalid-looking data pointer: %p", (U8 *)ptr);
}

static Err interp_validate_buf_len(Sint val) {
  if (val > 0 && val < (Sint)IND_MAX) return nullptr;
  return errf("invalid data length: " FMT_SINT, val);
}

// TODO: either provide a way to align, or auto-align to `sizeof(void*)`.
static Err intrin_comp_const(Sint buf, Sint len, Sint reg, Interp *interp) {
  try(interp_validate_buf_ptr(buf));
  try(interp_validate_buf_len(len));
  try(asm_validate_reg(reg));
  try(interp_comp_const(interp, (const U8 *)buf, (Ind)len, (U8)reg));
  return nullptr;
}

static Err intrin_comp_static(Sint len, Sint reg, Interp *interp) {
  try(interp_validate_buf_len(len));
  try(asm_validate_reg(reg));
  try(interp_comp_static(interp, (Ind)len, (U8)reg));
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

static Err intrin_comp_call(Sint ptr, Interp *interp) {
  Sym *sym;
  try(interp_sym_by_ptr(interp, ptr, &sym));
  return comp_append_call_sym(&interp->comp, sym);
}

static Err intrin_parse(Sint delim, Interp *interp) {
  try(validate_char_ascii_printable(delim));
  try(interp_parse_until(interp, (U8)delim));
  return nullptr;
}

static Err intrin_import(Sint buf, Sint len, Interp *interp) {
  try(interp_validate_buf_ptr(buf));
  try(interp_validate_buf_len(len));

  const auto path = (const char *)buf;
  if (DEBUG) aver((Sint)strlen(path) == len);
  try(interp_import(interp, path)) return nullptr;
}

static Err intrin_extern_proc(Sint out_len, Sint inp_len, Interp *interp) {
  try(interp_extern_proc(interp, inp_len, out_len));
  return nullptr;
}

static Err intrin_find_word(Sint buf, Sint len, Interp *interp) {
  try(interp_validate_buf_ptr(buf));
  try(interp_validate_buf_len(len));
  try(interp_find_word(interp, (const char *)buf, (Ind)len));
  return nullptr;
}

static Err intrin_inline_word(Sint ptr, Interp *interp) {
  Sym *sym;
  try(interp_sym_by_ptr(interp, ptr, &sym));
  try(comp_inline_sym(&interp->comp, sym));
  return nullptr;
}

static Err intrin_execute(Sint ptr, Interp *interp) {
  Sym *sym;
  try(interp_sym_by_ptr(interp, ptr, &sym));
  try(interp_call_sym(interp, sym));
  return nullptr;
}

// FIXME: probably need `Local*` instead of FP offset.
static Err intrin_get_local(Sint buf, Sint len, Interp *interp) {
  try(interp_validate_buf_ptr(buf));
  try(interp_validate_buf_len(len));
  try(interp_get_local(interp, (const char *)buf, (Ind)len));
  return nullptr;
}

// FIXME use in `f_lang_cc_reg.f` instead of custom asm.
static Err intrin_comp_local_get(Sint ptr, Interp *interp) {
  const auto comp = &interp->comp;
  Local     *loc;
  try(comp_validate_local(comp, ptr, &loc));
  try(comp_append_local_get(comp, loc));
  return nullptr;
}

// FIXME use in `f_lang_cc_reg.f` instead of custom asm.
static Err intrin_comp_local_set(Interp *interp) {
  const auto comp = &interp->comp;
  Local     *loc;
  try(comp_validate_local(comp, ptr, &loc));
  try(comp_append_local_set(comp, loc));
  return nullptr;
}

static constexpr auto INTRIN_COMP_LOCAL_GET = (Sym){
  .type        = SYM_INTRIN,
  .name.buf    = "comp_local_get",
  .intrin      = (void *)intrin_comp_local_get,
  .throws      = true,
  .comp_only   = true,
  .interp_only = true,
};

static constexpr auto INTRIN_COMP_LOCAL_SET = (Sym){
  .type        = SYM_INTRIN,
  .name.buf    = "comp_local_set",
  .intrin      = (void *)intrin_comp_local_set,
  .throws      = true,
  .comp_only   = true,
  .interp_only = true,
};

static constexpr Sym USED INTRIN[] = {
  INTRIN_COLON,           INTRIN_SEMICOLON,     INTRIN_BRACKET_BEG,
  INTRIN_BRACKET_END,     INTRIN_IMMEDIATE,     INTRIN_COMP_ONLY,
  INTRIN_NOT_COMP_ONLY,   INTRIN_INLINE,        INTRIN_THROWS,
  INTRIN_REDEFINE,        INTRIN_HERE,          INTRIN_COMP_INSTR,
  INTRIN_COMP_LOAD,       INTRIN_COMP_CONST,    INTRIN_COMP_STATIC,
  INTRIN_COMP_CALL,       INTRIN_QUIT,          INTRIN_RET,
  INTRIN_RECUR,           INTRIN_CHAR,          INTRIN_PARSE,
  INTRIN_PARSE_WORD,      INTRIN_IMPORT,        INTRIN_IMPORT_QUOTE,
  INTRIN_IMPORT_TICK,     INTRIN_EXTERN_PTR,    INTRIN_EXTERN_PROC,
  INTRIN_FIND_WORD,       INTRIN_INLINE_WORD,   INTRIN_EXECUTE,
  INTRIN_GET_LOCAL,       INTRIN_ANON_LOCAL,    INTRIN_COMP_LOCAL_GET,
  INTRIN_COMP_LOCAL_SET,  INTRIN_DEBUG_FLUSH,   INTRIN_DEBUG_THROW,
  INTRIN_DEBUG_STACK_LEN, INTRIN_DEBUG_STACK,   INTRIN_DEBUG_DEPTH,
  INTRIN_DEBUG_TOP_INT,   INTRIN_DEBUG_TOP_PTR, INTRIN_DEBUG_TOP_STR,
  INTRIN_DEBUG_MEM,       INTRIN_DEBUG_WORD,    INTRIN_DEBUG_SYNC_CODE,
};
