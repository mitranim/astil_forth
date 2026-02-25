/*
This file defines assembly procedures for Arm64 for the variant
of our Forth system which uses the native procedure call ABI.

Compare `./arch_arm64_cc_stack.c` which uses a stack-based callvention.

Special registers:
- `x28` = pointer to interpreter / compiler.

The special registers must be kept in sync with `lang.f`.
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
    "unable to call " FMT_QUOTED
    ": %d input parameters required, but only " FMT_SINT
    " are available on the data stack\n",
    name,
    inp_len,
    ints_len
  );
}

static Err asm_call_norm(Interp *interp, const Sym *sym) {
  const auto fun     = comp_sym_exec_instr(&interp->comp, sym);
  const auto inp_len = sym->inp_len;
  const auto out_len = sym->out_len;

  aver(inp_len <= ASM_INP_PARAM_REG_LEN);
  aver(out_len <= ASM_OUT_PARAM_REG_LEN);

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

  if (sym->throws) {
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

// See `asm_append_call_intrin` for the explanation of the calling convention.
static Err asm_call_intrin(Interp *interp, const Sym *sym) {
  aver(sym->type == SYM_INTRIN);

  const auto ints = &interp->ints;

  Sint inps[ASM_INP_PARAM_REG_LEN] = {};
  Sint outs[ASM_OUT_PARAM_REG_LEN] = {};

  aver(sym->inp_len < ASM_INP_PARAM_REG_LEN);
  aver(sym->out_len < ASM_OUT_PARAM_REG_LEN);

  Ind inp = 0;
  while (inp < sym->inp_len) {
    // Pop in backward order.
    const auto ind = sym->inp_len - 1 - inp;
    try(int_stack_pop(ints, &inps[ind]));
    inp++;
  }

  aver(inp < ASM_INP_PARAM_REG_LEN);
  inps[inp++] = (Sint)interp;

  Ind out = 0;
  while (out < sym->out_len) {
    aver((inp + out) < ASM_INP_PARAM_REG_LEN);
    inps[inp + out] = (Sint)&outs[out];
    out++;
  }

  typedef Err(Fun)(Sint, Sint, Sint, Sint, Sint, Sint, Sint, Sint);

  const auto fun = (Fun *)sym->intrin;
  const auto err = fun(
    inps[0], inps[1], inps[2], inps[3], inps[4], inps[5], inps[6], inps[7]
  );

  if (sym->throws) try(err);

  out = 0;
  while (out < sym->out_len) {
    try(int_stack_push(ints, outs[out++]));
  }
  return nullptr;
}

static S8 asm_sym_err_reg(const Sym *sym) {
  if (!sym->throws) return -1;

  switch (sym->type) {
    case SYM_NORM:   return (S8)sym->out_len;
    case SYM_INTRIN: return ASM_PARAM_REG_0;
    case SYM_EXTERN: unreachable();
    default:         unreachable();
  }
}

static void asm_append_sym_epilogue_ok(Comp *comp, Sym *sym) {
  if (!sym->throws) return;
  const auto reg = asm_sym_err_reg(sym);
  aver(reg >= 0);
  asm_append_zero_reg(comp, (U8)reg);
}

/*
Procedures which "throw" actually return the exception in the
next output GPR, immediately after the last non-error result.
An alternative, which we tried earlier, is to use a dedicated
callee-saved register for all exceptions.

When using a dedicated exception register:
- "try" is always 1 instruction: `cbnz <err>, <epi>`.
- "catch" is always 2 instructions: `mov` and `eor`.
- ABI incompatibility: have to stash and restore the register
  at the boundaries between external and internal procedures,
  which requires KNOWING where the boundaries are.

When using a regular output register:
- "try" is between 1 and 3 instructions.
- "catch" is completely free.
- Perfect ABI compatibility: no clobbering of callee-saved registers.

Using a regular output register ultimately wins out by making it possible
to pass arbitrary procedures as callbacks to libc, without having to tell
the compiler to stash and restore the error register, which would be easy
to forget. That said, we also recommend using `catches` in such callbacks
to ensure exceptions are not ignored, which is also easy to forget...
*/
static void asm_append_try(Comp *comp, const Sym *caller, const Sym *callee) {
  IF_DEBUG(aver(caller->throws));
  IF_DEBUG(aver(callee->throws));

  const auto reg = asm_sym_err_reg(callee);
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
Must be used after compiling every call to a non-extern procedure.

When "catching", we don't need to do anything, because all values are already
in the expected registers. The effect of "catch" is to append the "exception"
to the list of outputs, which is handled in `comp_cc_reg.c`.
*/
static Err asm_append_try_catch(
  Comp *comp, Sym *caller, const Sym *callee, bool force_catch
) {
  if (force_catch) {
    if (!callee->throws) {
      return err_catch_no_throw(callee->name.buf);
    }
    return nullptr;
  }

  if (!callee->throws) return nullptr;
  if (caller->catches) return nullptr;

  caller->throws = true;
  asm_append_try(comp, caller, callee);
  return nullptr;
}

/*
We use a custom calling convention for interpreter / compiler intrinsics.
They're implemented in C and some of them want to return multiple outputs.
The Arm64 ABI supports multiple outputs in general-purpose registers, but
the C ABI is limited to 2 GPR outputs, using a two-field struct; for us,
this means 1 output and 1 error. We have intrinsics with 2 outputs and 1
error, which doesn't fit into the C ABI.

So instead, we stack-allocate outputs and pass pointers to them.
The interpreter pointer is placed between inputs and outputs.

Example call of `intrin_parse`:

  mov x0, <char>           -- input
  mov x1, x28              -- interp
  sub sp, sp, 16
  add x2, sp, 0            -- out_buf
  add x3, sp, 8            -- out_len
  adrp & add & blr <parse>
  add sp, sp, 16
  cbnz x0, <epi_err>
  ldur x0, [sp, -16]       -- out_buf
  ldur x1, [sp, -8]        -- out_len

This comes with some ABI problems. The calling code always uses word-sized
values and addressing. We need values to be loaded and stored in word-sized
chunks. For example, when an output of an intrinsic describes a register and
is `U8` in C, using `U8*` for the output is wrong; C would compile to `strb`
when storing the output, resulting in the 7 upper bytes of the receiving
address (under little endian) to be garbage. We need ALL stores of output
values to be word-sized. Intrinsic procedures must define their output
pointers appropriately.
*/
static Err asm_append_call_intrin(
  Comp *comp, Sym *caller, const Sym *callee, bool catch
) {
  aver(callee->type == SYM_INTRIN);

  const auto inps   = callee->inp_len;
  const auto outs   = callee->out_len;
  const auto size   = sizeof(Sint);
  const auto sp_off = asm_align_sp_off(size * outs);

  asm_append_mov_reg(comp, callee->inp_len, ASM_REG_INTERP);

  if (sp_off) {
    asm_append_sub_imm(comp, ASM_REG_SP, ASM_REG_SP, sp_off);

    U8 out = 0;
    while (out < outs) {
      U8 reg = inps + 1 + out;
      aver(reg < ASM_INP_PARAM_REG_LEN);
      asm_append_add_imm(comp, reg, ASM_REG_SP, out * size);
      out++;
    }
  }

  // Free to use because intrin calls clobber everything anyway.
  constexpr auto fun = ASM_SCRATCH_REG_8;

  asm_append_dysym_load(comp, callee->name.buf, fun);
  asm_append_branch_link_to_reg(comp, fun);
  if (sp_off) asm_append_add_imm(comp, ASM_REG_SP, ASM_REG_SP, sp_off);
  try(asm_append_try_catch(comp, caller, callee, catch));

  if (sp_off) {
    U8 reg = 0;
    while (reg < outs) {
      const auto out_off = (Sint)(reg * size) - (Sint)sp_off;
      aver(reg < ASM_OUT_PARAM_REG_LEN);
      aver(out_off < 0);
      asm_append_load_unscaled_offset(comp, reg, ASM_REG_SP, out_off);
      reg++;
    }
  }

  asm_register_call(comp, caller);
  return nullptr;
}

static void asm_append_call_extern(Comp *comp, Sym *caller, const Sym *callee) {
  aver(callee->type == SYM_EXTERN);

  // Free to use because extern calls clobber everything anyway.
  constexpr auto reg = ASM_SCRATCH_REG_8;

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
      return asm_instr_load_scaled_offset(tar_reg, ASM_REG_FP, off);
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
      return asm_instr_store_scaled_offset(src_reg, ASM_REG_FP, off);
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
static void asm_resolve_local_location(Comp *comp, Local *loc, Sym *sym) {
  if (loc->location != LOC_UNKNOWN) return;

  const auto reg = bits_pop_low(&comp->ctx.vol_regs);

  if (bits_has(ASM_VOLATILE_REGS, reg)) {
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

  asm_resolve_local_location(comp, loc, sym);
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
  asm_resolve_local_location(comp, loc, sym);
  *write->instr = asm_instr_local_write(loc, write->reg);
}

static void asm_fixup_locals(Comp *comp, Sym *sym) {
  const auto ctx = &comp->ctx;

  // Remaining volatile registers available for locals.
  aver(ctx->vol_regs == BITS_ALL);
  ctx->vol_regs = bits_del_all(ASM_VOLATILE_REGS, sym->clobber);

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
