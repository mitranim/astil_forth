/*
This file defines assembly procedures for Arm64 for the variant
of our Forth system which uses the native procedure call ABI.

Compare `./arch_arm64_cc_stack.c` which uses a stack-based callvention.

Special registers:
- `x28` = pointer to interpreter / compiler.

The special registers must be kept in sync with `lang_r.f`.
*/
#pragma once
#include "./arch_arm64.h"
#include "./interp.h"
#include "./lib/bits.c"
#include "./sym.h"

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

static Err arch_call_norm(Interp *interp, const Sym *sym) {
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

  Sint register x7 __asm__("x7")          = inp_len > 7 ? stack_pop(ints) : 0;
  Sint register x6 __asm__("x6")          = inp_len > 6 ? stack_pop(ints) : 0;
  Sint register x5 __asm__("x5")          = inp_len > 5 ? stack_pop(ints) : 0;
  Sint register x4 __asm__("x4")          = inp_len > 4 ? stack_pop(ints) : 0;
  Sint register x3 __asm__("x3")          = inp_len > 3 ? stack_pop(ints) : 0;
  Sint register x2 __asm__("x2")          = inp_len > 2 ? stack_pop(ints) : 0;
  Sint register x1 __asm__("x1")          = inp_len > 1 ? stack_pop(ints) : 0;
  Sint register x0 __asm__("x0")          = inp_len > 0 ? stack_pop(ints) : 0;
  auto register interp_reg __asm__("x28") = interp;

  /*
  This construct lets us support:

  - Multiple output parameters.
  - Returning an error / exception as an additional output.
  - Special interpreter register.

  Our alternative stack-based calling convention uses an additional
  trampoline written in assembly, but we don't need one here yet.

  Callee-saved registers are listed here as clobbers
  because otherwise we get weird rare crashes, which
  shouldn't really happen because our code generally
  respects the platform ABI. TODO figure out why.
  */
  __asm__ volatile("blr %[fun]\n"
                   : "+r"(x0),
                     "+r"(x1),
                     "+r"(x2),
                     "+r"(x3),
                     "+r"(x4),
                     "+r"(x5),
                     "+r"(x6),
                     "+r"(x7)
                   : [fun] "r"(fun), "r"(interp_reg)
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
                     "x27",
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

  Err err = nullptr;

  if (sym->err == ERR_MODE_THROW) {
    switch (out_len) {
      case 0: {
        err = (Err)x0;
        break;
      }
      case 1: {
        err = (Err)x1;
        break;
      }
      case 2: {
        err = (Err)x2;
        break;
      }
      case 3: {
        err = (Err)x3;
        break;
      }
      case 4: {
        err = (Err)x4;
        break;
      }
      case 5: {
        err = (Err)x5;
        break;
      }
      case 6: {
        err = (Err)x6;
        break;
      }
      case 7: {
        err = (Err)x7;
        break;
      }
      default: unreachable();
    }
  }

  return err;
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

  TODO: revise the convention; use callee-allocated stack pointers.
  */
  typedef Err(Fun)(Sint, Sint, Sint, Sint, Sint, Sint, Sint, Sint);
  const auto fun = (Fun *)sym->intrin;
  const auto err = fun(x0, x1, x2, x3, x4, x5, x6, x7);

  if (sym->err == ERR_MODE_THROW) return err;
  return nullptr;
}

static S8 arch_sym_err_reg(const Sym *sym) {
  if (sym->err != ERR_MODE_THROW) return -1;

  switch (sym->type) {
    case SYM_NORM:   return (S8)sym->out_len;
    case SYM_INTRIN: return ARCH_PARAM_REG_0;
    case SYM_EXTERN: unreachable();
    default:         unreachable();
  }
}

static void asm_append_sym_epilogue_ok(Comp *comp, Sym *sym) {
  if (sym->err != ERR_MODE_THROW) return;
  const auto reg = arch_sym_err_reg(sym);
  aver(reg >= 0);
  asm_append_zero_reg(comp, (U8)reg);
}

/*
Procedures which "throw" actually return an error in the next output GPR,
immediately after the last non-error result. This allows us to support the
no-throw mode, where the error is revealed as one of the outputs, and makes
"catch" completely free. The no-throw mode allows two-way ABI compatibility
with C, which is also entirely no-throw.
*/
static void asm_append_try(Comp *comp, const Sym *caller, const Sym *callee) {
  IF_DEBUG(aver(caller->err == ERR_MODE_THROW));
  IF_DEBUG(aver(callee->err == ERR_MODE_THROW));

  const auto reg = arch_sym_err_reg(callee);
  IF_DEBUG(aver(reg >= 0));

  const auto callee_err = (U8)reg;
  const auto caller_err = caller->out_len;

  if (caller_err == callee_err) {
    // cbnz <err>, <epi_err>
    asm_append_fixup_try(comp, caller_err);
    return;
  }

  /*
  Often, error register doesn't match, and we're allowed to immediately
  clobber the caller's error register because it's not among the callee
  output registers.

    mov  <calleR_err>, <calleE_err>
    cbnz <calleR_err>, <epi_err>
  */
  if (caller->out_len >= callee->out_len || callee->type == SYM_INTRIN) {
    asm_append_mov_reg(comp, caller_err, callee_err);
    asm_append_fixup_try(comp, caller_err);
    return;
  }

  /*
  If the caller's error register is among the callee's
  output registers, we must handle it more carefully.

    cbz <calleE_err>, <rest>
    mov <calleR_err>, <calleE_err>
    b   <epi_err>
    <rest>: proceeed
  */
  asm_append_compare_branch_zero(comp, callee_err, 3);
  asm_append_mov_reg(comp, caller_err, callee_err);
  asm_append_fixup_throw(comp);
}

/*
Used in no-throw words to "catch" errors from yes-throw words.

Defined to mirror the stack-CC implementation. Don't need to do
anything here, because all values are already in the expected
registers. The effect of "catch" in reg-CC is to append the
callee's error to the list of outputs, which is handled in
`comp_cc_reg.c`.
*/
static void asm_append_catch(Comp *comp, const Sym *caller, const Sym *callee) {
  (void)comp;
  (void)caller;
  (void)callee;
  IF_DEBUG(aver(callee->err == ERR_MODE_THROW));
}

static void asm_append_call_intrin_output(Comp *comp, const Sym *callee) {
  const auto len = callee->out_len;
  if (!len) return;

  /*
  We have many intrinsics which want to return `(val err)` in parameter
  GPRs, which is possible in C under Arm64, using an output struct with
  two fields. Unfortunately, we also have comp intrinsics which want to
  return `(val0 val1 err)` in GPRs, which is not supported in C or C++;
  compilers fall back on vector registers, even when using C++ tuples,
  which count as structs.

  So for now, intrinsic outputs are just pushed into the Forth data stack.
  This also makes it easier to reuse them between CC.

    ldr x8, [x28, INTERP_INTS_TOP]
    ... ldr xN, [x8, -8]! ...
    str x8, [x28, INTERP_INTS_TOP]

  This obviously violates the idea of native-only calls and avoiding the use
  of the data stack in compiled code. Our feeble excuse is that compiler
  intrinsics are meant only for compile-only words, which are run only when
  the interpreter actually exists. When / if we implement AOT compilation,
  we can prove that interp-only words are not called from `main`, directly
  or indirectly. So it should be both safe and not costly.
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

static Err asm_append_call_intrin_after(
  Comp *comp, Sym *caller, const Sym *callee, bool catch
) {
  try(asm_append_try_catch(comp, caller, callee, catch));
  asm_append_call_intrin_output(comp, callee);
  return nullptr;
}

static Err asm_append_call_intrin(
  Comp *comp, Sym *caller, const Sym *callee, bool catch
) {
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
  try(asm_append_call_intrin_after(comp, caller, callee, catch));
  return nullptr;
}

static void asm_append_call_extern(Comp *comp, Sym *caller, const Sym *callee) {
  aver(callee->type == SYM_EXTERN);

  // Free to use because extern calls clobber everything anyway.
  constexpr auto reg = ARCH_SCRATCH_REG_8;

  asm_append_dysym_load(comp, callee->name.buf, reg);
  asm_append_branch_link_to_reg(comp, reg);
  asm_register_call(comp, caller);
}

/*
TODO: descriptive error instead of crash.

TODO: for both callventions: support larger offsets.
Arm64 `ldr` and `str` are limited to 32760, which is
insufficient when abusing `alloca`.
*/
static void asm_validate_local_off(Ind off) { aver(off > 0 && off <= 32'760); }

// SYNC[asm_local_read].
static Instr asm_instr_local_read(Local *loc, U8 tar_reg) {
  IF_DEBUG(aver(loc->location != LOC_UNKNOWN));

  switch (loc->location) {
    case LOC_REG: {
      return asm_instr_mov_reg(tar_reg, loc->reg);
    }
    case LOC_MEM: {
      const auto off = loc->fp_off;
      asm_validate_local_off(off);
      return asm_instr_load_scaled_offset(tar_reg, ARCH_REG_FP, off);
    }
    case LOC_UNKNOWN: [[fallthrough]];
    default:          unreachable();
  }
}

// SYNC[asm_local_write].
static Instr asm_instr_local_write(Local *loc, U8 src_reg) {
  IF_DEBUG(aver(loc->location != LOC_UNKNOWN));

  switch (loc->location) {
    case LOC_REG: {
      return asm_instr_mov_reg(loc->reg, src_reg);
    }
    case LOC_MEM: {
      const auto off = loc->fp_off;
      asm_validate_local_off(off);
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
