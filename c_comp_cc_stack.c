#pragma once
#include "./c_arch.c"
#include "./c_comp.c"
#include "./lib/dict.c"

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
    list_valid((const List *)&ctx->fixup) &&
    stack_valid((const Stack *)&ctx->locals) &&
    dict_valid((const Dict *)&ctx->local_dict)
  );
}

// clang-format on

static Err comp_ctx_deinit(Comp_ctx *ctx) {
  try(stack_deinit(&ctx->locals));
  list_deinit(&ctx->fixup);
  dict_deinit(&ctx->local_dict);
  ptr_clear(ctx);
  return nullptr;
}

static Err comp_ctx_init(Comp_ctx *ctx) {
  Stack_opt opt = {.len = 128};
  try(stack_init(&ctx->locals, &opt));
  return nullptr;
}

static void comp_ctx_trunc(Comp_ctx *ctx) {
  list_trunc(&ctx->fixup);
  stack_trunc(&ctx->locals);
  dict_trunc((Dict *)&ctx->local_dict);

  ptr_clear(&ctx->sym);
  ptr_clear(&ctx->compiling);
  ptr_clear(&ctx->redefining);
}

// SYNC[local_fp_off].
static Sint local_fp_off(Ind ind) {
  return -(((Sint)ind + 1) * (Sint)sizeof(Sint));
}

// Used for calculating the FP offset.
static Ind comp_local_ind(Comp *comp, Local *loc) {
  return stack_ind(&comp->ctx.locals, loc);
}

static Err comp_append_push_imm(Comp *comp, Sint imm) {
  asm_append_stack_push_imm(comp, imm);
  IF_DEBUG(eprintf("[system] compiled push of number " FMT_SINT "\n", imm));
  return nullptr;
}

static Err comp_append_local_get(Comp *comp, Local *loc) {
  const auto off = local_fp_off(comp_local_ind(comp, loc));
  asm_append_local_read(comp, off);
  return nullptr;
}

static Err comp_append_local_set(Comp *comp, Local *loc) {
  const auto off = local_fp_off(comp_local_ind(comp, loc));
  asm_append_local_write(comp, off);
  return nullptr;
}

static Err comp_call_intrin(Interp *interp, const Sym *sym) {
  if (sym->throws) {
    typedef Err(Fun)(Interp *);
    const auto fun = (Fun *)sym->intrin;
    return fun(interp);
  }

  typedef void(Fun)(Interp *);
  const auto fun = (Fun *)sym->intrin;
  fun(interp);
  return nullptr;
}

static void comp_append_call_norm(Comp *comp, Sym *caller, const Sym *callee) {
  asm_append_call_norm(comp, caller, callee);
}
