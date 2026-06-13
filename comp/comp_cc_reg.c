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
#include "../clib/bits.c"
#include "../clib/dict.c"
#include "../clib/err.c"
#include "../clib/num.h"
#include "../clib/stack.c"
#include "../clib/str.h"
#include "./arch.c"
#include "./arch_arm64.h"
#include "./arch_arm64_cc_reg.c"
#include "./comp.c"
#include "./interp.h"
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

// SYNC[comp_ctx_trunc].
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
  ptr_clear(&ctx->saved_reg);
  ptr_clear(&ctx->arg_low);
  ptr_clear(&ctx->arg_len);
  ptr_clear(&ctx->compiling);
  ptr_clear(&ctx->redefining);
  ptr_clear(&ctx->try_all);
}

// SYNC[comp_ctx_rewind].
static Err comp_ctx_snapshot(const Comp_ctx *prev, Comp_ctx *next) {
  // Can't rewind dictionary state.
  const auto len = prev->local_dict.len;
  if (len) {
    return errf(
      "unable to snapshot compilation context: non-empty local dict (" FMT_IND
      ")",
      len
    );
  }

  *next = *prev;
  return nullptr;
}

// SYNC[comp_ctx_rewind].
static void comp_ctx_rewind(const Comp_ctx *prev, Comp_ctx *next) {
  next->sym        = prev->sym;
  next->anon_locs  = prev->anon_locs;
  next->fp_off     = prev->fp_off;
  next->vol_regs   = prev->vol_regs;
  next->saved_reg  = prev->saved_reg;
  next->arg_low    = prev->arg_low;
  next->arg_len    = prev->arg_len;
  next->redefining = prev->redefining;
  next->compiling  = prev->compiling;
  next->has_alloca = prev->has_alloca;
  next->try_all    = prev->try_all;

  stack_rewind(&prev->locals, &next->locals);
  dict_trunc((Dict *)&next->local_dict);
  stack_rewind(&prev->asm_fix, &next->asm_fix);
  stack_rewind(&prev->loc_fix, &next->loc_fix);

  for (Ind ind = 0; ind < arr_cap(next->reg_vals); ind++) {
    next->reg_vals[ind] = prev->reg_vals[ind];
  }
}

// Returns a token representing a local which can be given to Forth code.
// In the stack-based calling convention, this returns a different value.
static Local *local_token(Local *loc) { return loc; }

static void validate_volatile_reg(U8 reg) {
  aver(bits_has(ASM_REGS_VOLATILE, reg));
}

static void validate_tracked_arg_reg(U8 reg) {
  aver(bits_has(ASM_ARG_REGS, reg));
}

static bool is_tracked_arg_reg(U8 reg) { return bits_has(ASM_ARG_REGS, reg); }

static const char *comp_local_name(Local *loc) {
  if (!loc) return "(nil)";
  if (loc->name.len) return loc->name.buf;
  return "(anonymous)";
}

static Local *comp_local_get_for_reg(Comp *comp, U8 reg) {
  validate_tracked_arg_reg(reg);
  const auto val = comp->ctx.reg_vals[reg];
  return val.type == REG_VAL_LOC ? val.loc : nullptr;
}

static bool comp_local_has_reg(Comp *comp, Local *loc, U8 reg) {
  validate_tracked_arg_reg(reg);
  return comp_local_get_for_reg(comp, reg) == loc;
}

static S8 comp_local_reg_any(Comp *comp, Local *loc) {
  constexpr U8 ceil = arr_cap(comp->ctx.reg_vals);

  /*
  Could be "optimized" by creating two-way associations with extra
  book-keeping. There's no point and it would make things fragile.
  */
  for (U8 reg = 0; reg < ceil; reg++) {
    if (comp_local_get_for_reg(comp, reg) == loc) return (S8)reg;
  }
  return -1;
}

static bool comp_local_has_regs(Comp *comp, Local *loc) {
  return comp_local_reg_any(comp, loc) >= 0;
}

static void comp_local_reg_add(Comp *comp, Local *loc, U8 reg) {
  const auto vals = comp->ctx.reg_vals;

  validate_tracked_arg_reg(reg);
  aver(!vals[reg].type);

  vals[reg] = (Reg_val){.type = REG_VAL_LOC, .loc = loc};
}

static void comp_local_regs_clear(Comp *comp, Local *loc) {
  const auto   ctx  = &comp->ctx;
  constexpr U8 ceil = arr_cap(comp->ctx.reg_vals);

  for (U8 reg = 0; reg < ceil; reg++) {
    if (comp_local_get_for_reg(comp, reg) != loc) continue;
    ptr_clear(&ctx->reg_vals[reg]);
  }
}

// For debug logging.
static Bits comp_local_reg_bits(Comp *comp, const Local *loc) {
  constexpr U8 ceil = arr_cap(comp->ctx.reg_vals);
  Bits         out  = 0;

  for (U8 reg = 0; reg < ceil; reg++) {
    if (comp_local_get_for_reg(comp, reg) != loc) continue;
    bits_add_to(&out, reg);
  }
  return out;
}

static void comp_local_confirm_writes(Local *loc) {
  auto write = loc->write;
  loc->write = nullptr;

  while (write) {
    write->confirmed = true;
    write            = write->prev;
  }

  loc->read = true; // Also confirm all subsequent writes.
}

/*
Used for code which takes a local's address. Evicts the local to memory and
makes it "volatile": the compiler can no longer form temporary associations
between such a local and argument registers, because the value may be read
"out of band" through the address.
*/
static void comp_local_evict(Comp *comp, Local *loc) {
  const auto type = loc->location;
  if (type == LOC_MEM) return;

  // Locations are undecided until finalizing the function at the semicolon.
  aver(type == LOC_UNKNOWN);

  comp_local_alloc_mem(comp, loc);
  loc->location = LOC_MEM;

  /*
  The caller may now load data from the address in ways invisible to the
  compiler. So the compiler must assume the value is read at some point.

  Incorrect behavior if we don't do this:

    123      \ mov x0, 123
    { val }  \ nop
    ref' val \ add x0, x29, 16
    @        \ (junk data)

  If we only confirm subsequent writes, but not prior writes:

    123      \ mov x0, 123
    { val }  \ nop
    234      \ Clobber x0; write is never confirmed.
    ref' val \ add x0, x29, 16
    @        \ (junk data)

  Correct behavior if we confirm prior and future writes:

    123     \ mov x0, 123
    { val } \ str x0, [x29, #16]
  */
  comp_local_confirm_writes(loc);

  // Invalidates all register associations for this local.
  loc->vol = true;
}

static const char *comp_local_fmt_reg_bits(Comp *comp, const Local *loc) {
  return uint32_to_bit_str((U32)comp_local_reg_bits(comp, loc));
}

static Err err_args_partial(
  const Sym *sym, const char *action, Sint low, Sint len
) {
  return errf(
    "in " FMT_QUOTED
    " (%s): %s: prior values were partially consumed by assignments: " FMT_SINT
    " of " FMT_SINT
    "; hint: either assign to more locals, or use `--` inside braces to discard unused values",
    sym->name.buf,
    wordlist_name(sym->wordlist),
    action,
    low,
    len
  );
}

static Err err_args_arity(const Sym *sym, const char *action, Sint req, Sint ava) {
  return errf(
    "in " FMT_QUOTED " (%s): %s: arity mismatch: required: " FMT_SINT
    ", provided: " FMT_SINT,
    sym->name.buf,
    wordlist_name(sym->wordlist),
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

  if (arg_low) return err_args_partial(sym, action, arg_low, arg_len);
  if (req != arg_len) return err_args_arity(sym, action, req, arg_len);
  return nullptr;
}

static Err comp_validate_call_args(Comp *comp, const Sym *callee) {
  const auto               name = &callee->name;
  static constexpr auto    cap  = 36 + arr_cap(name->buf);
  static thread_local char buf[cap];

  snprintf(buf, cap, "unable to compile call to " FMT_QUOTED, name->buf);
  return comp_validate_args(comp, buf, callee->inp_len);
}

static Err comp_validate_ret_args(Comp *comp) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  try(comp_validate_args(comp, "unable to compile return", sym->out_len));

  if (!sym->has_err) return nullptr;
  if (sym->out_len) return nullptr;

  return errf(
    "unable to compile return: " FMT_QUOTED
    " has error output but returns no outputs",
    sym->name.buf
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

static Loc_write *comp_append_local_write(Comp *comp, Local *loc, U8 reg) {
  const auto confirm = loc->read || loc->vol;

  const auto fix = stack_push(
    &comp->ctx.loc_fix,
    (Loc_fixup){
      .type  = LOC_FIX_WRITE,
      .write = (Loc_write){
        .instr     = asm_append_breakpoint(comp, ASM_CODE_LOC_WRITE),
        .prev      = loc->write,
        .loc       = loc,
        .reg       = reg,
        .confirmed = confirm,
      }
    }
  );

  return loc->write = &fix->write;
}

static void comp_clear_tracked_arg_reg(Comp *comp, U8 reg) {
  validate_tracked_arg_reg(reg);

  const auto loc          = comp_local_get_for_reg(comp, reg);
  comp->ctx.reg_vals[reg] = (Reg_val){};

  if (!loc || loc->stable || comp_local_has_regs(comp, loc)) return;
  comp_append_local_write(comp, loc, reg);
  loc->stable = true;
}

/*
Must be invoked whenever a register is used for ANYTHING
other than the current word's input or output parameters.
Examples:

- An input argument for the next call.
- Side-effect clobbers in some call.
- Output results from some call.
*/
static Err comp_clobber_reg(Comp *comp, U8 reg) {
  validate_volatile_reg(reg);

  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  bits_add_to(&sym->clobber, reg);

  if (is_tracked_arg_reg(reg)) comp_clear_tracked_arg_reg(comp, reg);
  return nullptr;
}

static Err comp_clobber_regs(Comp *comp, Bits clob) {
  while (clob) try(comp_clobber_reg(comp, bits_pop_low(&clob)));
  return nullptr;
}

static Err comp_clobber_from_call(Comp *comp, const Sym *callee) {
  /*
  The callee's clobber list must include all of its auxiliary side-effectful
  clobbers. But its inputs and outputs may or may not be considered clobbers.
  For example an unary "identity" function which takes one value and returns
  it as-is doesn't really clobber anything. Placing its input in `x0` before
  the call may have clobbered a local which was previously in `x0`, but when
  the input comes from another local, which is now associated with `x0`, the
  second local should not be clobbered by such a call.

  In contrast, "intrin" and "extern" functions are assumed to always
  clobber all volatile registers, including input and output params,
  because they're opaque to us.
  */
  try(comp_clobber_regs(comp, callee->clobber));

  if (callee->has_err) {
    try(comp_clobber_reg(comp, callee->out_len - 1));
  }

  /*
  Volatile locals may be modified by calls; our compiler is unable to detect
  out-of-band loads and stores of locals by their address, because store and
  load operations are implemented in program code via self-assembly.

  So, we use a simple solution: every call evicts all volatile locals
  from argument registers, forcing subsequent access to use memory.
  */
  const auto   ctx  = &comp->ctx;
  constexpr U8 ceil = arr_cap(ctx->reg_vals);

  for (U8 reg = 0; reg < ceil; reg++) {
    const auto loc = comp_local_get_for_reg(comp, reg);

    if (!loc) continue;
    if (!loc->vol) continue;

    ptr_clear(&ctx->reg_vals[reg]);

    IF_DEBUG(eprintf(
      "[debug] in " FMT_QUOTED ": evicted volatile local " FMT_QUOTED
      " from argument register %d before calling " FMT_QUOTED "\n",
      ctx->sym->name.buf,
      loc->name.buf,
      reg,
      callee->name.buf
    ));
  }

  return nullptr;
}

static Err comp_local_reg_reset(Comp *comp, Local *loc, U8 reg) {
  validate_tracked_arg_reg(reg);
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
  try(asm_validate_arg_reg(reg));
  try(comp_local_reg_reset(comp, loc, reg));

  IF_DEBUG(eprintf(
    "[system] appended \"set\" of local " FMT_QUOTED " from register %d",
    loc->name.buf,
    reg
  ));

  return nullptr;
}

/*
Abstract "set" operation invoked via `{ }` braces. Used both for parameter
declarations and regular assignments.

For regular locals, this does not immediately create a "write", as those are
added lazily on clobbers. Does not check, confirm, or invalidate the latest
pending "write" if any; writes are confirmed by "reads", and link with each
other for a chain confirmation.

For volatile locals whose address is available to the code we're compiling,
this has to immediately store the value because it may be read out-of-band
through the local's address.
*/
static Err comp_append_local_set_next(Comp *comp, Local *loc) {
  const auto ctx = &comp->ctx;
  const auto rem = (Sint)ctx->arg_len - (Sint)ctx->arg_low;

  if (!(rem > 0)) return err_assign_no_args(loc->name.buf);

  const auto reg = ctx->arg_low++;
  if (loc->vol) {
    const auto write = comp_append_local_write(comp, loc, reg);
    write->confirmed = true;
    loc->stable      = true;
  }
  else {
    try(comp_local_reg_reset(comp, loc, reg));
  }

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

  if (out) *out = reg;
  return nullptr;
}

static Err comp_next_out_param_reg(Comp *comp, U8 *out) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));

  const auto reg = sym->out_len++;
  try(asm_validate_output_param_reg(reg));

  if (out) *out = reg;
  return nullptr;
}

static Err comp_next_arg_reg(Comp *comp, U8 *out) {
  const auto reg = comp->ctx.arg_len++;
  try(asm_validate_arg_reg(reg));
  try(comp_clobber_reg(comp, reg));
  if (out) *out = reg;
  return nullptr;
}

/*
Caution: only one scratch reg may be requested at a time.
TODO: support asking for multiple scratch regs.
Requires adding the ability to "free" them.
*/
static Err comp_scratch_reg(Comp *comp, U8 *out) {
  const auto reg = comp->ctx.arg_len;
  try(asm_validate_arg_reg(reg));
  validate_volatile_reg(reg);
  try(comp_clobber_reg(comp, reg));
  if (out) *out = reg;
  return nullptr;
}

// Used by `intrin_comp_args_set`.
static void comp_args_set(Comp *comp, U8 next) {
  const auto ctx = &comp->ctx;
  ctx->arg_len   = next;
  ctx->arg_low   = 0;
}

// Concrete "read" operation used as a fallback by "get".
static void comp_append_local_read(Comp *comp, Local *loc, U8 reg) {
  (void)comp;

  comp_local_confirm_writes(loc);

  stack_push(
    &comp->ctx.loc_fix,
    (Loc_fixup){
      .type = LOC_FIX_READ,
      .read = (Loc_read){
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
  try(asm_validate_arg_reg(reg));
  if (comp_local_has_reg(comp, loc, reg)) return nullptr;

  if (!loc->stable) return err_local_get_not_inited(loc->name.buf);

  try(comp_clobber_reg(comp, reg));
  comp_append_local_read(comp, loc, reg);
  comp_local_reg_add(comp, loc, reg);
  return nullptr;
}

/*
Abstract "get" operation invoked by simply mentioning
a local inside a word. May fall back on a "read".

Note that "get" happens as part of building an argument list,
which clobbers argument registers, which may be associated
with other locals. The clobbers are handled in that logic.
*/
static Err comp_append_local_get_next(Comp *comp, Local *loc) {
  auto reg = comp->ctx.arg_len;
  try(asm_validate_arg_reg(reg));

  if (comp_local_has_reg(comp, loc, reg)) {
    IF_DEBUG(eprintf(
      "[debug] local " FMT_QUOTED
      " already associated with register %d; skipping a \"get next\" operation\n",
      loc->name.buf,
      reg
    ));

    comp->ctx.arg_len++;
    return nullptr;
  }

  /*
  Like `comp_next_arg_reg`, with a modification: when the register already has
  the same local, this doesn't clobber the register. So for example, if a word
  simply forwards its inputs to another word, the forwarding doesn't cause its
  parameters to be added to its clobber list.
  */
  comp->ctx.arg_len++;
  if (comp_local_get_for_reg(comp, reg) != loc) {
    try(comp_clobber_reg(comp, reg));
  }

  /*
  When a local is already associated with an argument register,
  we can immediately copy its value from that register into our
  new argument register, and associate them.
  */
  const auto any = comp_local_reg_any(comp, loc);
  if (any >= 0) {
    asm_append_mov_reg(comp, reg, (U8)any);
    comp_local_reg_add(comp, loc, reg);
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
  const auto ctx = &comp->ctx;

  if (ctx->arg_low) {
    Sym *sym;
    try(comp_require_current_sym(comp, &sym));
    return err_args_partial(
      sym, "unable to push immediate value", ctx->arg_low, ctx->arg_len
    );
  }

  try(asm_validate_arg_reg(reg));
  try(comp_clobber_reg(comp, reg));

  const auto instrs = &comp->code.code_write;
  const auto floor  = instrs->len;

  asm_append_imm_to_reg(comp, reg, imm, has_load);

  const auto ceil = instrs->len;

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
  Comp *comp, const Sym *caller, const Sym *callee, bool auto_try
) {
  (void)caller;

  const auto ctx = &comp->ctx;

  /*
  An error, if any, is returned in the last output GPR. By default errors are
  visible. We also support blanket implicit "try" via "try_all". In that case,
  `asm_append_call_norm` and `asm_append_call_intrin` auto-insert the "try".
  Here we just hide the error since it's been checked already.
  */
  if (auto_try && callee->has_err) {
    ctx->arg_len = callee->out_len - 1;
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

static Err comp_append_call_norm(Comp *comp, Sym *callee, bool err_mode) {
  IF_DEBUG(aver(callee->type == SYM_NORM));

  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  try(comp_before_append_call(comp, callee));

  if (callee->norm.inlinable) {
    try(asm_inline_sym(comp, caller, callee, err_mode));
  }
  else {
    try(asm_append_call_norm(comp, caller, callee, err_mode));
    sym_register_call(caller, callee);
  }

  try(comp_after_append_call(comp, caller, callee, err_mode));
  return nullptr;
}

static Err comp_append_call_intrin(Comp *comp, Sym *callee, bool err_mode) {
  IF_DEBUG(aver(callee->type == SYM_INTRIN));
  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  try(comp_before_append_call(comp, callee));
  try(asm_append_call_intrin(comp, caller, callee, err_mode));
  try(comp_after_append_call(comp, caller, callee, err_mode));
  sym_register_call(caller, callee);
  return nullptr;
}

static Err comp_append_call_extern(Comp *comp, Sym *callee) {
  IF_DEBUG(aver(callee->type == SYM_EXTERN));

  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  try(comp_before_append_call(comp, callee));
  asm_append_call_extern(comp, caller, callee);

  constexpr bool err_mode = false;
  try(comp_after_append_call(comp, caller, callee, err_mode));
  sym_register_call(caller, callee);
  return nullptr;
}

/*
Used at every branch point to evict locals from parameter
registers to their "stable" locations.
*/
static Err comp_barrier(Comp *comp, Sint req) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));

  const auto   ctx     = &comp->ctx;
  const auto   arg_low = ctx->arg_low;
  const auto   arg_len = ctx->arg_len;
  constexpr U8 ceil    = arr_cap(ctx->reg_vals);

  if (arg_low) {
    return err_args_partial(sym, "in control flow", arg_low, arg_len);
  }
  if (req != arg_len) {
    return err_args_arity(sym, "in control flow", req, arg_len);
  }
  for (U8 reg = 0; reg < ceil; reg++) {
    comp_clear_tracked_arg_reg(comp, reg);
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
  const auto reg = ASM_PARAM_REG_0;
  try(comp_clobber_reg(comp, reg));
  asm_append_sub_reg_ext(comp, reg, ASM_REG_SP, reg);
  asm_append_sp_align(comp, reg);
  asm_append_add_imm(comp, ASM_REG_SP, reg, 0);
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

/*
Examples without and with `try`:

  fun: word0 { -- err }              err end
  fun: word1 { -- val err }    10    err end
  fun: word2 { -- val val err} 10 20 err end

  word0 { err }
  word1 { val err }
  word2 { val val err }

  err   try
  word0 try
  word1 try { val }
  word2 try { val val }
*/
static Err comp_append_try(Comp *comp, const Sym *sym) {
  if (!sym->has_err) {
    return errf(
      "in " FMT_QUOTED ": unable to `try`: current word has no error output",
      sym->name.buf
    );
  }

  const auto ctx     = &comp->ctx;
  const auto name    = sym->name.buf;
  const auto arg_low = ctx->arg_low;
  const auto arg_len = ctx->arg_len;

  if (arg_low) {
    return err_args_partial(sym, "unable to `try`", arg_low, arg_len);
  }

  if (!arg_len) {
    return errf(
      "in " FMT_QUOTED ": unable to `try`: need at least one argument", name
    );
  }

  const auto src_reg = (U8)(arg_len - 1);
  const auto tar_reg = (U8)asm_sym_err_reg(sym);
  validate_tracked_arg_reg(src_reg);
  validate_tracked_arg_reg(tar_reg);
  asm_append_try(comp, tar_reg, src_reg);

  ctx->arg_len--;
  return nullptr;
}

static Err comp_append_throw(Comp *comp, const Sym *sym) {
  if (!sym->has_err) {
    return errf(
      "in " FMT_QUOTED ": unable to `throw`: current word has no error output",
      sym->name.buf
    );
  }
  try(comp_validate_args(comp, "unable to `throw`", 1));
  comp_clear_args(comp);

  const auto src_reg = ASM_PARAM_REG_0;
  const auto tar_reg = asm_sym_err_reg(sym);
  aver(tar_reg >= 0);

  if (tar_reg != src_reg) {
    try(comp_clobber_reg(comp, (U8)tar_reg));
    asm_append_mov_reg(comp, (U8)tar_reg, (U8)src_reg);
  }

  asm_append_fixup_throw(comp);
  return nullptr;
}

/*
TODO needs additional heuristics. Has way too many false positives.
In addition, the user doesn't always want this. Some of our tests
and examples use named locals which are intentionally ignored.
*/
static void comp_warn_unused_locals(Comp_ctx *) {
  /*
  const auto sym = ctx->sym;
  aver(!!sym);

  for (stack_range(auto, loc, &ctx->locals)) {
    if (loc->location == LOC_UNKNOWN) {
      eprintf(
        "[warning] in " FMT_QUOTED ": unused local " FMT_QUOTED "\n",
        sym->name.buf,
        comp_local_name(loc)
      );
    }
  }
  */
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
