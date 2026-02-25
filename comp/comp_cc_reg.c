/*
This file contains compilation logic for the register-based callvention
which is mutually exclusive with the stack-based callvention.

Ideally, this logic should be arch-independent, while arch-specific logic
should be placed in `arch_*.c` files. However, currently it hardcodes the
assumption that input and output registers match 1-to-1, which holds when
targeting Arm64 and Risc-V, and maybe some other arches, but not for x64.

TODO: to support x64, we'd probably need to:

- Treat "args" as speculative: they could be inputs or outputs.
- Treat every register in "args" as speculative.
- Emit a fixup for every "arg".
- When encountering a "call" or "recur", commit to "args" being "inps".
  Modify the latest "args" to reflect that. Commit each to the corresponding
  input register number, according to the ABI.
- When encountering a "ret", commit to "args" being "outs".
  Do the same as above, but using output registers according to the ABI.
- After a "call", remember that we have "outs" rather than "args".
  "Outs" have specific known registers.
- Assignments of locals after a call use the "out" regs as sources.
- A "ret" in the "out" state uses the prior output regs as-is, unless
  there's a mismatch between register types (we only use GPRs though).
- The next "arg" or "call" in the "out" state requires converting
  prior "outs" into "inps", moving between registers if needed.

Instead of this hacky-sounding FSM logic, the usual "proper" solution
is to build an AST during the first pass, then run semantic analysis,
often involving extra intermediary representations, before eventually
assembling. It would boil down to similar decisions anyway.

Our system is built with the express goal of avoiding exactly that,
and exploring the design space of AST-free single-pass forward-only
assembly, which is only possible in a language like Forth. However,
we still had to resort to an IR of sorts: the "fixups".
*/
#pragma once
#include "./arch.c"
#include "./arch_arm64.h"
#include "./arch_arm64_cc_reg.c"
#include "./comp.c"
#include "./interp.h"
#include "./lib/bits.c"
#include "./lib/dict.c"
#include "./lib/err.c"
#include "./lib/num.h"
#include "./lib/stack.c"
#include "./lib/str.h"
#include "./read.c"
#include "./sym.h"

// clang-format off

// Sanity check used in debugging.
static bool comp_ctx_valid(const Comp_ctx *ctx) {
  return (
    ctx &&
    is_aligned(ctx) &&
    is_aligned(&ctx->sym) &&
    is_aligned(&ctx->asm_fix) &&
    is_aligned(&ctx->loc_fix) &&
    is_aligned(&ctx->locals) &&
    is_aligned(&ctx->local_dict) &&
    is_aligned(&ctx->anon_locs) &&
    is_aligned(&ctx->fp_off) &&
    is_aligned(&ctx->reg_vals) &&
    is_aligned(&ctx->vol_regs) &&
    is_aligned(ctx->sym) &&
    (!ctx->sym || sym_valid(ctx->sym)) &&
    stack_valid((const Stack *)&ctx->asm_fix) &&
    stack_valid((const Stack *)&ctx->loc_fix) &&
    stack_valid((const Stack *)&ctx->locals) &&
    dict_valid((const Dict *)&ctx->local_dict)
  );
}

// clang-format on

static Err comp_ctx_deinit(Comp_ctx *ctx) {
  try(stack_deinit(&ctx->loc_fix));
  try(stack_deinit(&ctx->asm_fix));
  try(stack_deinit(&ctx->locals));
  dict_deinit(&ctx->local_dict);
  ptr_clear(ctx);
  return nullptr;
}

static Err comp_ctx_init(Comp_ctx *ctx) {
  ptr_clear(ctx);
  Stack_opt opt = {.len = 1024};
  try(stack_init(&ctx->locals, &opt));
  try(stack_init(&ctx->asm_fix, &opt));
  try(stack_init(&ctx->loc_fix, &opt));
  return nullptr;
}

static void comp_ctx_trunc(Comp_ctx *ctx) {
  stack_trunc(&ctx->loc_fix);
  stack_trunc(&ctx->asm_fix);
  stack_trunc(&ctx->locals);
  dict_trunc((Dict *)&ctx->local_dict);
  arr_clear(ctx->reg_vals);
  ctx->vol_regs = BITS_ALL; // Invalid initial state for bug detection.

  ptr_clear(&ctx->sym);
  ptr_clear(&ctx->anon_locs);
  ptr_clear(&ctx->fp_off);
  ptr_clear(&ctx->arg_low);
  ptr_clear(&ctx->arg_len);
  ptr_clear(&ctx->compiling);
  ptr_clear(&ctx->redefining);
}

// SYNC[comp_ctx_rewind].
static void comp_ctx_rewind(Comp_ctx *tar, Comp_ctx *snap) {
  tar->sym        = snap->sym;
  tar->anon_locs  = snap->anon_locs;
  tar->fp_off     = snap->fp_off;
  tar->vol_regs   = snap->vol_regs;
  tar->arg_low    = snap->arg_low;
  tar->arg_len    = snap->arg_len;
  tar->redefining = snap->redefining;
  tar->compiling  = snap->compiling;
  tar->has_alloca = snap->has_alloca;

  stack_rewind(&snap->locals, &tar->locals);
  dict_rewind(&snap->local_dict, &tar->local_dict);
  stack_rewind(&snap->asm_fix, &tar->asm_fix);
  stack_rewind(&snap->loc_fix, &tar->loc_fix);

  for (Ind ind = 0; ind < arr_cap(tar->reg_vals); ind++) {
    tar->reg_vals[ind] = snap->reg_vals[ind];
  }
}

// Returns a token representing a local which can be given to Forth code.
// In the stack-based calling convention, this returns a different value.
static Local *local_token(Local *loc) { return loc; }

static void validate_volatile_reg(U8 reg) {
  aver(bits_has(ASM_VOLATILE_REGS, reg));
}

static void validate_param_reg(U8 reg) {
  aver(bits_has(ASM_ALL_PARAM_REGS, reg));
}

static bool is_param_reg(U8 reg) { return bits_has(ASM_ALL_PARAM_REGS, reg); }

static const char *comp_local_name(Local *loc) {
  if (!loc) return "(nil)";
  if (loc->name.len) return loc->name.buf;
  return "(anonymous)";
}

static Local *comp_local_get_for_reg(Comp *comp, U8 reg) {
  validate_param_reg(reg);
  const auto val = comp->ctx.reg_vals[reg];
  return val.type == REG_VAL_LOC ? val.loc : nullptr;
}

static bool comp_local_has_reg(Comp *comp, Local *loc, U8 reg) {
  validate_param_reg(reg);
  return comp_local_get_for_reg(comp, reg) == loc;
}

static S8 comp_local_reg_any(Comp *comp, Local *loc) {
  const auto ctx = &comp->ctx;
  /*
  Could be "optimized" by creating two-way associations with extra
  book-keeping. There's no point and it would make things fragile.
  */
  for (U8 reg = 0; reg < arr_cap(ctx->reg_vals); reg++) {
    if (comp_local_get_for_reg(comp, reg) == loc) return (S8)reg;
  }
  return -1;
}

static bool comp_local_has_regs(Comp *comp, Local *loc) {
  return comp_local_reg_any(comp, loc) >= 0;
}

static void comp_local_reg_add(Comp *comp, Local *loc, U8 reg) {
  const auto vals = comp->ctx.reg_vals;

  validate_param_reg(reg);
  aver(!vals[reg].type);

  vals[reg] = (Reg_val){.type = REG_VAL_LOC, .loc = loc};
}

static void comp_local_regs_clear(Comp *comp, Local *loc) {
  const auto ctx = &comp->ctx;

  for (U8 reg = 0; reg < arr_cap(ctx->reg_vals); reg++) {
    if (comp_local_get_for_reg(comp, reg) != loc) continue;
    ctx->reg_vals[reg] = (Reg_val){};
  }
}

// For debug logging.
static Bits comp_local_reg_bits(Comp *comp, const Local *loc) {
  const auto ctx = &comp->ctx;
  Bits       out = 0;

  for (U8 reg = 0; reg < arr_cap(ctx->reg_vals); reg++) {
    if (comp_local_get_for_reg(comp, reg) != loc) continue;
    bits_add_to(&out, reg);
  }
  return out;
}

static const char *comp_local_fmt_reg_bits(Comp *comp, const Local *loc) {
  return uint32_to_bit_str((U32)comp_local_reg_bits(comp, loc));
}

static Err err_args_partial(
  const char *caller, const char *action, Sint low, Sint len
) {
  return errf(
    "in " FMT_QUOTED
    ": %s: prior values were partially consumed by assignments: " FMT_SINT
    " of " FMT_SINT
    "; hint: either use more assignments, or use `--` inside braces to discard unused values",
    caller,
    action,
    low,
    len
  );
}

static Err err_args_arity(
  const char *caller, const char *action, Sint req, Sint ava
) {
  return errf(
    "in " FMT_QUOTED ": %s: arity mismatch: required: " FMT_SINT
    ", provided: " FMT_SINT,
    caller,
    action,
    req,
    ava
  );
}

static Err comp_validate_args(Comp *comp, const char *action, Sint req) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));

  const auto ctx     = &comp->ctx;
  const auto arg_low = ctx->arg_low;
  const auto arg_len = ctx->arg_len;
  const auto name    = sym->name.buf;

  if (arg_low) return err_args_partial(name, action, arg_low, arg_len);
  if (req != arg_len) return err_args_arity(name, action, req, arg_len);
  return nullptr;
}

static Err comp_validate_call_args(Comp *comp, const Sym *callee) {
  return comp_validate_args(comp, "unable to compile call", callee->inp_len);
}

static Err comp_validate_ret_args(Comp *comp) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  try(comp_validate_args(comp, "unable to compile return", sym->out_len));

  if (!sym->throws) return nullptr;
  if (sym->out_len < ASM_OUT_PARAM_REG_LEN) return nullptr;

  return errf(
    "unable to compile return: too many output parameters: %d registers are available, %d are used, but 1 more is required for implicitly returned exception values",
    ASM_OUT_PARAM_REG_LEN,
    sym->out_len
  );
}

static Err comp_validate_recur_args(Comp *comp) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  try(comp_validate_args(comp, "unable to compile recursive call", sym->inp_len));
  return nullptr;
}

static Err err_assign_no_args(const char *name) {
  return errf(
    "unable to assign to " FMT_QUOTED ": there are no available arguments", name
  );
}

static void comp_append_local_write(Comp *comp, Local *loc, U8 reg) {
  const auto fix = stack_push(
    &comp->ctx.loc_fix,
    (Loc_fixup){
      .type  = LOC_FIX_WRITE,
      .write = (Local_write){
        .instr     = asm_append_breakpoint(comp, ASM_CODE_LOC_WRITE),
        .prev      = loc->write,
        .loc       = loc,
        .reg       = reg,
        .confirmed = loc->read,
      }
    }
  );
  loc->write = &fix->write;
}

/*
Must be invoked whenever a register needs to be used for ANYTHING
other than the current procedure's input parameters. Examples:

- An input argument for the next call.
- Side-effect clobbers in some call.
- Output results from some call.
*/
static Err comp_clobber_reg(Comp *comp, U8 reg) {
  validate_volatile_reg(reg);

  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  bits_add_to(&sym->clobber, reg);

  if (!is_param_reg(reg)) return nullptr;

  const auto loc          = comp_local_get_for_reg(comp, reg);
  comp->ctx.reg_vals[reg] = (Reg_val){};

  if (!loc || loc->stable || comp_local_has_regs(comp, loc)) return nullptr;
  comp_append_local_write(comp, loc, reg);
  loc->stable = true;
  return nullptr;
}

static Err comp_clobber_regs(Comp *comp, Bits clob) {
  while (clob) try(comp_clobber_reg(comp, bits_pop_low(&clob)));
  return nullptr;
}

static Err comp_clobber_from_call(Comp *comp, const Sym *callee) {
  /*
  For now, we require the clobber list to include inputs, outputs,
  and auxiliary clobbers. We end up checking input clobbers twice:
  here and when building the arguments. Might consider deduping at
  some later point.
  */
  try(comp_clobber_regs(comp, callee->clobber));

  // Exceptions implicitly use an additional output register.
  if (callee->throws) {
    try(comp_clobber_reg(comp, callee->out_len));
  }
  return nullptr;
}

static Err comp_local_reg_reset(Comp *comp, Local *loc, U8 reg) {
  validate_param_reg(reg);
  comp_local_regs_clear(comp, loc);

  const auto ctx  = &comp->ctx;
  const auto prev = comp_local_get_for_reg(comp, reg);
  if (prev && prev != loc) try(comp_clobber_reg(comp, reg));

  ctx->reg_vals[reg] = (Reg_val){.type = REG_VAL_LOC, .loc = loc};
  loc->stable        = false;

  IF_DEBUG({
    if (prev) {
      eprintf(
        "[system] reset local " FMT_QUOTED
        " to register %d, clobbering local " FMT_QUOTED "\n",
        loc->name.buf,
        reg,
        prev->name.buf
      );
    }
    else {
      eprintf(
        "[system] reset local " FMT_QUOTED " to register %d\n", loc->name.buf, reg
      );
    }
  });
  return nullptr;
}

// Lower-level, unsafe analogue of `comp_append_local_set_next`.
static Err comp_append_local_set(Comp *comp, Local *loc, U8 reg) {
  try(asm_validate_output_param_reg(reg));
  try(comp_local_reg_reset(comp, loc, reg));

  IF_DEBUG(eprintf(
    "[system] appended \"set\" of local " FMT_QUOTED " from register %d",
    loc->name.buf,
    reg
  ));

  return nullptr;
}

/*
Abstract "set" operation invoked via `{}` braces. Used both for parameter
declarations and regular assignments. Does not immediately create a "write";
those are added lazily on clobbers. Does not check, confirm, or invalidate
the latest pending "write" if any; writes are confirmed by "reads", and link
with each other for a chain confirmation.
*/
static Err comp_append_local_set_next(Comp *comp, Local *loc) {
  const auto ctx = &comp->ctx;
  const auto rem = (Sint)ctx->arg_len - (Sint)ctx->arg_low;

  if (!(rem > 0)) return err_assign_no_args(loc->name.buf);

  const auto reg = ctx->arg_low++;
  try(comp_local_reg_reset(comp, loc, reg));

  if (ctx->arg_len == ctx->arg_low) {
    ctx->arg_len = 0;
    ctx->arg_low = 0;
  }
  return nullptr;
}

static Err comp_next_inp_param_reg(Comp *comp, U8 *out) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));

  const auto reg = sym->inp_len++;
  try(asm_validate_input_param_reg(reg));
  bits_add_to(&sym->clobber, reg);

  if (out) *out = reg;
  return nullptr;
}

static Err comp_next_out_param_reg(Comp *comp, U8 *out) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));

  const auto reg = sym->out_len++;
  try(asm_validate_output_param_reg(reg));
  bits_add_to(&sym->clobber, reg);

  if (out) *out = reg;
  return nullptr;
}

static Err comp_next_arg_reg(Comp *comp, U8 *out) {
  const auto reg = comp->ctx.arg_len++;
  try(asm_validate_input_param_reg(reg));
  try(comp_clobber_reg(comp, reg));
  if (out) *out = reg;
  return nullptr;
}

// TODO: support asking for multiple scratch regs (when we need that).
static Err comp_scratch_reg(Comp *comp, U8 *out) {
  const auto reg = comp->ctx.arg_len;
  try(asm_validate_input_param_reg(reg));
  try(comp_clobber_reg(comp, reg));
  if (out) *out = reg;
  return nullptr;
}

// Concrete "read" operation used as a fallback by "get".
static void comp_append_local_read(Comp *comp, Local *loc, U8 reg) {
  (void)comp;
  auto write = loc->write;
  loc->write = nullptr;
  loc->read  = true; // Confirm subsequent writes.

  while (write) {
    write->confirmed = true;
    write            = write->prev;
  }

  stack_push(
    &comp->ctx.loc_fix,
    (Loc_fixup){
      .type = LOC_FIX_READ,
      .read = (Local_read){
        .instr = asm_append_breakpoint(comp, ASM_CODE_LOC_READ),
        .loc   = loc,
        .reg   = reg,
      }
    }
  );
}

static Err err_local_get_not_inited(const char *name) {
  return errf(
    "unable to get the value of local " FMT_QUOTED ": value not initialized",
    name
  );
}

// Lower-level, unsafe analogue of `comp_append_local_get_next`.
static Err comp_append_local_get(Comp *comp, Local *loc, U8 reg) {
  try(asm_validate_input_param_reg(reg));
  if (comp_local_has_reg(comp, loc, reg)) return nullptr;

  if (!loc->stable) return err_local_get_not_inited(loc->name.buf);

  comp_append_local_read(comp, loc, reg);
  comp_local_reg_add(comp, loc, reg);
  return nullptr;
}

/*
Abstract "get" operation invoked by simply mentioning
a local inside a word. May fall back on a "read".

Note that "get" happens as part of building an argument list,
which clobbers parameter registers, which may be associated
with other locals. The clobbers are handled in that logic.
*/
static Err comp_append_local_get_next(Comp *comp, Local *loc) {
  auto reg = comp->ctx.arg_len;
  try(asm_validate_input_param_reg(reg));

  if (comp_local_has_reg(comp, loc, reg)) {
    comp->ctx.arg_len++;
    return nullptr;
  }

  try(comp_next_arg_reg(comp, &reg));

  const auto any = comp_local_reg_any(comp, loc);
  if (any >= 0) {
    asm_append_mov_reg(comp, reg, (U8)any);
    return nullptr;
  }

  if (!loc->stable) return err_local_get_not_inited(loc->name.buf);

  comp_append_local_read(comp, loc, reg);
  comp_local_reg_add(comp, loc, reg);
  return nullptr;
}

static Err err_dup_param(const char *name) {
  return errf("duplicate parameter " FMT_QUOTED, name);
}

static Err comp_add_input_param(Comp *comp, Word_str name) {
  if (comp_local_get(comp, name.buf)) return err_dup_param(name.buf);

  Local *loc;
  try(comp_local_add(comp, name, &loc));

  U8 reg;
  try(comp_next_inp_param_reg(comp, &reg));

  comp_local_reg_add(comp, loc, reg);
  return nullptr;
}

/*
We don't immediately declare output parameters by name, because they're not
initialized. To be used, they have to be assigned, which also automatically
declares them. However, we keep their count, for the call signature.
*/
static Err comp_add_output_param(Comp *comp, Word_str name, U8 *reg) {
  (void)name;
  try(comp_next_out_param_reg(comp, reg));
  return nullptr;
}

static Err comp_append_imm_to_reg(Comp *comp, U8 reg, Sint imm, bool *has_load) {
  try(asm_validate_input_param_reg(reg));
  try(comp_clobber_reg(comp, reg));

  const auto instrs = &comp->code.code_write;
  const auto floor  = instrs->len;

  asm_append_imm_to_reg(comp, reg, imm, has_load);

  const auto ceil = instrs->len;
  const auto ctx  = &comp->ctx;

  aver(!ctx->reg_vals[reg].type);
  ctx->reg_vals[reg] = (Reg_val){
    .type        = REG_VAL_IMM,
    .imm         = imm,
    .instr_floor = floor,
    .instr_ceil  = ceil,
  };

  IF_DEBUG(eprintf(
    "[system] compiled constant " FMT_SINT " as argument in register %d\n", imm, reg
  ));
  return nullptr;
}

static Err comp_append_push_imm(Comp *comp, Sint imm) {
  const auto reg = comp->ctx.arg_len++;
  return comp_append_imm_to_reg(comp, reg, imm, nullptr);
}

static Err comp_call_intrin(Interp *interp, const Sym *sym) {
  try(asm_call_intrin(interp, sym));
  return nullptr;
}

static Err comp_before_append_call(Comp *comp, const Sym *callee) {
  try(comp_validate_call_args(comp, callee));
  try(comp_clobber_from_call(comp, callee));
  return nullptr;
}

/*
The code below assumes that the SAME registers are used for inps and outs,
which allows to treat outputs as inputs. This is very nice, but works ONLY
on some architectures, including Arm64 which is what we currently support,
and ONLY when not mixing GPRs and FPRs.

TODO: for x64, we'll need a more general approach.
See the comment at the top of this file.
*/
static Err comp_after_append_call(
  Comp *comp, const Sym *caller, const Sym *callee, bool catch
) {
  (void)caller;

  const auto ctx = &comp->ctx;

  /*
  An exception, if any, is always returned in the last output register.
  `asm_append_call_norm` decides how to handle it depending on whether
  the caller prefers automatic "catch"; we can also force a "catch".
  Here we only need to adjust the argument count when "catching".
  */
  if (catch) {
    aver(callee->throws);
    ctx->arg_len = callee->out_len + 1;
  }
  else {
    ctx->arg_len = callee->out_len;
  }

  ctx->arg_low = 0;
  return nullptr;
}

static void comp_clear_args(Comp *comp) {
  const auto ctx = &comp->ctx;
  ctx->arg_len   = 0;
  ctx->arg_low   = 0;
}

static Err comp_append_call_norm(
  Comp *comp, const Sym *callee, bool catch, bool *inlined
) {
  IF_DEBUG(aver(callee->type == SYM_NORM));

  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  try(comp_before_append_call(comp, callee));

  if (callee->norm.inlinable) {
    try(asm_inline_sym(comp, caller, callee, catch));
    if (inlined) *inlined = true;
  }
  else {
    try(asm_append_call_norm(comp, caller, callee, catch));
    if (inlined) *inlined = false;
  }

  try(comp_after_append_call(comp, caller, callee, catch));
  return nullptr;
}

static Err comp_append_call_intrin(Comp *comp, const Sym *callee, bool catch) {
  IF_DEBUG(aver(callee->type == SYM_INTRIN));
  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  try(comp_before_append_call(comp, callee));
  try(asm_append_call_intrin(comp, caller, callee, catch));
  try(comp_after_append_call(comp, caller, callee, catch));
  return nullptr;
}

static Err comp_append_call_extern(Comp *comp, const Sym *callee) {
  IF_DEBUG(aver(callee->type == SYM_EXTERN));

  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  try(comp_before_append_call(comp, callee));
  asm_append_call_extern(comp, caller, callee);

  constexpr bool catch = false;
  try(comp_after_append_call(comp, caller, callee, catch));
  return nullptr;
}

static Err comp_barrier(Comp *comp) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));

  const auto ctx     = &comp->ctx;
  const auto arg_low = ctx->arg_low;
  const auto arg_len = ctx->arg_len;
  const auto name    = sym->name.buf;

  if (arg_low) {
    return err_args_partial(name, "in control flow", arg_low, arg_len);
  }
  if (arg_len) {
    return err_args_arity(name, "in control flow", 0, arg_len);
  }
  for (U8 reg = 0; reg < arr_cap(ctx->reg_vals); reg++) {
    try(comp_clobber_reg(comp, reg));
  }
  return nullptr;
}

static Err err_alloca_size_zero() {
  return err_str("unable to `alloca`: provided size is zero");
}

static Err err_alloca_size_negative(Sint size) {
  return errf("unable to `alloca`: provided size " FMT_SINT "is negative", size);
}

static Err err_alloca_size_exceeds(Sint size, Uint limit) {
  return errf(
    "unable to `alloca`: provided size " FMT_SINT " exceeds limit " FMT_UINT,
    size,
    limit
  );
}

static Err comp_alloca_const(Comp *comp, Reg_val val) {
  const auto size = val.imm;
  if (!size) return err_alloca_size_zero();
  if (size < 0) return err_alloca_size_negative(size);
  if (size > (Sint)IND_MAX) return err_alloca_size_exceeds(size, IND_MAX);

  // If possible, delete prior "imm to reg" instructions.
  const auto instrs = &comp->code.code_write;
  if (instrs->len == val.instr_ceil) instrs->len = val.instr_floor;

  const auto off = asm_align_sp_off((Ind)size);

  // sub x0, sp, <off>
  // mov sp, x0
  asm_append_sub_imm(comp, ASM_PARAM_REG_0, ASM_REG_SP, off);
  asm_append_add_imm(comp, ASM_REG_SP, ASM_PARAM_REG_0, 0);
  return nullptr;
}

// sub x0, sp, x0
// and x0, x0, 0xfffffffffffffff0
// mov sp, x0
static Err comp_alloca_dynamic(Comp *comp) {
  asm_append_sub_reg_ext(comp, ASM_PARAM_REG_0, ASM_REG_SP, ASM_PARAM_REG_0);
  asm_append_sp_align(comp, ASM_PARAM_REG_0);
  asm_append_add_imm(comp, ASM_REG_SP, ASM_PARAM_REG_0, 0);
  return nullptr;
}

static Err comp_alloca(Comp *comp) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  try(comp_validate_args(comp, "unable to `alloca`", 1));

  const auto ctx = &comp->ctx;
  const auto val = ctx->reg_vals[0];

  if (val.type == REG_VAL_IMM) {
    try(comp_alloca_const(comp, val));
  }
  else {
    try(comp_alloca_dynamic(comp));
  }

  sym->norm.has_alloca = true;
  return nullptr;
}

static Err comp_append_try_or_throw(
  Comp *comp, const Sym *sym, const char *msg, void(asm_append)(Comp *)
) {
  aver(sym->throws);
  try(comp_validate_args(comp, msg, 1));
  comp_clear_args(comp);

  const auto src_reg = ASM_PARAM_REG_0;
  const auto tar_reg = asm_sym_err_reg(sym);
  aver(tar_reg >= 0);

  if (tar_reg != src_reg) {
    try(comp_clobber_reg(comp, (U8)tar_reg));
    asm_append_mov_reg(comp, (U8)tar_reg, (U8)src_reg);
  }

  asm_append(comp);
  return nullptr;
}

static void comp_asm_append_try(Comp *comp) {
  asm_append_fixup_try(comp, ASM_PARAM_REG_0);
}

static Err comp_append_try(Comp *comp, const Sym *sym) {
  return comp_append_try_or_throw(
    comp, sym, "unable to `try`", comp_asm_append_try
  );
}

static Err comp_append_throw(Comp *comp, const Sym *sym) {
  return comp_append_try_or_throw(
    comp, sym, "unable to `throw`", asm_append_fixup_throw
  );
}

static const char *loc_fixup_fmt(Loc_fixup *fix) {
  static thread_local char BUF[4096];

  switch (fix->type) {
    case LOC_FIX_READ: {
      const auto val = &fix->read;
      return sprintbuf(
        BUF,
        "{type = read, loc = %s (%p), instr = %p, reg = %d}",
        val->loc->name.buf,
        val->loc,
        val->instr,
        val->reg
      );
    }
    case LOC_FIX_WRITE: {
      const auto val = &fix->write;
      return sprintbuf(
        BUF,
        "{type = write, loc = %s (%p), instr = %p, reg = %d, prev = %p, confirmed = %s}",
        val->loc->name.buf,
        val->loc,
        val->instr,
        val->reg,
        val->prev,
        bool_str(val->confirmed)
      );
    }
    default: unreachable();
  }
}
