/*
This file defines assembly procedures for Arm64 for the variant of our Forth
system which uses the Forth data stack for its default calling convention.

Compare `./c_arch_arm64_cc_reg.c` which follows the native call convention.

Special registers:
- `x28` = interpreter pointer
- `x27` = top of integer stack
- `x26` = floor of integer stack
- `x0`  = error string pointer

The special registers are also hardcoded in `f_lang.f`.
*/
#pragma once
#include "./c_arch_arm64.c"
#include "./lib/err.h"
#include "./lib/num.h"

// Only works if the CWD matches this file's directory 😔.
__asm__(".include \"./c_arch_arm64_cc_stack.s\"");

// Defined in `./c_arch_arm64_cc_stack.s`.
extern Err asm_call_forth(Err err, void *fun, void *interp) __asm__(
  "asm_call_forth"
);

static Err interp_call_norm(Interp *interp, const Sym *sym) {
  try(comp_code_ensure_sym_ready(&interp->comp.code, sym));
  try(asm_call_forth(nullptr, sym->norm.exec.floor, interp));
  return nullptr;
}

static Err asm_append_instr_from_int(Comp *comp, Sint val) {
  try(imm_unsigned((Uint)val, sizeof(Instr) * CHAR_BIT));
  asm_append_instr(comp, (Instr)val);
  return nullptr;
}

static void asm_append_stack_push_from(Comp *comp, U8 reg) {
  asm_append_store_pre_post(comp, reg, ASM_REG_INT_TOP, 8);
}

static void asm_append_stack_pop_into(Comp *comp, U8 reg) {
  asm_append_load_pre_post(comp, reg, ASM_REG_INT_TOP, -8);
}

static void asm_append_stack_push_imm(Comp *comp, Sint imm) {
  /*
  mov <reg>, <imm>
  str <reg>, [x27], 8
  */
  asm_append_imm_to_reg(comp, ASM_SCRATCH_REG_8, imm, nullptr);
  asm_append_stack_push_from(comp, ASM_SCRATCH_REG_8);
}

// SYNC[asm_local_read].
static void asm_append_local_read(Comp *comp, Sint off) {
  asm_append_load_unscaled_offset(comp, ASM_SCRATCH_REG_8, ASM_REG_FP, off);
  asm_append_stack_push_from(comp, ASM_SCRATCH_REG_8);
}

// SYNC[asm_local_write].
static void asm_append_local_write(Comp *comp, Sint off) {
  asm_append_stack_pop_into(comp, ASM_SCRATCH_REG_8);
  asm_append_store_unscaled_offset(comp, ASM_SCRATCH_REG_8, ASM_REG_FP, off);
}

static void asm_append_call_intrin_before(Comp *comp, Uint ints_top_off) {
  asm_append_store_scaled_offset(
    comp, ASM_REG_INT_TOP, ASM_REG_INTERP, ints_top_off
  );
}

static void asm_append_call_intrin_after(Comp *comp, Uint ints_top_off) {
  asm_append_load_scaled_offset(
    comp, ASM_REG_INT_TOP, ASM_REG_INTERP, ints_top_off
  );
}

/*
TODO: words which contain such calls may not appear in compiled executables.
We detect these calls and set `Sym.interp_only` for later use.
*/
static void asm_append_call_intrin(
  Comp *comp, const Sym *caller, const Sym *callee, Uint ints_top_off
) {
  aver(callee->type == SYM_INTRIN);

  constexpr auto reg = ASM_SCRATCH_REG_8;

  asm_append_call_intrin_before(comp, ints_top_off);
  asm_append_mov_reg_to_reg(comp, ASM_PARAM_REG_0, ASM_REG_INTERP);
  asm_append_dysym_load(comp, callee->name.buf, reg);
  asm_append_branch_link_to_reg(comp, reg);
  asm_register_call(comp, caller);
  asm_append_call_intrin_after(comp, ints_top_off);

  if (callee->throws) asm_append_try(comp);
  else asm_append_zero_reg(comp, ASM_ERR_REG);
}

static void asm_append_load_extern_ptr(Comp *comp, const char *name) {
  constexpr auto reg = ASM_SCRATCH_REG_8;
  asm_append_dysym_load(comp, name, reg);
  asm_append_stack_push_from(comp, reg);
}

static void asm_append_call_extern_proc(
  Comp *comp, Sym *caller, const Sym *callee
) {
  aver(callee->type == SYM_EXT_PROC);

  const auto inp_len = callee->inp_len;
  const auto out_len = callee->out_len;

  aver(inp_len <= ASM_PARAM_REG_LEN);
  aver(out_len <= 1);

  // TODO use `ldp` for even pairs.
  if (inp_len > 7) asm_append_stack_pop_into(comp, ASM_PARAM_REG_7);
  if (inp_len > 6) asm_append_stack_pop_into(comp, ASM_PARAM_REG_6);
  if (inp_len > 5) asm_append_stack_pop_into(comp, ASM_PARAM_REG_5);
  if (inp_len > 4) asm_append_stack_pop_into(comp, ASM_PARAM_REG_4);
  if (inp_len > 3) asm_append_stack_pop_into(comp, ASM_PARAM_REG_3);
  if (inp_len > 2) asm_append_stack_pop_into(comp, ASM_PARAM_REG_2);
  if (inp_len > 1) asm_append_stack_pop_into(comp, ASM_PARAM_REG_1);
  if (inp_len > 0) asm_append_stack_pop_into(comp, ASM_PARAM_REG_0);

  constexpr auto reg = ASM_SCRATCH_REG_8;

  asm_append_dysym_load(comp, callee->name.buf, reg);
  asm_append_branch_link_to_reg(comp, reg);
  asm_register_call(comp, caller);
  if (out_len) asm_append_stack_push_from(comp, ASM_ERR_REG);
  asm_append_zero_reg(comp, ASM_ERR_REG);
}
