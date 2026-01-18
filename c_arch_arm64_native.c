/*
This file defines assembly procedures for Arm64 for the variant of our Forth
system which uses the native procedure call ABI.

Compare `./c_arch_arm64_stack.c` which uses stack-based conventions.

Special registers:
- `x28` = error string pointer.
- `x27` = interpreter pointer.

The special registers must be kept in sync with `f_lang_native.f`.
*/
#pragma once
#include "./c_arch_arm64.c"

/*
FIXME:

Everything below needs translating into the new convention,
and renaming both here and in the other file to consolidate
and abstract this for the other files.
*/

static Err interp_call_norm(Interp *interp, const Sym *sym) {
  try(comp_code_ensure_sym_ready(&interp->comp.code, sym));

  // transfer args from control stack
  // put them in registers
  unreachable();
}

// static void asm_append_stack_pop_into(Comp *comp, U8 reg) {
//   asm_append_load_pre_post(comp, reg, ASM_REG_INT_TOP, -8);
// }

// static Err err_unrec_local_ind(Sint ind) {
//   return errf("unrecognized local index: " FMT_SINT, ind);
// }

// static Err asm_validate_local_ind(const Asm_locs *locs, Sint ind) {
//   if (ind >= 0 && ind < locs->next) return nullptr;
//   return err_unrec_local_ind(ind);
// }

// static Err asm_append_local_pop(Comp *comp, Sint ind) {
//   const auto locs = &comp->locs;
//   try(asm_validate_local_ind(locs, ind));

//   const auto off = asm_local_fp_off((Ind)ind);
//   asm_append_stack_pop_into(comp, ASM_SCRATCH_REG_8);
//   asm_append_store_unscaled_offset(comp, ASM_SCRATCH_REG_8, ASM_REG_FP, off);
//   return nullptr;
// }

// static Err asm_append_local_push(Comp *comp, Sint ind) {
//   const auto locs = &comp->locs;
//   try(asm_validate_local_ind(locs, ind));

//   const auto off = asm_local_fp_off((Ind)ind);
//   asm_append_load_unscaled_offset(comp, ASM_SCRATCH_REG_8, ASM_REG_FP, off);
//   asm_append_stack_push_from(comp, ASM_SCRATCH_REG_8);
//   return nullptr;
// }

// static bool asm_appended_local_push(Comp *comp, const char *name) {
//   const auto locs = &comp->locs;
//   const auto inds = &locs->inds;
//   if (!dict_has(inds, name)) return false;
//   averr(asm_append_local_push(comp, dict_get(inds, name)));
//   return true;
// }

// static void asm_append_stack_push_imm(Comp *comp, Sint imm) {
//   /*
//   mov <reg>, <imm>
//   str <reg>, [x27], 8
//   */
//   asm_append_imm_to_reg(comp, ASM_SCRATCH_REG_8, imm, nullptr);
//   asm_append_stack_push_from(comp, ASM_SCRATCH_REG_8);
// }

// static void asm_append_call_intrin_before(Comp *comp, Uint ints_top_off) {
//   asm_append_store_scaled_offset(
//     heap, ASM_REG_INT_TOP, ASM_REG_INTERP, ints_top_off
//   );
// }

// static void asm_append_call_intrin_after(Comp *comp, Uint ints_top_off) {
//   asm_append_load_scaled_offset(
//     heap, ASM_REG_INT_TOP, ASM_REG_INTERP, ints_top_off
//   );
// }

// /*
// TODO: words which contain such calls may not appear in compiled executables.
// We detect these calls and set `Sym.interp_only` for later use.
// */
// static void asm_append_call_intrin(
//   Comp *comp, const Sym *caller, const Sym *callee, Uint ints_top_off
// ) {
//   aver(callee->type == SYM_INTRIN);

//   constexpr auto reg = ASM_SCRATCH_REG_8;

//   asm_append_call_intrin_before(comp, ints_top_off);
//   asm_append_mov_reg_to_reg(comp, ASM_PARAM_REG_0, ASM_REG_INTERP);
//   asm_append_dysym_load(comp, callee->name.buf, reg);
//   asm_append_branch_link_to_reg(comp, reg);
//   asm_register_call(comp, caller);
//   asm_append_call_intrin_after(comp, ints_top_off);

//   if (callee->throws) asm_append_try(comp);
//   else asm_append_zero_reg(comp, ASM_ERR_REG);
// }

// static void asm_append_load_extern_ptr(Comp *comp, const char *name) {
//   constexpr auto reg = ASM_SCRATCH_REG_8;
//   asm_append_dysym_load(comp, name, reg);
//   asm_append_stack_push_from(comp, reg);
// }

// static void asm_append_call_extern_proc(Comp *comp, Sym *caller, const Sym *callee) {
//   aver(callee->type == SYM_EXT_PROC);

//   const auto inp_len = callee->ext_proc.inp_len;
//   const auto out_len = callee->ext_proc.out_len;

//   aver(inp_len <= ASM_PARAM_REG_LEN);
//   aver(out_len <= 1);

//   // TODO use `ldp` for even pairs.
//   if (inp_len > 7) asm_append_stack_pop_into(comp, ASM_PARAM_REG_7);
//   if (inp_len > 6) asm_append_stack_pop_into(comp, ASM_PARAM_REG_6);
//   if (inp_len > 5) asm_append_stack_pop_into(comp, ASM_PARAM_REG_5);
//   if (inp_len > 4) asm_append_stack_pop_into(comp, ASM_PARAM_REG_4);
//   if (inp_len > 3) asm_append_stack_pop_into(comp, ASM_PARAM_REG_3);
//   if (inp_len > 2) asm_append_stack_pop_into(comp, ASM_PARAM_REG_2);
//   if (inp_len > 1) asm_append_stack_pop_into(comp, ASM_PARAM_REG_1);
//   if (inp_len > 0) asm_append_stack_pop_into(comp, ASM_PARAM_REG_0);

//   constexpr auto reg = ASM_SCRATCH_REG_8;

//   asm_append_dysym_load(comp, callee->name.buf, reg);
//   asm_append_branch_link_to_reg(comp, reg);
//   asm_register_call(comp, caller);
//   if (out_len) asm_append_stack_push_from(comp, ASM_ERR_REG);
//   asm_append_zero_reg(comp, ASM_ERR_REG);
// }
