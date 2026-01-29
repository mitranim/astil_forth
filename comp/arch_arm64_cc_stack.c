/*
This file defines assembly procedures for Arm64 for the variant of our Forth
system which uses the Forth data stack for its calling convention.

Compare `./arch_arm64_cc_reg.c` which uses the native reg-based callvention.

Special registers:
- `x28` = interpreter pointer
- `x27` = top of integer stack
- `x26` = floor of integer stack
- `x0`  = error string pointer

The special registers are also hardcoded in `lang_s.f`.
*/
#pragma once
#include "./interp.h"
#include "./lib/err.h"
#include "./lib/num.h"

#ifdef CLANGD
#include "./arch_arm64.c"
#endif

// `.include` doesn't support file-relative paths 😔.
__asm__(".include \"comp/arch_arm64_cc_stack.s\"");

// Defined in `./arch_arm64_cc_stack.s`.
extern Err asm_call_forth(Err err, const Instr *fun, void *interp) __asm__(
  "asm_call_forth"
);

static Err arch_call_norm(Interp *interp, const Sym *sym) {
  const auto fun = comp_sym_exec_instr(&interp->comp, sym);
  return asm_call_forth(nullptr, fun, interp);
}

static Err asm_append_instr_from_int(Comp *comp, Sint val) {
  try(imm_unsigned((Uint)val, sizeof(Instr) * CHAR_BIT));
  asm_append_instr(comp, (Instr)val);
  return nullptr;
}

static void asm_append_stack_push_from(Comp *comp, U8 reg) {
  asm_append_store_pre_post(comp, reg, ARCH_REG_INT_TOP, 8);
}

static void asm_append_stack_pop_into(Comp *comp, U8 reg) {
  asm_append_load_pre_post(comp, reg, ARCH_REG_INT_TOP, -8);
}

static void asm_append_stack_push_imm(Comp *comp, Sint imm) {
  /*
  mov <reg>, <imm>
  str <reg>, [x27], 8
  */
  asm_append_imm_to_reg(comp, ARCH_SCRATCH_REG_8, imm, nullptr);
  asm_append_stack_push_from(comp, ARCH_SCRATCH_REG_8);
}

// TODO: for both callventions: detect when local offsets don't fit
// into `ldur/stur`. We can either produce an error, or emit different
// instructions which allow for a larger negative offset.
//
// SYNC[asm_local_read].
static void asm_append_local_read(Comp *comp, Sint off) {
  asm_append_load_unscaled_offset(comp, ARCH_SCRATCH_REG_8, ARCH_REG_FP, off);
  asm_append_stack_push_from(comp, ARCH_SCRATCH_REG_8);
}

// SYNC[asm_local_write].
static void asm_append_local_write(Comp *comp, Sint off) {
  asm_append_stack_pop_into(comp, ARCH_SCRATCH_REG_8);
  asm_append_store_unscaled_offset(comp, ARCH_SCRATCH_REG_8, ARCH_REG_FP, off);
}

static void asm_append_call_intrin_before(Comp *comp) {
  asm_append_store_scaled_offset(
    comp, ARCH_REG_INT_TOP, ARCH_REG_INTERP, INTERP_INTS_TOP
  );
}

static void asm_append_call_intrin_after(Comp *comp) {
  asm_append_load_scaled_offset(
    comp, ARCH_REG_INT_TOP, ARCH_REG_INTERP, INTERP_INTS_TOP
  );
}

/*
TODO: words which contain such calls may not appear in compiled executables.
We detect these calls and set `Sym.interp_only` for later use.
*/
static void asm_append_call_intrin(
  Comp *comp, const Sym *caller, const Sym *callee
) {
  IF_DEBUG(aver(callee->type == SYM_INTRIN));

  constexpr auto reg = ARCH_SCRATCH_REG_8;

  asm_append_call_intrin_before(comp);
  asm_append_mov_reg(comp, ARCH_PARAM_REG_0, ARCH_REG_INTERP);
  asm_append_dysym_load(comp, callee->name.buf, reg);
  asm_append_branch_link_to_reg(comp, reg);
  asm_register_call(comp, caller);
  asm_append_call_intrin_after(comp);

  if (callee->throws) asm_append_try(comp);
  else asm_append_zero_reg(comp, ARCH_REG_ERR);
}

static void asm_append_call_extern(Comp *comp, Sym *caller, const Sym *callee) {
  IF_DEBUG(aver(callee->type == SYM_EXTERN));

  const auto inp_len = callee->inp_len;
  const auto out_len = callee->out_len;

  IF_DEBUG(aver(inp_len <= ARCH_INP_PARAM_REG_LEN));
  IF_DEBUG(aver(out_len <= 1));

  // TODO use `ldp` for even pairs.
  if (inp_len > 7) asm_append_stack_pop_into(comp, ARCH_PARAM_REG_7);
  if (inp_len > 6) asm_append_stack_pop_into(comp, ARCH_PARAM_REG_6);
  if (inp_len > 5) asm_append_stack_pop_into(comp, ARCH_PARAM_REG_5);
  if (inp_len > 4) asm_append_stack_pop_into(comp, ARCH_PARAM_REG_4);
  if (inp_len > 3) asm_append_stack_pop_into(comp, ARCH_PARAM_REG_3);
  if (inp_len > 2) asm_append_stack_pop_into(comp, ARCH_PARAM_REG_2);
  if (inp_len > 1) asm_append_stack_pop_into(comp, ARCH_PARAM_REG_1);
  if (inp_len > 0) asm_append_stack_pop_into(comp, ARCH_PARAM_REG_0);

  constexpr auto reg = ARCH_SCRATCH_REG_8;

  asm_append_dysym_load(comp, callee->name.buf, reg);
  asm_append_branch_link_to_reg(comp, reg);
  asm_register_call(comp, caller);
  if (out_len) asm_append_stack_push_from(comp, ARCH_REG_ERR);
  asm_append_zero_reg(comp, ARCH_REG_ERR);
}
