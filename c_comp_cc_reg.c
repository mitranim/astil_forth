/*
This file contains compilation logic for the register-based callvention
which is mutually exclusive with the stack-based callvention.

Ideally, this logic should be arch-independent, while arch-specific logic
should be placed in `c_arch_*` files. However, currently it hardcodes the
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
#include "./c_arch.c"
#include "./c_comp.c"
#include "./c_interp.h"
#include "./c_read.c"
#include "./lib/bits.c"
#include "./lib/dict.c"
#include "./lib/err.c"
#include "./lib/num.h"
#include "./lib/stack.c"
#include "./lib/str.h"

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
    is_aligned(&ctx->loc_regs) &&
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
  arr_clear(ctx->loc_regs);
  ctx->vol_regs = BITS_ALL; // Invalid initial state for bug detection.

  ptr_clear(&ctx->sym);
  ptr_clear(&ctx->mem_locs);
  ptr_clear(&ctx->arg_low);
  ptr_clear(&ctx->arg_len);
  ptr_clear(&ctx->compiling);
  ptr_clear(&ctx->redefining);
}

// Returns a token representing a local which can be given to Forth code.
// In the stack-based calling convention, this returns a different value.
static Local *local_token(Local *loc) { return loc; }

static void validate_volatile_reg(U8 reg) {
  aver(bits_has(ARCH_VOLATILE_REGS, reg));
}

static void validate_param_reg(U8 reg) {
  aver(bits_has(ARCH_ALL_PARAM_REGS, reg));
}

static bool is_param_reg(U8 reg) { return bits_has(ARCH_ALL_PARAM_REGS, reg); }

static Local *comp_local_get_for_reg(Comp *comp, U8 reg) {
  validate_param_reg(reg);
  return comp->ctx.loc_regs[reg];
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
  for (U8 reg = 0; reg < arr_cap(ctx->loc_regs); reg++) {
    if (ctx->loc_regs[reg] == loc) return (S8)reg;
  }
  return -1;
}

static bool comp_local_has_regs(Comp *comp, Local *loc) {
  return comp_local_reg_any(comp, loc) >= 0;
}

static void comp_local_reg_add(Comp *comp, Local *loc, U8 reg) {
  validate_param_reg(reg);
  aver(!comp_local_get_for_reg(comp, reg));
  const auto ctx     = &comp->ctx;
  ctx->loc_regs[reg] = loc;
}

static void comp_local_reg_del(Comp *comp, Local *loc, U8 reg) {
  validate_param_reg(reg);

  const auto prev = comp_local_get_for_reg(comp, reg);
  aver(!prev || prev == loc);

  const auto ctx     = &comp->ctx;
  ctx->loc_regs[reg] = nullptr;
}

static void comp_local_reg_reset(Comp *comp, Local *loc, U8 reg) {
  validate_param_reg(reg);

  const auto ctx = &comp->ctx;
  for (U8 ind = 0; ind < arr_cap(ctx->loc_regs); ind++) {
    if (ctx->loc_regs[ind] != loc) continue;
    ctx->loc_regs[ind] = nullptr;
  }

  ctx->loc_regs[reg] = loc;
  loc->stable        = false;
}

static void comp_local_regs_clear(Comp *comp, Local *loc) {
  const auto ctx = &comp->ctx;
  for (U8 reg = 0; reg < arr_cap(ctx->loc_regs); reg++) {
    if (ctx->loc_regs[reg] == loc) ctx->loc_regs[reg] = nullptr;
  }
}

// For debug logging.
static Bits comp_local_reg_bits(const Comp *comp, const Local *loc) {
  const auto ctx = &comp->ctx;
  Bits       out = 0;
  for (U8 reg = 0; reg < arr_cap(ctx->loc_regs); reg++) {
    if (ctx->loc_regs[reg] == loc) bits_add_to(&out, reg);
  }
  return out;
}

static const char *comp_local_fmt_reg_bits(const Comp *comp, const Local *loc) {
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
    "in " FMT_QUOTED ": %s: arity mismatch: required vs provided: " FMT_SINT
    " vs " FMT_SINT "",
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
  try(comp_validate_args(comp, "unable to return", sym->out_len));
  return nullptr;
}

static Err comp_validate_recur_args(Comp *comp) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  try(comp_validate_args(comp, "unable to recur", sym->inp_len));
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

// Lower-level, unsafe analogue of `comp_append_local_set_next`.
static Err comp_append_local_set(Comp *comp, Local *loc, U8 reg) {
  try(arch_validate_output_param_reg(reg));
  comp_local_reg_reset(comp, loc, reg);
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
  comp_local_reg_reset(comp, loc, reg);

  if (ctx->arg_len == ctx->arg_low) {
    ctx->arg_len = 0;
    ctx->arg_low = 0;
  }
  return nullptr;
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

  const auto loc = comp_local_get_for_reg(comp, reg);
  comp_local_reg_del(comp, loc, reg);

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
  return comp_clobber_regs(comp, callee->clobber);
}

static Err comp_next_inp_param_reg(Comp *comp, U8 *out) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));

  const auto reg = sym->inp_len++;
  try(arch_validate_input_param_reg(reg));
  bits_add_to(&sym->clobber, reg);

  if (out) *out = reg;
  return nullptr;
}

static Err comp_next_out_param_reg(Comp *comp, U8 *out) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));

  const auto reg = sym->out_len++;
  try(arch_validate_output_param_reg(reg));
  bits_add_to(&sym->clobber, reg);

  if (out) *out = reg;
  return nullptr;
}

static Err comp_next_arg_reg(Comp *comp, U8 *out) {
  const auto reg = comp->ctx.arg_len++;
  try(arch_validate_input_param_reg(reg));
  try(comp_clobber_reg(comp, reg));
  if (out) *out = reg;
  return nullptr;
}

// TODO: support asking for multiple scratch regs (when we need that).
static Err comp_scratch_reg(Comp *comp, U8 *out) {
  const auto reg = comp->ctx.arg_len;
  try(arch_validate_input_param_reg(reg));
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
  try(arch_validate_input_param_reg(reg));
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
  try(arch_validate_input_param_reg(reg));

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

static Err comp_append_push_imm(Comp *comp, Sint imm) {
  U8 reg;
  try(comp_next_arg_reg(comp, &reg));
  asm_append_imm_to_reg(comp, reg, imm, nullptr);

  IF_DEBUG(eprintf(
    "[system] compiled literal number " FMT_SINT " as argument in register %d\n",
    imm,
    reg
  ));
  return nullptr;
}

static Err comp_call_intrin(Interp *interp, const Sym *sym) {
  try(arch_call_intrin(interp, sym));
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
static Err comp_after_append_call(Comp *comp, const Sym *callee) {
  const auto ctx = &comp->ctx;
  ctx->arg_len   = callee->out_len;
  ctx->arg_low   = 0;
  return nullptr;
}

static void comp_clear_args(Comp *comp) {
  const auto ctx = &comp->ctx;
  ctx->arg_len   = 0;
  ctx->arg_low   = 0;
}

static Err comp_append_call_norm(Comp *comp, const Sym *callee, bool *inlined) {
  IF_DEBUG(aver(callee->type == SYM_NORM));

  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  try(comp_before_append_call(comp, callee));

  if (callee->norm.inlinable) {
    try(asm_inline_sym(comp, callee));
    if (inlined) *inlined = true;
  }
  else {
    try(asm_append_call_norm(comp, caller, callee));
    if (inlined) *inlined = false;
  }

  try(comp_after_append_call(comp, callee));
  return nullptr;
}

static Err comp_append_call_intrin(Comp *comp, const Sym *callee) {
  IF_DEBUG(aver(callee->type == SYM_INTRIN));
  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  try(comp_before_append_call(comp, callee));
  asm_append_call_intrin(comp, caller, callee);
  try(comp_after_append_call(comp, callee));
  return nullptr;
}

static Err comp_append_call_extern(Comp *comp, const Sym *callee) {
  IF_DEBUG(aver(callee->type == SYM_EXTERN));
  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  try(comp_before_append_call(comp, callee));
  asm_append_call_extern(comp, caller, callee);
  try(comp_after_append_call(comp, callee));
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
    return err_args_partial(
      name, " arity mismatch in control flow", arg_low, arg_len
    );
  }
  if (arg_len) {
    return err_args_arity(name, " arity mismatch in control flow", 0, arg_len);
  }
  for (U8 reg = 0; reg < arr_cap(ctx->loc_regs); reg++) {
    try(comp_clobber_reg(comp, reg));
  }
  return nullptr;
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
        "{type = write, loc = %s (%p), instr = %p, reg = %d, prev = %p, confirmed = %d}",
        val->loc->name.buf,
        val->loc,
        val->instr,
        val->reg,
        val->prev,
        val->confirmed
      );
    }
    default: unreachable();
  }
}
