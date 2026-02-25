/*
This file contains some compilation logic for the stack-based callvention
which is mutually exclusive with the register-based callvention.
*/
#pragma once
#include "./arch.c"
#include "./arch_arm64.h"
#include "./comp.c"
#include "./lib/dict.c"

// clang-format off

// Sanity check used in debugging.
static bool comp_ctx_valid(const Comp_ctx *ctx) {
  return (
    ctx &&
    is_aligned(ctx) &&
    is_aligned(&ctx->sym) &&
    is_aligned(&ctx->asm_fix) &&
    is_aligned(&ctx->locals) &&
    is_aligned(&ctx->local_dict) &&
    is_aligned(&ctx->anon_locs) &&
    is_aligned(&ctx->fp_off) &&
    stack_valid((const Stack *)&ctx->asm_fix) &&
    stack_valid((const Stack *)&ctx->locals) &&
    dict_valid((const Dict *)&ctx->local_dict)
  );
}

// clang-format on

static Err comp_ctx_deinit(Comp_ctx *ctx) {
  try(stack_deinit(&ctx->asm_fix));
  try(stack_deinit(&ctx->asm_fix));
  dict_deinit(&ctx->local_dict);
  ptr_clear(ctx);
  return nullptr;
}

static Err comp_ctx_init(Comp_ctx *ctx) {
  ptr_clear(ctx);
  Stack_opt opt = {.len = 1024};
  try(stack_init(&ctx->asm_fix, &opt));
  try(stack_init(&ctx->locals, &opt));
  return nullptr;
}

static void comp_ctx_trunc(Comp_ctx *ctx) {
  stack_trunc(&ctx->asm_fix);
  stack_trunc(&ctx->locals);
  dict_trunc((Dict *)&ctx->local_dict);

  ptr_clear(&ctx->sym);
  ptr_clear(&ctx->anon_locs);
  ptr_clear(&ctx->fp_off);
  ptr_clear(&ctx->compiling);
  ptr_clear(&ctx->redefining);
}

// SYNC[comp_ctx_rewind].
static void comp_ctx_rewind(Comp_ctx *tar, Comp_ctx *snap) {
  tar->sym        = snap->sym;
  tar->anon_locs  = snap->anon_locs;
  tar->fp_off     = snap->fp_off;
  tar->redefining = snap->redefining;
  tar->compiling  = snap->compiling;
  tar->has_alloca = snap->has_alloca;

  stack_rewind(&snap->locals, &tar->locals);
  dict_rewind(&snap->local_dict, &tar->local_dict);
  stack_rewind(&snap->asm_fix, &tar->asm_fix);
}

// Returns a token representing a local which can be given to Forth code.
// In the reg-based calling convention, this returns a different value.
static Sint local_token(Local *loc) { return loc->fp_off; }

static Err comp_append_push_imm(Comp *comp, Sint imm) {
  asm_append_stack_push_imm(comp, imm);
  IF_DEBUG(eprintf("[system] compiled push of number " FMT_SINT "\n", imm));
  return nullptr;
}

static Err comp_append_local_get_next(Comp *comp, Local *loc) {
  const auto off = loc->fp_off;
  asm_append_local_read(comp, off);
  return nullptr;
}

/*
The language bootstrap file implements this on its own.

static Err comp_append_local_set_next(Comp *comp, Local *loc) {
  const auto off = loc->fp_off;
  asm_append_local_write(comp, off);
  return nullptr;
}
*/

static Err comp_call_intrin(Interp *interp, const Sym *callee) {
  IF_DEBUG(aver(callee->type == SYM_INTRIN));
  typedef Err(Fun)(Interp *);
  const auto fun = (Fun *)callee->intrin;
  const auto err = fun(interp);
  if (callee->throws) return err;
  return nullptr;
}

/*
This doesn't merge all clobbers from the callee because it would be a lie:
in stack-CC, we don't actually track all register clobbers. We do however
track the one we care about: the error reg.
*/
static void comp_add_clobbers(Sym *caller, const Sym *callee) {
  IF_DEBUG(aver(caller->type == SYM_NORM));
  const auto reg = ASM_REG_ERR;
  if (bits_has(callee->clobber, reg)) bits_add_to(&caller->clobber, reg);
}

static Err comp_append_call_norm(
  Comp *comp, const Sym *callee, bool catch, bool *inlined
) {
  IF_DEBUG(aver(callee->type == SYM_NORM));

  Sym *caller;
  try(comp_require_current_sym(comp, &caller));

  if (callee->norm.inlinable) {
    try(comp_inline_sym(comp, caller, callee, catch));
    if (inlined) *inlined = true;
  }
  else {
    try(asm_append_call_norm(comp, caller, callee, catch));
    if (inlined) *inlined = false;
  }
  comp_add_clobbers(caller, callee);
  return nullptr;
}

static Err comp_append_call_intrin(Comp *comp, const Sym *callee, bool catch) {
  IF_DEBUG(aver(callee->type == SYM_INTRIN));
  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  try(asm_append_call_intrin(comp, caller, callee, catch));
  comp_add_clobbers(caller, callee);
  return nullptr;
}

static Err comp_append_call_extern(Comp *comp, const Sym *callee) {
  IF_DEBUG(aver(callee->type == SYM_EXTERN));
  Sym *caller;
  try(comp_require_current_sym(comp, &caller));
  asm_append_call_extern(comp, caller, callee);
  comp_add_clobbers(caller, callee);
  return nullptr;
}
