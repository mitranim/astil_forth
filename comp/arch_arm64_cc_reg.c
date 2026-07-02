/*
This file defines assembly functions for Arm64 for the variant
of our Forth system which uses the native procedure call ABI.

Compare `./arch_arm64_cc_stack.c` which uses a stack-based callvention.

Special registers:
- `x28` = pointer to interpreter / compiler.

The special registers must be kept in sync with `lang.af`.
*/
#pragma once
#include "../clib/bits.c"
#include "./arch_arm64.h"
#include "./interp.h"
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

  assert_fatal(inp_len <= ASM_INP_PARAM_REG_LEN);
  assert_fatal(out_len <= ASM_OUT_PARAM_REG_LEN);

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

  const auto reg_out_len = sym->has_err ? out_len - 1 : out_len;

  if (reg_out_len > 0) try(int_stack_push(ints, x0));
  if (reg_out_len > 1) try(int_stack_push(ints, x1));
  if (reg_out_len > 2) try(int_stack_push(ints, x2));
  if (reg_out_len > 3) try(int_stack_push(ints, x3));
  if (reg_out_len > 4) try(int_stack_push(ints, x4));
  if (reg_out_len > 5) try(int_stack_push(ints, x5));
  if (reg_out_len > 6) try(int_stack_push(ints, x6));
  if (reg_out_len > 7) try(int_stack_push(ints, x7));

  Err err = nullptr;

  if (sym->has_err) {
    switch (out_len - 1) {
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

#define TRUST_FUN_ABI __attribute__((no_sanitize("function")))

// See `asm_append_call_intrin` for the explanation of the calling convention.
TRUST_FUN_ABI static Err asm_call_intrin(Interp *interp, const Sym *sym) {
  assert_fatal(sym->type == SYM_INTRIN);

  const auto ints = &interp->ints;

  Sint inps[ASM_INP_PARAM_REG_LEN] = {};
  Sint outs[ASM_OUT_PARAM_REG_LEN] = {};

  assert_fatal(sym->inp_len < ASM_INP_PARAM_REG_LEN);
  assert_fatal(sym->out_len < ASM_OUT_PARAM_REG_LEN);

  Ind inp = 0;
  while (inp < sym->inp_len) {
    // Pop in backward order.
    const auto ind = sym->inp_len - 1 - inp;
    try(int_stack_pop(ints, &inps[ind]));
    inp++;
  }

  assert_fatal(inp < ASM_INP_PARAM_REG_LEN);
  inps[inp++] = (Sint)interp;

  const U8 reg_out_len = sym->has_err ? sym->out_len - 1 : sym->out_len;

  Ind out = 0;
  while (out < reg_out_len) {
    assert_fatal((inp + out) < ASM_INP_PARAM_REG_LEN);
    inps[inp + out] = (Sint)&outs[out];
    out++;
  }

  typedef Err(Fun)(Sint, Sint, Sint, Sint, Sint, Sint, Sint, Sint);

  const auto fun = (Fun *)sym->intrin;
  const auto err = fun(
    inps[0], inps[1], inps[2], inps[3], inps[4], inps[5], inps[6], inps[7]
  );

  if (sym->has_err) try(err);

  out = 0;
  while (out < reg_out_len) {
    try(int_stack_push(ints, outs[out++]));
  }
  return nullptr;
}

static S8 asm_sym_err_reg(const Sym *sym) {
  if (!sym->has_err) return -1;

  switch (sym->type) {
    case SYM_NORM:   return (S8)(sym->out_len - 1);
    case SYM_INTRIN: return (S8)(sym->out_len - 1);
    case SYM_EXTERN: unreachable();
    default:         unreachable();
  }
}

static void asm_append_sym_epilogue_ok(Comp *comp, Sym *sym) {
  (void)comp;
  (void)sym;
}

/*
In the reg-CC version of our system, an error is a regular output,
always explicitly declared and returned in the next GPR after the
non-error outputs.

This approach wins on simplicity and ABI interop. Signatures reflect the ABI.
We can pass callbacks to `libc` and they just work. A C program could declare
a function type such as, for example, `Err Func(intptr_t)`, obtain a pointer
to a Forth word with that signature, and call it as-is, without glue.

We also provide shortcuts `.try` / `.throw` for returning an error locally,
very similar to Swift. We also provide an opt-in `.try_all` setting, which
causes the compiler to implicitly insert `.try` for errors, reducing noise.
It still keeps errors in word signatures, which always reflect word ABI.

The cost of `.try` here is between 1 and 3 instructions on Arm64.
*/
static void asm_append_try(
  Comp *comp,
  U8    tar_reg, // Current word's (caller's) error register.
  U8    src_reg  // Error register we're testing.
) {
  if (tar_reg == src_reg) {
    // cbnz <reg>, <epi_reg>
    asm_append_fixup_try(comp, tar_reg);
    return;
  }

  /*
  Most of the time, `.try` happens against multiple outputs,
  where the last output is an error. When testing an error,
  we are not allowed to clobber any prior outputs. However,
  if the target error register is higher, we can renumerate
  without clobbering the prior values.

    mov  <tar_reg>, <src_reg>
    cbnz <tar_reg>, <epi_reg>
  */
  if (tar_reg > src_reg) {
    asm_append_mov_reg(comp, tar_reg, src_reg);
    asm_append_fixup_try(comp, tar_reg);
    return;
  }

  /*
  If the target error register is among the outputs preceding
  the source error register, we must handle it more carefully.

    cbz <src_reg>, <rest>
    mov <tar_reg>, <src_reg>
    b   <epi_reg>
    <rest>: proceeed
  */
  asm_append_compare_branch_zero(comp, src_reg, 3);
  asm_append_mov_reg(comp, tar_reg, src_reg);
  asm_append_fixup_throw(comp);
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

Example call of `intrin_read_until_char`:

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
values to be word-sized. Intrinsic functions must define their output
pointers appropriately.
*/
static Err asm_append_call_intrin(
  Comp *comp, Sym *caller, const Sym *callee, bool err_mode
) {
  (void)caller;
  assert_fatal(callee->type == SYM_INTRIN);

  const U8  inps   = callee->inp_len;
  const U8  outs   = callee->has_err ? callee->out_len - 1 : callee->out_len;
  const Ind size   = sizeof(Sint);
  const Ind sp_off = asm_align_sp_off(MUL(size, outs));

  asm_append_mov_reg(comp, callee->inp_len, ASM_REG_INTERP);

  if (sp_off) {
    asm_append_sub_imm(comp, ASM_REG_SP, ASM_REG_SP, sp_off);

    U8 out = 0;
    while (out < outs) {
      U8 reg = inps + 1 + out;
      assert_fatal(reg < ASM_INP_PARAM_REG_LEN);
      asm_append_add_imm(comp, reg, ASM_REG_SP, MUL(out, size));
      out++;
    }
  }

  // Free to use because intrin calls clobber everything anyway.
  constexpr auto reg = ASM_SCRATCH_REG_8;

  asm_append_dysym_load(comp, callee->name.buf, reg, &comp->code.intrins);
  asm_append_branch_link_to_reg(comp, reg);
  if (sp_off) asm_append_add_imm(comp, ASM_REG_SP, ASM_REG_SP, sp_off);

  if (callee->has_err && outs) {
    assert_fatal(outs < ASM_OUT_PARAM_REG_LEN);
    asm_append_mov_reg(comp, outs, ASM_PARAM_REG_0);
  }

  if (sp_off) {
    U8 reg = 0;
    while (reg < outs) {
      const auto out_off = (Sint)(reg * size) - (Sint)sp_off;
      assert_fatal(reg < ASM_OUT_PARAM_REG_LEN);
      assert_fatal(out_off < 0);
      asm_append_load_unscaled_offset(comp, reg, ASM_REG_SP, out_off);
      reg++;
    }
  }

  (void)err_mode;

  return nullptr;
}

static void asm_append_call_extern(Comp *comp, const Sym *callee) {
  assert_fatal(callee->type == SYM_EXTERN);

  // Free to use because extern calls clobber everything anyway.
  constexpr auto reg = ASM_SCRATCH_REG_8;

  asm_append_dysym_load(comp, callee->name.buf, reg, &comp->code.externs);
  asm_append_branch_link_to_reg(comp, reg);
}

/*
TODO: descriptive error instead of crash.

TODO: for both callventions: support larger offsets.
Arm64 `ldr` and `str` are limited to 32760, which is
insufficient when abusing `alloca`.
*/
static void asm_validate_local_off(Ind off) {
  assert_fatal(off > 0 && off <= 32'760);
}

// SYNC[asm_local_addressing].
static Instr asm_instr_local_read(Local *loc, U8 tar_reg) {
  IF_DEBUG(assert_fatal(loc->location != LOC_UNKNOWN));

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

// SYNC[asm_local_addressing].
static Instr asm_instr_local_write(Local *loc, U8 src_reg) {
  IF_DEBUG(assert_fatal(loc->location != LOC_UNKNOWN));

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
We accumulate clobbers of volatile registers from ALL sources across the entire
function, and always treat these registers as temporary, meaning that no locals
across the function would be assigned to these locations, even for locals whose
lifetimes do not overlap with clobbers. Simple way of avoiding an additional IR
and data flow analysis.
*/
static void asm_resolve_local_location(Comp *comp, Local *loc, Sym *sym) {
  if (loc->location != LOC_UNKNOWN) return;

  const auto ctx = &comp->ctx;

  if (ctx->vol_regs) {
    const auto reg = bits_pop_low(&ctx->vol_regs);

    if (bits_has(ASM_REGS_VOLATILE, reg)) {
      loc->location = LOC_REG;
      loc->reg      = reg;
      bits_add_to(&sym->clobber, reg);
      return;
    }
  }

  // Can we assign a callee-saved register?
  if (ctx->saved_reg < ASM_STABLE_REG_LAST) {
    U8 reg;
    if (ctx->saved_reg) reg = ++ctx->saved_reg;
    else reg = (ctx->saved_reg = ASM_STABLE_REG_FIRST);

    loc->location = LOC_REG;
    loc->reg      = reg;
    return;
  }

  comp_local_alloc_mem(comp, loc);
  loc->location = LOC_MEM;
}

static void asm_fixup_loc_read(Comp *comp, Loc_fixup *fix, Sym *sym) {
  IF_DEBUG(assert_fatal(fix->type == LOC_FIX_READ));

  const auto read = &fix->read;
  const auto loc  = read->loc;

  asm_resolve_local_location(comp, loc, sym);
  *read->instr = asm_instr_local_read(loc, read->reg);
}

static void asm_fixup_loc_reloc(Comp *comp, Loc_fixup *fix, Sym *sym) {
  IF_DEBUG(assert_fatal(fix->type == LOC_FIX_RELOC));

  const auto reloc = &fix->reloc;
  const auto instr = reloc->instr;

  // This VERY dirty hack allows us to preserve the simplicity of forward-only
  // single-pass assembly, without having to invent an IR and have the library
  // and program code deal with an extra API. Nops are cheap and significantly
  // better than memory ops anyway.
  if (!reloc->confirmed) {
    if (asm_skipped_prologue_instr(comp, sym, instr)) return;
    *instr = ASM_INSTR_NOP;
    return;
  }

  const auto loc = reloc->loc;
  asm_resolve_local_location(comp, loc, sym);
  *instr = asm_instr_local_write(loc, reloc->reg);
}

/*
Figures out which remaining volatile registers are available for locals.

Input parameters in general are not automatically considered clobbers.
We have to exclude them here, because otherwise, locals would often be
relocated to parameter registers before param values are actually used.
*/
static Bits asm_remaining_vol_regs(const Sym *sym) {
  auto regs = bits_del_all(ASM_REGS_VOLATILE, sym->clobber);

  for (U8 reg = 0; reg < sym->inp_len; reg++) {
    bits_del_from(&regs, reg);
  }

  return regs;
}

static void asm_fixup_locals(Comp *comp, Sym *sym) {
  const auto ctx = &comp->ctx;

  // Figure out which volatile registers are available for locals.
  assert_fatal(ctx->vol_regs == BITS_ALL);
  ctx->vol_regs = asm_remaining_vol_regs(sym);

  assert_fatal(!ctx->saved_reg);

  for (stack_range(auto, fix, &ctx->loc_fix)) {
    switch (fix->type) {
      case LOC_FIX_READ: {
        asm_fixup_loc_read(comp, fix, sym);
        continue;
      }
      case LOC_FIX_RELOC: {
        asm_fixup_loc_reloc(comp, fix, sym);
        continue;
      }
      default: unreachable();
    }
  }
}

static void asm_backtrack_instrs_opt(Comp *comp, Ind floor, Ind ceil) {
  const auto instrs = &comp->code.code_write;
  if (instrs->len == ceil) instrs->len = floor;
}
