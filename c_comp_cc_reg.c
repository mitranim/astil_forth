#pragma once
#include "./c_asm.c"
#include "./c_comp.h"
#include "./c_interp.h"
#include "./c_read.c"
#include "./lib/bits.c"
#include "./lib/dict.c"
#include "./lib/err.c"
#include "./lib/list.c"
#include "./lib/num.h"
#include "./lib/stack.c"
#include "./lib/str.h"

// clang-format off

// Sanity check used in unwinding and debugging.
static bool comp_ctx_valid(const Comp_ctx *ctx) {
  return (
    ctx &&
    is_aligned(ctx) &&
    is_aligned(&ctx->sym) &&
    is_aligned(&ctx->fixup) &&
    is_aligned(&ctx->locals) &&
    is_aligned(&ctx->local_dict) &&
    is_aligned(&ctx->clobber) &&
    is_aligned(&ctx->loc_regs) &&
    stack_valid((const Stack *)&ctx->fixup) &&
    stack_valid((const Stack *)&ctx->locals) &&
    dict_valid((const Dict *)&ctx->local_dict)
  );
}

// clang-format on

static Err comp_ctx_deinit(Comp_ctx *ctx) {
  try(stack_deinit(&ctx->fixup));
  try(stack_deinit(&ctx->locals));
  dict_deinit(&ctx->local_dict);
  ptr_clear(ctx);
  return nullptr;
}

static Err comp_ctx_init(Comp_ctx *ctx) {
  Stack_opt opt = {.len = 128};
  try(stack_init(&ctx->fixup, &opt));
  try(stack_init(&ctx->locals, &opt));
  return nullptr;
}

static void comp_ctx_trunc(Comp_ctx *ctx) {
  stack_trunc(&ctx->fixup);
  stack_trunc(&ctx->locals);
  dict_trunc((Dict *)&ctx->local_dict);
  arr_clear(ctx->loc_regs);

  ptr_clear(&ctx->sym);
  ptr_clear(&ctx->arg_low);
  ptr_clear(&ctx->arg_len);
  ptr_clear(&ctx->clobber);
  ptr_clear(&ctx->body);
  ptr_clear(&ctx->compiling);
  ptr_clear(&ctx->redefining);
}

static void validate_temp_reg(U8 reg) { aver(reg < ASM_PARAM_REG_LEN); }

static bool comp_local_has_reg(Comp *comp, Local *loc, U8 reg) {
  validate_temp_reg(reg);
  return comp->loc_regs[reg] == loc;

  // return bits_has(loc->temp_regs, reg);
}

static S8 comp_local_reg_any(Comp *comp, Local *loc) {
  for (Ind reg = 0; reg < arr_cap(comp->loc_regs); reg++) {
    if (comp->loc_regs[reg] == loc) return (S8)reg;
  }
  return -1;
}

/*
Could be "optimized" by creating two-way associations with extra
book-keeping. There's no point and it would make things fragile.
*/
static bool comp_local_has_regs(Comp *comp, Local *loc) {
  return comp_local_reg_any(comp, loc) >= 0;

  // return loc->temp_regs;
}

static void comp_local_reg_add(Comp *comp, Local *loc, U8 reg) {
  validate_temp_reg(reg);
  aver(!comp->loc_regs[reg]);
  comp->loc_regs[reg] = loc;

  // bits_add_to(&loc->temp_regs, reg);
}

static void comp_local_reg_del(Comp *comp, Local *loc, U8 reg) {
  validate_temp_reg(reg);
  aver(!comp->loc_regs[reg] || (comp->loc_regs[reg] == loc));
  comp->loc_regs[reg] = nullptr;

  // bits_del_from(&loc->temp_regs, reg);
}

static void comp_local_reg_reset(Comp *comp, Local *loc, U8 reg) {
  validate_temp_reg(reg);
  aver(!comp->loc_regs[reg]);

  for (Ind ind = 0; ind < arr_cap(comp->loc_regs); ind++) {
    if (comp->loc_regs[reg] != loc) continue;
    comp->loc_regs[reg] = nullptr;
  }
  comp->loc_regs[reg] = loc;

  /*
  auto regs = loc->temp_regs;
  while (regs) local_reg_del(loc, bits_pop_low(&regs));
  loc->temp_regs = bits_only(reg);
  */
}

static Loc_write *asm_append_local_write(Comp *comp, Local *loc, U8 reg) {
  const auto fix = list_append_ptr(
    &comp->fixup,
    (Asm_fixup){
      .type      = ASM_FIXUP_LOC_WRITE,
      .loc_write = (Loc_write){
        .instr = asm_append_breakpoint(comp, ASM_CODE_LOC_WRITE),
        .loc   = loc,
        .reg   = reg,
      }
    }
  );

  const auto next = &fix->loc_write;
  next->prev      = loc->write;
  loc->write      = next;
  return next;
}

static void asm_append_local_read(Comp *comp, Local *loc, U8 reg) {
  list_append(
    &comp->fixup,
    (Asm_fixup){
      .type      = ASM_FIXUP_LOC_READ,
      .loc_write = (Loc_write){
        .instr = asm_append_breakpoint(comp, ASM_CODE_LOC_READ),
        .loc   = loc,
        .reg   = reg,
      }
    }
  );
}

// static void append_local_write(Local *loc, U8 reg) {
//   const auto next = asm_append_local_write(comp, loc, reg);
//   next->prev      = loc->write;
//   loc->write      = next;
// }

static void comp_local_reg_clobber(Comp *comp, Comp *comp, U8 reg) {
  const auto loc = comp->loc_regs[reg];
  validate_temp_reg(reg);
  comp_local_reg_del(comp, loc, reg);
  if (!loc || comp_local_has_regs(comp, loc)) return;
  asm_append_local_write(comp, loc, reg);
}

static void interp_local_regs_clobber(Interp *interp, Bits regs) {
  const auto comp = &interp->comp;

  while (regs) {
    const auto reg = bits_pop_low(&regs);
    comp_local_reg_clobber(comp, reg);
    bits_add_to(&interp->comp.clobber, reg);
  }
}

/*
Must be provided to Forth code as a compiler intrinsic. Control constructs such
as counted loops must use this to emit a barrier AFTER initializing their own
locals or otherwise using the available arguments.

  #if ->
    use x0 for condition check
    emit clobber barrier
    <body>

  +loop: ->
    create locals; use "set" with x0 x1 x2
    emit clobber barrier
    begin loop
    use "get" with locals
*/
static void interp_local_clobber_barrier(Interp *interp) {
  const auto comp = &interp->comp;

  for (U8 reg = 0; reg < arr_cap(interp->comp.regs); reg++) {
    comp_local_reg_clobber(comp, reg);
  }
}

static Err err_arity_mismatch(const char *name, U8 inp_len, U8 arg_len) {
  return errf(
    "unable to call " FMT_QUOTED
    ": arity mismatch: takes %d parameters, but %d arguments are provided",
    name,
    inp_len,
    arg_len
  );
}

static Err err_call_partial_args(const char *name, U8 low, U8 len) {
  return errf(
    "unable to call " FMT_QUOTED
    ": earlier arguments were partially consumed by assignments: %d of %d; hint: use %d more assignments to clear the arguments",
    name,
    low,
    len,
    len - low
  );
}

static Err comp_validate_argument_and_parameter_len(Comp *comp) {
  const auto sym     = comp->sym;
  const auto name    = sym->name.buf;
  const auto inp_len = sym->inp_len;
  const auto arg_low = comp->arg_low;
  const auto arg_len = comp->arg_len;

  if (!arg_low) {
    if (arg_len == inp_len) return nullptr;
    return err_arity_mismatch(name, inp_len, arg_len);
  }

  if (arg_low >= arg_len) {
    return err_arity_mismatch(name, inp_len, 0);
  }

  return err_call_partial_args(name, arg_low, arg_len);
}

// Concrete "read" operation used as a fallback by "get".
static void comp_append_local_read(Comp *comp, Comp *comp, Local *loc, U8 reg) {
  (void)comp;
  auto write = loc->write;
  loc->write = nullptr;

  while (write) {
    write->confirmed = true;
    write            = write->prev;
  }
  asm_append_local_read(comp, loc, reg);
}

/*
Abstract "get" operation. May fall back on a "read".

Note that "get" happens as part of building an argument list,
which clobbers parameter registers, which may be associated
with other locals. The clobbers are handled in that logic.
*/
static Err comp_append_local_get(Comp *comp, Local *loc) {
  const auto reg = comp->arg_len++;
  try(asm_validate_input_param_reg(reg));
  if (comp_local_has_reg(comp, loc, reg)) return nullptr;

  const auto any = comp_local_reg_any(comp, loc);
  if (any >= 0) {
    asm_append_mov_reg_to_reg(&comp->code, reg, (U8)any);
    return;
  }

  comp_append_local_read(comp, loc, reg);
  comp_local_reg_add(comp, loc, reg);
  return nullptr;
}

static Err err_assign_no_args(const char *name) {
  return errf(
    "unable to assign to " FMT_QUOTED ": there are no available arguments", name
  );
}

/*
Abstract "set" operation. Does not immediately create a "write";
those are added lazily on clobbers. Does not check, confirm, or
invalidate the latest pending "write" if any; writes are confirmed
by "reads", and link with each other for a chain confirmation.
*/
static Err comp_append_local_set(Comp *comp, Local *loc) {
  const auto rem = (Sint)comp->arg_len - (Sint)comp->arg_low;
  if (!(rem > 0)) return err_assign_no_args(loc->name.buf);

  const auto reg = comp->arg_low++;

  // Failure in this assertion implies an unhandled clobber.
  aver(!comp_local_has_reg(comp, loc, reg));
  comp_local_reg_reset(comp, loc, reg);
  return nullptr;
}

static Err asm_validate_input_param_reg(U8 reg) {
  if (reg <= ASM_PARAM_REG_LEN) return nullptr;
  return errf(
    "too many input parameters: %d registers available, %d parameters requested",
    reg,
    ASM_PARAM_REG_LEN
  );
}

static Err asm_validate_output_param_reg(U8 reg) {
  if (reg <= ASM_PARAM_REG_LEN) return nullptr;
  return errf(
    "too many output parameters: %d registers available, %d parameters requested",
    reg,
    ASM_PARAM_REG_LEN
  );
}

static Err err_dup_param(const char *name) {
  return errf("duplicate parameter " FMT_QUOTED, name);
}

static Err comp_add_input_param(Comp *comp, Word_str name) {
  if (comp_local_get(comp, name.buf)) return err_dup_param(name.buf);

  Local *loc;
  try(comp_local_add(comp, name, &loc));

  const auto sym = comp->ctx.sym;
  const auto reg = sym->inp_len++;

  try(asm_validate_input_param_reg(reg));
  comp_local_reg_add(comp, loc, reg);
  bits_add_to(&sym->clobber, reg);
  return nullptr;
}

/*
We don't immediately declare output parameters by name, because they're not
initialized. To be used, they have to be assigned, which also automatically
declares them. However, we keep their count, for the call signature.
*/
static Err comp_add_output_param(Comp *comp, Word_str name) {
  (void)name;
  const auto sym = comp->ctx.sym;
  const auto reg = sym->out_len++;

  try(asm_validate_output_param_reg(reg));
  bits_add_to(&comp->clobber, reg);
  bits_add_to(&comp->sym->clobber, reg);
  return nullptr;
}

static Err interp_brace_assignment(Interp *interp) {
  const auto read = interp->reader;
  const auto comp = &interp->comp;
  comp->body      = true;

  for (;;) {
    try(read_word(read));
    const auto word = read->word;

    if (str_eq(&word, "}")) return nullptr;
    if (str_eq(&word, "--")) break;

    Local *loc;
    try(comp_local_get_or_make(comp, word, &loc));
    try(comp_append_local_set(comp, loc));
  }

  // Treat the rest as comments for the sake of consistent
  // syntax between parameters and assignments.
  //
  // TODO: consider requiring the count to match the remaining
  // arg len.
  comp->ctx.arg_low = comp->ctx.arg_len = 0;
  for (;;) {
    try(read_word(read));
    const auto word = read->word;
    if (str_eq(&word, "}")) return nullptr;
  }
  return nullptr;
}

static Err interp_brace_params(Interp *interp) {
  const auto read = interp->reader;
  const auto comp = &interp->comp;
  comp->body      = true;

  for (;;) {
    try(read_word(read));
    const auto word = read->word;
    if (str_eq(&word, "}")) return nullptr;
    if (str_eq(&word, "--")) break;
    try(comp_add_input_param(comp, word));
  }

  for (;;) {
    try(read_word(read));
    const auto word = read->word;
    if (str_eq(&word, "}")) return nullptr;
    try(comp_add_output_param(comp, word));
  }
  return nullptr;
}

static Err intrin_brace(Interp *interp) {
  const auto comp = &interp->comp;
  if (!comp->body) return interp_brace_params(interp);
  return interp_brace_assignment(interp);
}

// FIXME cleanup
// FIXME rename "compile_" procs to "interp_comp_"
static Err err_undefined_word(const char *name);
static Err interp_require_sym(Interp *interp, const char *name, Sym **out);
Err        interp_call_sym(Interp *interp, const Sym *sym);
Err        comp_append_call_sym(Comp *comp, Sym *callee);

static Err comp_append_push_imm(Comp *comp, Sint imm) {
  const auto reg = comp->arg_len++;
  try(asm_validate_input_param_reg(reg));
  asm_append_imm_to_reg(&comp->code, reg, imm, nullptr);

  IF_DEBUG(eprintf(
    "[system] compiled literal number " FMT_SINT " as argument in register %d\n",
    imm,
    reg
  ));
}

static void FIXME_add_to_interp_init(Interp *interp) {
  comp_deinit(&interp->comp);
}

static void FIXME_add_to_colon(Interp *interp) {
  const auto comp = &interp->comp;
  comp_trunc(comp);

  Sym *sym  = {};  // FIXME use actual symbol!
  comp->sym = sym; // FIXME set other fields too
}

static Err FIXME_add_to_semicolon(Interp *interp) {
  const auto comp = &interp->comp;
  // ... finalize the other stuff
  bits_add_all_to(&comp->sym->clobber, comp->clobber);
  comp_trunc(comp);
  return nullptr;
}

/*

↑ new stuff

Interfaces with existing code.

↓ old stuff that wants to be consistent between native and stack

*/

static Err comp_call_intrin(Interp *interp, const Sym *sym) {
  // transfer input args from control stack
  // figure out what to do about output args
  // call intrinsic and assume it returns an error; fix the signatures for all

  return err_str("not yet implemented");
}

static Err comp_append_call_norm(Comp *comp, Comp *comp) {
  const auto sym = comp->ctx.sym;

  // Must include: inputs, outputs, auxiliary clobbers.
  auto clob = sym->clobber;
  while (clob) comp_local_reg_clobber(comp, bits_pop_low(&clob));

  try(comp_validate_argument_and_parameter_len(comp));

  // Call WITHOUT resetting input count. Some control structures use intrinsics
  // to "set" the currently available args into new locals.

  // if (sym->immediate) try(call_sym(sym));
  // else try(comp_append_call_sym(sym));

  // arg_len = sym->out_len;
  // arg_low = 0;

  return err_str("not yet implemented");
}
