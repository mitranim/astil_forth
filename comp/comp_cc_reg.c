/*
Compilation logic for the register-based version of our system.

Currently hardcodes the assumption "reg number = argument index",
and assumes that inputs and outputs are in the same registers.
This works beautifully on Arm64. If we ever support more arches,
we'd need adapter logic for converting register numbers, which
should be sufficient for RISC-V. x64 is a little trickier; we'd
also need to convert output registers back into input registers,
and do it lazily rather than immediately.
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
    is_aligned(&ctx->args) &&
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

// SYNC[comp_ctx_fields].
static void comp_ctx_trunc(Comp_ctx *ctx) {
  stack_trunc(&ctx->loc_fix);
  stack_trunc(&ctx->asm_fix);
  stack_trunc(&ctx->locals);
  dict_trunc((Dict *)&ctx->local_dict);
  arr_clear(ctx->args);

  ctx->vol_regs = BITS_ALL; // Invalid initial state for bug detection.

  ptr_clear(&ctx->sym);
  ptr_clear(&ctx->anon_locs);
  ptr_clear(&ctx->fp_off);
  ptr_clear(&ctx->saved_reg);
  ptr_clear(&ctx->arg_len);
  ptr_clear(&ctx->compiling);
  ptr_clear(&ctx->redefining);
  ptr_clear(&ctx->try_all);
  ptr_clear(&ctx->slop);
}

// SYNC[comp_ctx_fields].
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

// SYNC[comp_ctx_fields].
static void comp_ctx_rewind(const Comp_ctx *prev, Comp_ctx *next) {
  next->sym        = prev->sym;
  next->anon_locs  = prev->anon_locs;
  next->fp_off     = prev->fp_off;
  next->vol_regs   = prev->vol_regs;
  next->saved_reg  = prev->saved_reg;
  next->arg_len    = prev->arg_len;
  next->redefining = prev->redefining;
  next->compiling  = prev->compiling;
  next->has_alloca = prev->has_alloca;
  next->try_all    = prev->try_all;
  next->slop       = prev->slop;

  stack_rewind(&prev->locals, &next->locals);
  dict_trunc((Dict *)&next->local_dict);
  stack_rewind(&prev->asm_fix, &next->asm_fix);
  stack_rewind(&prev->loc_fix, &next->loc_fix);

  static constexpr U8 ceil = arr_cap(next->args);
  for (U8 ind = 0; ind < ceil; ind++) {
    next->args[ind] = prev->args[ind];
  }
}

static const char *comp_local_name(Local *loc) {
  if (!loc) return "(nil)";
  if (loc->name.len) return loc->name.buf;
  return "(anonymous)";
}

static Comp_arg comp_arg_unknown(void) {
  return (Comp_arg){.type = COMP_ARG_UNKNOWN};
}

static Comp_arg comp_arg_local(Local *loc) {
  return (Comp_arg){
    .type  = COMP_ARG_LOCAL,
    .local = loc,
  };
}

static Local *comp_arg_local_ref(const Comp_arg *arg) {
  switch (arg->type) {
    case COMP_ARG_UNKNOWN: [[fallthrough]];
    case COMP_ARG_IMM:     [[fallthrough]];
    case COMP_ARG_ERR:     return nullptr;
    case COMP_ARG_LOCAL:   return arg->local;
    default:               unreachable();
  }
  return nullptr;
}

static bool comp_local_has_arg(const Comp_ctx *ctx, const Local *loc) {
  if (!loc) return false;

  static constexpr U8 ceil = arr_cap(ctx->args);

  for (U8 reg = 0; reg < ceil; reg++) {
    const auto arg = &ctx->args[reg];
    if (comp_arg_local_ref(arg) == loc) return true;
  }
  return false;
}

static Err comp_args_fold(
  Comp *comp,
  Uint  argc,
  Sint *imm0,
  Sint *imm1,
  Sint *known_count,
  Sint *consumed_count
) {
  if (argc < 1 || argc > 2) {
    return errf("invalid fold arg count: " FMT_UINT, argc);
  }

  const auto ctx = &comp->ctx;
  if (ctx->arg_len < argc) return errf("not enough fold args");

  const U8 reg0 = (U8)(ctx->arg_len - argc);
  auto     arg0 = &ctx->args[reg0];
  auto     arg1 = argc == 2 ? &ctx->args[reg0 + 1] : nullptr;

  Comp_arg_imm src0  = {};
  Comp_arg_imm src1  = {};
  bool         has0  = false;
  bool         has1  = false;
  Uint         known = 0;

  if (arg0->type == COMP_ARG_IMM) {
    src0 = arg0->imm;
    has0 = true;
  }

  if (arg1) {
    if (arg1->type == COMP_ARG_IMM) {
      src1 = arg1->imm;
      has1 = true;
    }
  }

  if (argc == 1) {
    if (has0) known = 1;
  }
  else {
    if (has1) {
      known = 1;
      if (has0) known = 2;
    }
  }

  const auto instrs   = &comp->code.code_write;
  Uint       consumed = 0;

  if (argc == 1) {
    if (has0 && instrs->len == src0.ceil) {
      instrs->len = src0.floor;
      *arg0       = comp_arg_unknown();
      consumed    = 1;
    }
  }
  else {
    if (has1 && instrs->len == src1.ceil) {
      instrs->len = src1.floor;
      *arg1       = comp_arg_unknown();
      consumed    = 1;

      if (has0 && instrs->len == src0.ceil) {
        instrs->len = src0.floor;
        *arg0       = comp_arg_unknown();
        consumed    = 2;
      }
    }
  }

  if (imm0) *imm0 = has0 ? src0.num : 0;
  if (imm1) *imm1 = has1 ? src1.num : 0;
  if (known_count) *known_count = (Sint)known;
  if (consumed_count) *consumed_count = (Sint)consumed;
  return nullptr;
}

static Err err_args_arity(
  const Sym *sym, const char *action, Sint min, Sint max, Sint ava
) {
  if (min == max) {
    return errf(
      "in " FMT_QUOTED " (%s): %s: arity mismatch: required: " FMT_SINT
      ", provided: " FMT_SINT,
      sym->name.buf,
      wordlist_name(sym->wordlist),
      action,
      max,
      ava
    );
  }

  return errf(
    "in " FMT_QUOTED " (%s): %s: arity mismatch: required: between " FMT_SINT
    " and " FMT_SINT ", provided: " FMT_SINT,
    sym->name.buf,
    wordlist_name(sym->wordlist),
    action,
    min,
    max,
    ava
  );
}

static Err comp_validate_args(Comp *comp, const char *action, Sint min, Sint max) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));

  const auto ctx     = &comp->ctx;
  const auto arg_len = ctx->arg_len;

  if (!(arg_len >= min && arg_len <= max)) {
    return err_args_arity(sym, action, min, max, arg_len);
  }
  return nullptr;
}

static Err comp_validate_call_args(Comp *comp, const Sym *callee) {
  const auto               name = &callee->name;
  static constexpr auto    cap  = 36 + arr_cap(name->buf);
  static thread_local char buf[cap];
  const auto               len = callee->inp_len;

  snprintf(buf, cap, "unable to compile call to " FMT_QUOTED, name->buf);
  return comp_validate_args(comp, buf, len, len);
}

static Err comp_validate_recur_args(Comp *comp) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  const auto len = sym->inp_len;
  try(comp_validate_args(comp, "unable to compile recursive call", len, len));
  return nullptr;
}

static void comp_append_local_reloc_from_reg(Comp *comp, Local *loc, U8 reg) {
  const auto confirm = loc->read;

  const auto fix = stack_push(
    &comp->ctx.loc_fix,
    (Loc_fixup){
      .type  = LOC_FIX_RELOC,
      .reloc = (Loc_reloc){
        .instr     = asm_append_breakpoint(comp, ASM_CODE_LOC_RELOC),
        .prev      = loc->reloc,
        .loc       = loc,
        .reg       = reg,
        .confirmed = confirm,
      }
    }
  );

  loc->stable = true;
  loc->reloc  = &fix->reloc;
}

static Err comp_register_clobber(Comp *comp, U8 reg) {
  try(asm_validate_arg_reg(reg));
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  bits_add_to(&sym->clobber, reg);
  return nullptr;
}

// See comment on `Local` for explanation.
static Err comp_forget_reg(Comp *comp, U8 reg) {
  try(asm_validate_arg_reg(reg));

  const auto ctx = &comp->ctx;
  const auto arg = &ctx->args[reg];
  const auto loc = comp_arg_local_ref(arg);
  *arg           = comp_arg_unknown();

  if (!loc || loc->stable || comp_local_has_arg(ctx, loc)) {
    return nullptr;
  }

  comp_append_local_reloc_from_reg(comp, loc, reg);
  IF_DEBUG(assert_fatal(loc->stable));
  return nullptr;
}

/*
Used at callsites and branch points to relocate locals from argument
registers to their stable locations and to forget comptime constants
located in arg registers. The caller is responsible for registering
or not registering this bitset as clobbers.
*/
static Err comp_forget_regs(Comp *comp, Bits regs) {
  const auto          ctx  = &comp->ctx;
  static constexpr U8 ceil = arr_cap(ctx->args);

  for (U8 reg = 0; reg < ceil; reg++) {
    if (!bits_has(regs, reg)) continue;
    try(comp_forget_reg(comp, reg));
  }
  return nullptr;
}

static Err comp_args_set(Comp *comp, Sint next) {
  if (!(next >= 0 && next <= ASM_ARG_LEN_MAX)) return err_too_many_args(next);

  const auto ctx  = &comp->ctx;
  const U8   prev = ctx->arg_len;
  const U8   len  = (U8)next;

  /*
  Growing means the program has done something to the intermediary
  registers. Their values are now unknown to us.

  We don't need this when shrinking, because it only indicates
  logical consumption of values without clobbering those regs.
  */
  if (len > prev) {
    for (U8 reg = prev; reg < len; reg++) {
      ctx->args[reg] = comp_arg_unknown();
    }
  }
  ctx->arg_len = len;
  return nullptr;
}

/*
Lower-level tool for internal use only. The vast majority of callers
should use `comp_append_push_from_local` instead of this. The caller
is also responsible for updating `ctx->arg_len`.

Used internally for both parameter declarations and regular assignments.

For regular locals, this does not immediately create a "reloc" as those are
added lazily on clobbers. Does not check, confirm, or invalidate the latest
pending "reloc" if any; relocs are confirmed by "reads", and link with each
other for a chain confirmation.
*/
static Err comp_assign_local_from_reg(Comp *comp, Local *loc, U8 reg) {
  try(asm_validate_arg_reg(reg));

  const auto ctx = &comp->ctx;

  if (!ctx->arg_len) {
    return errf(
      "unable to assign argument %d to " FMT_QUOTED ": no arguments available",
      reg,
      loc->name.buf
    );
  }

  if (reg >= ctx->arg_len) {
    return errf(
      "unable to assign argument %d to " FMT_QUOTED
      ": only %d arguments available",
      reg,
      loc->name.buf,
      ctx->arg_len
    );
  }

  const auto arg  = &ctx->args[reg];
  const auto prev = comp_arg_local_ref(arg);
  loc->stable     = false;

  static constexpr U8 ceil = arr_cap(ctx->args);

  /*
  Evict the new local from all other registers.
  */
  for (U8 reg0 = 0; reg0 < ceil; reg0++) {
    if (reg == reg0) {
      *arg = comp_arg_local(loc);
    }
    else {
      const auto arg = &ctx->args[reg0];
      if (comp_arg_local_ref(arg) == loc) {
        *arg = comp_arg_unknown();
      }
    }
  }

  if (prev && prev != loc && !prev->stable && !comp_local_has_arg(ctx, prev)) {
    comp_append_local_reloc_from_reg(comp, prev, reg);
  }

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

static Err comp_pop_into_local(Comp *comp, Local *loc) {
  const auto ctx = &comp->ctx;

  if (!ctx->arg_len) {
    return errf(
      "unable to assign to " FMT_QUOTED ": no arguments available", loc->name.buf
    );
  }

  const U8 reg = (U8)(ctx->arg_len - 1);
  try(comp_assign_local_from_reg(comp, loc, reg));
  ctx->arg_len = reg;
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

static Err comp_alloc_next_reg(Comp *comp, U8 *out) {
  try(asm_validate_arg_reg(comp->ctx.arg_len));
  const auto reg = comp->ctx.arg_len++;
  try(comp_forget_reg(comp, reg));
  try(comp_register_clobber(comp, reg));
  if (out) *out = reg;
  return nullptr;
}

// Concrete "read" operation used as a fallback by "get".
static void comp_append_local_read(Comp *comp, Local *loc, U8 reg) {
  auto reloc = loc->reloc;
  loc->reloc = nullptr;

  while (reloc) {
    reloc->confirmed = true;
    reloc            = reloc->prev;
  }

  loc->read = true; // Also confirm all subsequent relocs.

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

static Err comp_append_imm_to_reg(Comp *comp, U8 reg, Sint imm) {
  try(asm_validate_arg_reg(reg));

  try(comp_forget_reg(comp, reg));
  try(comp_register_clobber(comp, reg));

  const auto instrs = &comp->code.code_write;
  const auto floor  = instrs->len;

  asm_append_imm_to_reg(comp, reg, imm);

  comp->ctx.args[reg] = (Comp_arg){
    .type = COMP_ARG_IMM,
    .imm  = {.num = imm, .floor = floor, .ceil = instrs->len},
  };

  IF_DEBUG(eprintf(
    "[system] compiled constant " FMT_SINT " as argument in register %d\n", imm, reg
  ));
  return nullptr;
}

static Err comp_append_push_imm(Comp *comp, Sint imm) {
  const auto ctx = &comp->ctx;
  return comp_append_imm_to_reg(comp, ctx->arg_len++, imm);
}

/*
Invoked by simply mentioning a local inside a word.
Also available as a compiler intrinsic, and is used
by control flow structures in self-compilation.
*/
static Err comp_append_push_from_local(Comp *comp, Local *loc) {
  IF_DEBUG(assert_fatal(!!loc));

  const auto ctx = &comp->ctx;
  try(asm_validate_arg_reg(ctx->arg_len));

  auto       tar_reg = ctx->arg_len++;
  const auto tar_arg = &ctx->args[tar_reg];

  loc->used = true;

  if (comp_arg_local_ref(tar_arg) == loc) {
    IF_DEBUG(eprintf(
      "[debug] local " FMT_QUOTED
      " already associated with register %d; skipping instructions for \"push to local\"\n",
      loc->name.buf,
      tar_reg
    ));
    return nullptr;
  }

  const auto prev_loc = comp_arg_local_ref(tar_arg);
  *tar_arg            = comp_arg_unknown();

  S8 loc_reg = -1;

  /*
  Search for another register already associated with this local. This lets us
  immediately compile a mov instruction and avoid emitting a fixup.
  */
  for (S8 src_reg = (int)arr_cap(ctx->args) - 1; src_reg >= 0; src_reg--) {
    const auto src_arg = &ctx->args[src_reg];

    if (comp_arg_local_ref(src_arg) != loc) continue;
    loc_reg = src_reg;
  }

  if (prev_loc && !prev_loc->stable && !comp_local_has_arg(ctx, prev_loc)) {
    comp_append_local_reloc_from_reg(comp, prev_loc, tar_reg);
  }

  if (loc_reg >= 0) {
    asm_append_mov_reg(comp, tar_reg, (U8)loc_reg);
    try(comp_register_clobber(comp, tar_reg));
    *tar_arg = comp_arg_local(loc);
    return nullptr;
  }

  if (!loc->stable) return err_local_get_not_inited(loc->name.buf);

  comp_append_local_read(comp, loc, tar_reg);
  try(comp_register_clobber(comp, tar_reg));
  *tar_arg = comp_arg_local(loc);
  return nullptr;
}

static Err err_dup_param(const char *name) {
  return errf("duplicate parameter " FMT_QUOTED, name);
}

static Err comp_add_input_param(Comp *comp, Word_str name) {
  if (comp_local_get(comp, name.buf)) return err_dup_param(name.buf);

  U8 reg;
  try(comp_next_inp_param_reg(comp, &reg));
  if (comp_name_discards(name)) return nullptr;

  Local *loc;
  try(comp_local_add(comp, name, &loc));

  const auto ctx = &comp->ctx;
  const auto arg = &ctx->args[reg];

  IF_DEBUG(assert_fatal(arg->type == COMP_ARG_UNKNOWN));

  *arg = comp_arg_local(loc);
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

static Err comp_call_intrin(Interp *interp, const Sym *sym) {
  try(asm_call_intrin(interp, sym));
  return nullptr;
}

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
static Err comp_before_append_call(Comp *comp, const Sym *callee) {
  try(comp_validate_call_args(comp, callee));
  try(comp_forget_regs(comp, callee->clobber));
  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  bits_add_all_to(&caller->clobber, callee->clobber);
  return nullptr;
}

static Err comp_append_auto_try(
  Comp *comp, const Sym *caller, const Sym *callee, Comp_arg_err *out
) {
  try_assert(callee->has_err);

  const U8 caller_reg = caller->out_len - 1;
  const S8 callee_reg = asm_sym_err_reg(callee);
  try_assert(callee_reg >= 0);

  const auto ctx         = &comp->ctx;
  const auto instrs      = &comp->code.code_write;
  const Ind  instr_floor = instrs->len;
  const Sint fix_len     = stack_len(&ctx->asm_fix);
  try_assert(fix_len >= 0 && fix_len <= IND_MAX);
  const Ind fix_ind = (Ind)fix_len;

  asm_append_try(comp, caller_reg, (U8)callee_reg);

  try_assert(instrs->len > instr_floor);
  try_assert(stack_len(&ctx->asm_fix) == fix_ind + 1);

  *out = (Comp_arg_err){
    .try_instr_floor = instr_floor,
    .try_instr_ceil  = instrs->len,
    .try_fix_ind     = fix_ind,
  };
  return nullptr;
}

static Err comp_after_append_call(
  Comp *comp, const Sym *caller, const Sym *callee, bool auto_try
) {
  const auto ctx          = &comp->ctx;
  const bool auto_try_err = auto_try && callee->has_err;

  Comp_arg_err err = {};
  if (auto_try_err) {
    try(comp_append_auto_try(comp, caller, callee, &err));
  }

  /*
  An error, if any, is returned in the last output GPR. By default errors are
  visible. We also support blanket implicit "try" via "try_all". In that case,
  `comp_append_auto_try` inserts the "try" and records it for postfix `catch`.
  Here we hide the error since it's been checked already.
  */
  ctx->arg_len = callee->out_len - auto_try_err;
  for (U8 ind = 0; ind < callee->out_len; ind++) {
    ctx->args[ind] = comp_arg_unknown();
  }
  if (auto_try_err) {
    ctx->args[callee->out_len - 1] = (Comp_arg){.type = COMP_ARG_ERR, .err = err};
  }
  return nullptr;
}

static Err comp_append_call_norm(Comp *comp, Sym *callee, bool err_mode) {
  IF_DEBUG(assert_fatal(callee->type == SYM_NORM));

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
  IF_DEBUG(assert_fatal(callee->type == SYM_INTRIN));
  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  try(comp_before_append_call(comp, callee));
  try(asm_append_call_intrin(comp, caller, callee, err_mode));
  try(comp_after_append_call(comp, caller, callee, err_mode));
  sym_register_call(caller, callee);
  return nullptr;
}

static Err comp_append_call_extern(Comp *comp, Sym *callee) {
  IF_DEBUG(assert_fatal(callee->type == SYM_EXTERN));

  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  try(comp_before_append_call(comp, callee));
  asm_append_call_extern(comp, caller, callee);

  constexpr bool err_mode = false;
  try(comp_after_append_call(comp, caller, callee, err_mode));
  sym_register_call(caller, callee);
  return nullptr;
}

static Err comp_before_append_ret(Comp *comp) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));

  const U8 max = sym->out_len;
  const U8 min = sym->has_err && max ? (U8)(max - 1) : max;

  try(comp_validate_args(comp, "unable to compile return", min, max));

  if (!sym->has_err) return nullptr;

  const auto ctx     = &comp->ctx;
  const auto len     = ctx->arg_len;
  const U8   err_reg = min;

  if (len != max) {
    IF_DEBUG(assert(len == min));
    asm_append_zero_reg(comp, min);
    return nullptr;
  }

  const auto arg = &ctx->args[err_reg];
  if (arg->type != COMP_ARG_IMM) return nullptr;
  const auto err = arg->imm.num;
  if (err) return nullptr;

  return errf(
    "in " FMT_QUOTED
    ": unable to compile return: redundant nil error; hint: the compiler implicitly inserts nil error values",
    sym->name.buf
  );
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

static Err comp_alloca_const(Comp *comp, Sint size, U8 reg) {
  try(asm_validate_arg_reg(reg));

  if (!size) return err_alloca_size_zero();
  if (size < 0) return err_alloca_size_negative(size);
  if (size > (Sint)IND_MAX) return err_alloca_size_exceeds(size, IND_MAX);

  // sub Xd, SP, <off>
  // mov SP, Xd
  asm_append_sub_imm(comp, reg, ASM_REG_SP, asm_align_sp_off((Ind)size));
  asm_append_add_imm(comp, ASM_REG_SP, reg, 0);
  return nullptr;
}

// sub Xd, SP, Xd
// and Xd, Xd, 0xfffffffffffffff0
// mov SP, Xd
static Err comp_alloca_dynamic(Comp *comp, U8 reg) {
  try(asm_validate_arg_reg(reg));
  try(comp_forget_reg(comp, reg));
  try(comp_register_clobber(comp, reg));
  asm_append_sub_reg_ext(comp, reg, ASM_REG_SP, reg);
  asm_append_sp_align(comp, reg);
  asm_append_add_imm(comp, ASM_REG_SP, reg, 0);
  return nullptr;
}

static Err comp_alloca(Comp *comp) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  try(comp_validate_args(comp, "unable to `alloca`", 1, ASM_REGS_VOLATILE));

  const auto ctx = &comp->ctx;
  const U8   reg = (U8)(ctx->arg_len - 1);
  const auto arg = &ctx->args[reg];

  if (arg->type == COMP_ARG_IMM) {
    const auto imm = arg->imm;
    asm_backtrack_instrs_opt(comp, imm.floor, imm.ceil);
    try(comp_forget_reg(comp, reg));
    try(comp_alloca_const(comp, imm.num, reg));
  }
  else {
    try(comp_alloca_dynamic(comp, reg));
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
  const auto arg_len = ctx->arg_len;

  if (!arg_len) {
    return errf(
      "in " FMT_QUOTED ": unable to `try`: need at least one argument", name
    );
  }

  const auto src_reg = (U8)(arg_len - 1);
  const auto tar_reg = (U8)asm_sym_err_reg(sym);
  try(asm_validate_arg_reg(src_reg));
  try(asm_validate_arg_reg(tar_reg));
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

  try(comp_validate_args(comp, "unable to `throw`", 1, 1));
  comp->ctx.arg_len = 0;

  const auto src_reg = ASM_PARAM_REG_0;
  const auto tar_reg = asm_sym_err_reg(sym);
  assert_fatal(tar_reg >= 0);

  if (tar_reg != src_reg) {
    try(comp_register_clobber(comp, (U8)tar_reg));
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
static Err comp_check_unused_locals(Comp_ctx *ctx) {
  if (ctx->slop) return nullptr;

  const auto sym = ctx->sym;
  assert_fatal(!!sym);

  for (stack_range(auto, loc, &ctx->locals)) {
    if (loc->used) continue;
    if (loc->location != LOC_UNKNOWN) continue;

    return errf(
      "in " FMT_QUOTED ": unused local " FMT_QUOTED,
      sym->name.buf,
      comp_local_name(loc)
    );
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
    case LOC_FIX_RELOC: {
      const auto val = &fix->reloc;
      return sprintbuf(
        BUF,
        "{type = reloc, loc = %s (%p), instr = %p, reg = %d, prev = %p, confirmed = %s}",
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
