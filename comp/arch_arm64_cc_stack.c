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
#include "./sym.h"

#ifdef CLANGD
#include "./arch_arm64.c"
#endif

// `.include` doesn't support file-relative paths 😔.
__asm__(".include \"comp/arch_arm64_cc_stack.s\"");

// Defined in `./arch_arm64_cc_stack.s`.
extern Err asm_call_forth(Err err, const Instr *fun, void *interp) __asm__(
  "asm_call_forth"
);

static Err asm_call_norm(Interp *interp, const Sym *sym) {
  const auto fun = comp_sym_exec_instr(&interp->comp, sym);
  return asm_call_forth(nullptr, fun, interp);
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
static void asm_append_local_read(Comp *comp, Ind off) {
  asm_append_load_scaled_offset(comp, ASM_SCRATCH_REG_8, ASM_REG_FP, off);
  asm_append_stack_push_from(comp, ASM_SCRATCH_REG_8);
}

// SYNC[asm_local_write].
static void asm_append_local_write(Comp *comp, Ind off) {
  asm_append_stack_pop_into(comp, ASM_SCRATCH_REG_8);
  asm_append_store_scaled_offset(comp, ASM_SCRATCH_REG_8, ASM_REG_FP, off);
}

static S8 asm_sym_err_reg(const Sym *sym) {
  if (!sym->throws) return -1;
  return ASM_REG_ERR;
}

/*
Callers which don't officially throw nevertheless often clobber the error
register. In reg-CC, we track the clobbers, whereas in stack-CC we simply
always zero the register, so that outer callers don't mistake its clobber
value for an error.
*/
static void asm_append_sym_epilogue_ok(Comp *comp, Sym *sym) {
  if (!bits_has(sym->clobber, ASM_REG_ERR)) return;
  asm_append_zero_reg(comp, ASM_REG_ERR);
}

static void asm_append_pop_try(Comp *comp) {
  asm_append_stack_pop_into(comp, ASM_REG_ERR);
  asm_append_fixup_try(comp, ASM_REG_ERR); // cbnz <err>, <err_epi>
}

static void asm_append_pop_throw(Comp *comp) {
  asm_append_stack_pop_into(comp, ASM_REG_ERR);
  asm_append_fixup_throw(comp); // b <err_epi>
}

// Must be used after compiling every call to a non-extern procedure.
static Err asm_append_try_catch(
  Comp *comp, Sym *caller, const Sym *callee, bool force_catch
) {
  if (force_catch) {
    if (!callee->throws) {
      return err_catch_no_throw(callee->name.buf);
    }

    // Don't need to zero the error register here,
    // because that's done in the word epilogue in
    // the success case.
    asm_append_stack_push_from(comp, ASM_REG_ERR);
    return nullptr;
  }

  if (callee->throws) {
    caller->throws = true;
    asm_append_fixup_try(comp, ASM_REG_ERR); // cbnz <err>, <err_epi>
    return nullptr;
  }

  /*
  In stack-CC, `catches` makes EVERY call return an error value.
  If the callee doesn't throw, the error is always nil but it's
  on the stack regardless. This avoids having to remember which
  words throw and which ones don't; check error after each call.
  */
  if (caller->catches) {
    // `sp` acts as a zero value here.
    asm_append_stack_push_from(comp, ASM_REG_SP);
    return nullptr;
  }

  return nullptr;
}

static void asm_append_call_intrin_before(Comp *comp) {
  asm_append_store_scaled_offset(
    comp, ASM_REG_INT_TOP, ASM_REG_INTERP, INTERP_INTS_TOP
  );
}

static void asm_append_call_intrin_after(Comp *comp) {
  asm_append_load_scaled_offset(
    comp, ASM_REG_INT_TOP, ASM_REG_INTERP, INTERP_INTS_TOP
  );
}

/*
TODO: words which contain such calls may not appear in compiled executables.
We detect these calls and set `Sym.interp_only` for later use.
*/
static Err asm_append_call_intrin(
  Comp *comp, Sym *caller, const Sym *callee, bool catch
) {
  IF_DEBUG(aver(callee->type == SYM_INTRIN));

  constexpr auto reg = ASM_SCRATCH_REG_8;

  asm_append_call_intrin_before(comp);
  asm_append_mov_reg(comp, ASM_PARAM_REG_0, ASM_REG_INTERP);
  asm_append_dysym_load(comp, callee->name.buf, reg);
  asm_append_branch_link_to_reg(comp, reg);
  asm_register_call(comp, caller);
  asm_append_call_intrin_after(comp);
  try(asm_append_try_catch(comp, caller, callee, catch));
  return nullptr;
}

static void asm_append_call_extern(Comp *comp, Sym *caller, const Sym *callee) {
  IF_DEBUG(aver(callee->type == SYM_EXTERN));

  const auto inp_len = callee->inp_len;
  const auto out_len = callee->out_len;

  IF_DEBUG(aver(inp_len <= ASM_INP_PARAM_REG_LEN));
  IF_DEBUG(aver(out_len <= 1));

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
  if (out_len) asm_append_stack_push_from(comp, ASM_REG_ERR);

  /*
  Extern procedures always clobber the error register. We don't need to
  zero it here, because that's done in the epilogue in the success case.
  See `asm_append_sym_epilogue_ok`.
  */
}
