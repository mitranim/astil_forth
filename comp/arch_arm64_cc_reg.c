/*
This file defines assembly procedures for Arm64 for the variant
of our Forth system which uses the native procedure call ABI.

Compare `./arch_arm64_cc_stack.c` which uses a stack-based callvention.

Special registers:
- `x28` = error string pointer.
- `x27` = interpreter pointer.

The special registers must be kept in sync with `f_lang_r.f`.
*/
#pragma once
#include "./arch_arm64.h"
#include "./interp.h"
#include "./lib/bits.c"

#ifdef CLANGD
#include "./arch_arm64.c"
#endif

static Err err_call_arity_mismatch(const char *name, U8 inp_len, Sint ints_len) {
  return errf(
    "unable to call symbol %s: %d input parameters required, but only " FMT_SINT
    " are available on the data stack\n",
    name,
    inp_len,
    ints_len
  );
}

Err arch_call_norm(Interp *interp, const Sym *sym) {
  const auto fun     = comp_sym_exec_instr(&interp->comp, sym);
  const auto inp_len = sym->inp_len;
  const auto out_len = sym->out_len;

  aver(inp_len <= ARCH_INP_PARAM_REG_LEN);
  aver(out_len <= ARCH_OUT_PARAM_REG_LEN);

  const auto ints     = &interp->ints;
  const auto ints_len = stack_len(ints);

  if (inp_len > ints_len) {
    return err_call_arity_mismatch(sym->name.buf, inp_len, ints_len);
  }

  Sint register x7 __asm__("x7") = inp_len > 7 ? stack_pop(ints) : 0;
  Sint register x6 __asm__("x6") = inp_len > 6 ? stack_pop(ints) : 0;
  Sint register x5 __asm__("x5") = inp_len > 5 ? stack_pop(ints) : 0;
  Sint register x4 __asm__("x4") = inp_len > 4 ? stack_pop(ints) : 0;
  Sint register x3 __asm__("x3") = inp_len > 3 ? stack_pop(ints) : 0;
  Sint register x2 __asm__("x2") = inp_len > 2 ? stack_pop(ints) : 0;
  Sint register x1 __asm__("x1") = inp_len > 1 ? stack_pop(ints) : 0;
  Sint register x0 __asm__("x0") = inp_len > 0 ? stack_pop(ints) : 0;

  auto register reg_interp __asm__("x27") = interp;
  Err register reg_err __asm__("x28")     = nullptr;

  /*
  This construct lets us support:
  - Multiple output parameters.
  - Special interpreter register.
  - Special error register.

  Our alternative stack-based calling convention uses an additional
  trampoline written in assembly, but we don't need one here yet.

  SYNC[arch_arm64_cc_reg_special_regs].
  */
  __asm__ volatile("blr %[fun]\n"
                   : "+r"(x0),
                     "+r"(x1),
                     "+r"(x2),
                     "+r"(x3),
                     "+r"(x4),
                     "+r"(x5),
                     "+r"(x6),
                     "+r"(x7),
                     "+r"(reg_err)
                   : [fun] "r"(fun), "r"(reg_interp)
                   : "x8",
                     "x9",
                     "x10",
                     "x11",
                     "x12",
                     "x13",
                     "x14",
                     "x15",
                     "x16",
                     "x17",
                     "x19",
                     "x20",
                     "x21",
                     "x22",
                     "x23",
                     "x24",
                     "x25",
                     "x26",
                     "cc",
                     "memory");

  if (out_len > 0) try(int_stack_push(ints, x0));
  if (out_len > 1) try(int_stack_push(ints, x1));
  if (out_len > 2) try(int_stack_push(ints, x2));
  if (out_len > 3) try(int_stack_push(ints, x3));
  if (out_len > 4) try(int_stack_push(ints, x4));
  if (out_len > 5) try(int_stack_push(ints, x5));
  if (out_len > 6) try(int_stack_push(ints, x6));
  if (out_len > 7) try(int_stack_push(ints, x7));
  return reg_err;
}

static Err arch_call_intrin(Interp *interp, const Sym *sym) {
  aver(sym->type == SYM_INTRIN);

  const auto ints = &interp->ints;
  auto       ind  = sym->inp_len;
  aver(ind < ARCH_INP_PARAM_REG_LEN);

  Sint args[ARCH_INP_PARAM_REG_LEN] = {};
  args[ind]                         = (Sint)interp;

  while (ind) {
    ind--;
    try(int_stack_pop(ints, &args[ind]));
  }

  Sint x0 = args[0];
  Sint x1 = args[1];
  Sint x2 = args[2];
  Sint x3 = args[3];
  Sint x4 = args[4];
  Sint x5 = args[5];
  Sint x6 = args[6];
  Sint x7 = args[7];

  /*
  So we've just prepared the inputs, where are the outputs?
  For now, intrinsics push outputs into the Forth data stack,
  just like in the purely stack-based callvention.
  */

  typedef Err(Fun)(Sint, Sint, Sint, Sint, Sint, Sint, Sint, Sint);
  const auto fun = (Fun *)sym->intrin;
  const auto err = fun(x0, x1, x2, x3, x4, x5, x6, x7);
  return sym->throws ? err : nullptr;
}

static void asm_append_call_intrin_after(Comp *comp, const Sym *callee) {
  if (callee->throws) {
    // Our C procedures return errors in `x0`, while in Forth,
    // we dedicate a callee-saved register to errors.
    asm_append_mov_reg(comp, ARCH_REG_ERR, ARCH_PARAM_REG_0);
    asm_append_try(comp);
  }

  const auto len = callee->out_len;
  if (!len) return;

  /*
  We have one or two intrinsics which want to return `(val0 val1 err)`.
  Unfortunately, C does not support ABI-native multi-output parameters.
  Returning a struct is quirky because the behavior depends on how many
  fields it has. So for now, intrinsic outputs are just pushed into the
  Forth data stack. This also makes it easier to reuse them between CC.

    ldr x8, [x27, INTERP_INTS_TOP]
    ... ldr xN, [x8, -8]! ...
    str x8, [x27, INTERP_INTS_TOP]
  */

  constexpr auto stack_reg = ARCH_SCRATCH_REG_8;
  auto           out_reg   = len;
  aver(out_reg < ARCH_OUT_PARAM_REG_LEN);

  asm_append_load_unscaled_offset(
    comp, stack_reg, ARCH_REG_INTERP, INTERP_INTS_TOP
  );
  while (out_reg) {
    out_reg--;
    asm_append_load_pre_post(comp, out_reg, stack_reg, -8);
  }
  asm_append_store_unscaled_offset(
    comp, stack_reg, ARCH_REG_INTERP, INTERP_INTS_TOP
  );
}

static void asm_append_call_intrin(Comp *comp, Sym *caller, const Sym *callee) {
  aver(callee->type == SYM_INTRIN);

  // Free to use because intrin calls clobber everything anyway.
  constexpr auto reg = ARCH_SCRATCH_REG_8;

  // Under this callvention, our intrinsics always take `Interp*`
  // as an additional "secret" parameter following immediately
  // after the "official" parameters.
  asm_append_mov_reg(comp, callee->inp_len, ARCH_REG_INTERP);
  asm_append_dysym_load(comp, callee->name.buf, reg);
  asm_append_branch_link_to_reg(comp, reg);
  asm_register_call(comp, caller);
  asm_append_call_intrin_after(comp, callee);
}

static void asm_append_call_extern(Comp *comp, Sym *caller, const Sym *callee) {
  aver(callee->type == SYM_EXTERN);

  // Free to use because extern calls clobber everything anyway.
  constexpr auto reg = ARCH_SCRATCH_REG_8;

  asm_append_dysym_load(comp, callee->name.buf, reg);
  asm_append_branch_link_to_reg(comp, reg);
  asm_register_call(comp, caller);
}

static Instr asm_instr_local_read(Local *loc, U8 tar_reg) {
  IF_DEBUG(aver(loc->location != LOC_UNKNOWN));

  switch (loc->location) {
    case LOC_REG: {
      return asm_instr_mov_reg(tar_reg, loc->reg);
    }
    case LOC_MEM: {
      const auto off = local_fp_off(loc);
      return asm_instr_load_scaled_offset(tar_reg, ARCH_REG_FP, off);
    }
    case LOC_UNKNOWN: [[fallthrough]];
    default:          unreachable();
  }
}

static Instr asm_instr_local_write(Local *loc, U8 src_reg) {
  IF_DEBUG(aver(loc->location != LOC_UNKNOWN));

  switch (loc->location) {
    case LOC_REG: {
      return asm_instr_mov_reg(loc->reg, src_reg);
    }
    case LOC_MEM: {
      const auto off = local_fp_off(loc);
      return asm_instr_store_scaled_offset(src_reg, ARCH_REG_FP, off);
    }
    case LOC_UNKNOWN: [[fallthrough]];
    default:          unreachable();
  }
}

/*
We accumulate volatile clobbers from ALL sources across the entire procedure
and always treat these registers as temporary, meaning that no locals across
the procedure receive them as their locations, even when their lifetimes do
not overlap with clobbers. Dirty but simple way of avoiding an additional IR
and data flow analysis.
*/
static void arch_resolve_local_location(Comp *comp, Local *loc, Sym *sym) {
  if (loc->location != LOC_UNKNOWN) return;

  const auto reg = bits_pop_low(&comp->ctx.vol_regs);

  if (bits_has(ARCH_VOLATILE_REGS, reg)) {
    loc->location = LOC_REG;
    loc->reg      = reg;
    bits_add_to(&sym->clobber, reg);
    return;
  }

  comp_local_alloc_mem(comp, loc);
  loc->location = LOC_MEM;
}

static void asm_fixup_loc_read(Comp *comp, Loc_fixup *fix, Sym *sym) {
  IF_DEBUG(aver(fix->type == LOC_FIX_READ));

  const auto read = &fix->read;
  const auto loc  = read->loc;

  arch_resolve_local_location(comp, loc, sym);
  *read->instr = asm_instr_local_read(loc, read->reg);
}

static void asm_fixup_loc_write(Comp *comp, Loc_fixup *fix, Sym *sym) {
  IF_DEBUG(aver(fix->type == LOC_FIX_WRITE));

  const auto write = &fix->write;

  // This VERY dirty hack allows us to preserve the simplicity of forward-only
  // single-pass assembly, without having to invent an IR and have the library
  // and program code deal with an extra API. Nops are cheap and significantly
  // better than memory ops anyway.
  if (!write->confirmed) {
    const auto instr = write->instr;
    if (asm_skipped_prologue_instr(comp, sym, instr)) return;
    *instr = ASM_INSTR_NOP;
    return;
  }

  const auto loc = write->loc;
  arch_resolve_local_location(comp, loc, sym);
  *write->instr = asm_instr_local_write(loc, write->reg);
}

static void asm_fixup_locals(Comp *comp, Sym *sym) {
  const auto ctx = &comp->ctx;

  // Remaining volatile registers available for locals.
  aver(ctx->vol_regs == BITS_ALL);
  ctx->vol_regs = bits_del_all(ARCH_VOLATILE_REGS, sym->clobber);

  for (stack_range(auto, fix, &ctx->loc_fix)) {
    switch (fix->type) {
      case LOC_FIX_READ: {
        asm_fixup_loc_read(comp, fix, sym);
        continue;
      }
      case LOC_FIX_WRITE: {
        asm_fixup_loc_write(comp, fix, sym);
        continue;
      }
      default: unreachable();
    }
  }
}
