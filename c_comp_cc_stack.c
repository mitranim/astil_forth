/*
This file contains some compilation logic for the stack-based callvention
which is mutually exclusive with the register-based callvention.
*/
#pragma once
#include "./c_arch.c"
#include "./c_comp.c"
#include "./lib/dict.c"

// clang-format off

// Sanity check used in debugging.
static bool comp_ctx_valid(const Comp_ctx *ctx) {
  return (
    ctx &&
    is_aligned(ctx) &&
    is_aligned(&ctx->sym) &&
    is_aligned(&ctx->fixup) &&
    is_aligned(&ctx->locals) &&
    is_aligned(&ctx->local_dict) &&
    stack_valid((const Stack *)&ctx->fixup) &&
    stack_valid((const Stack *)&ctx->locals) &&
    dict_valid((const Dict *)&ctx->local_dict)
  );
}

// clang-format on

static Err comp_ctx_deinit(Comp_ctx *ctx) {
  try(stack_deinit(&ctx->locals));
  try(stack_deinit(&ctx->fixup));
  dict_deinit(&ctx->local_dict);
  ptr_clear(ctx);
  return nullptr;
}

static Err comp_ctx_init(Comp_ctx *ctx) {
  Stack_opt opt = {.len = 1024};
  try(stack_init(&ctx->fixup, &opt));
  try(stack_init(&ctx->locals, &opt));
  return nullptr;
}

static void comp_ctx_trunc(Comp_ctx *ctx) {
  stack_trunc(&ctx->fixup);
  stack_trunc(&ctx->locals);
  dict_trunc((Dict *)&ctx->local_dict);

  ptr_clear(&ctx->sym);
  ptr_clear(&ctx->compiling);
  ptr_clear(&ctx->redefining);
}

// Returns a token representing a local which can be given to Forth code.
// In the reg-based calling convention, this returns a different value.
static Sint local_token(Local *loc) { return local_fp_off(loc); }

static Err comp_append_push_imm(Comp *comp, Sint imm) {
  asm_append_stack_push_imm(comp, imm);
  IF_DEBUG(eprintf("[system] compiled push of number " FMT_SINT "\n", imm));
  return nullptr;
}

static Err comp_append_local_get(Comp *comp, Local *loc) {
  asm_append_local_read(comp, local_fp_off(loc));
  return nullptr;
}

static Err comp_append_local_set(Comp *comp, Local *loc) {
  asm_append_local_write(comp, local_fp_off(loc));
  return nullptr;
}

static Err comp_call_intrin(Interp *interp, const Sym *sym) {
  aver(sym->type == SYM_INTRIN);
  typedef Err(Fun)(Interp *);
  const auto fun = (Fun *)sym->intrin;
  const auto err = fun(interp);
  return sym->throws ? err : nullptr;
}

static Err comp_append_call_norm(Comp *comp, const Sym *callee) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  asm_append_call_norm(comp, sym, callee);
  return nullptr;
}

static Err comp_append_call_intrin(Comp *comp, const Sym *callee) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  asm_append_call_intrin(comp, sym, callee);
  return nullptr;
}

static Err comp_append_load_extern_ptr(Comp *comp, const char *name) {
  asm_append_load_extern_ptr(comp, name);
  return nullptr;
}

static Err comp_append_call_extern(Comp *comp, const Sym *callee) {
  Sym *sym;
  try(comp_require_current_sym(comp, &sym));
  asm_append_call_extern(comp, sym, callee);
  return nullptr;
}
